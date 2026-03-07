from langgraph.types import Command, interrupt
from langchain_core.messages import HumanMessage, AIMessage
from langgraph.graph import END
from dataclasses import dataclass, field
from typing import List, Dict, Any
import copy
import time

# =====================================================
# GraphState: Dataclass with helper methods
# =====================================================
@dataclass
class GraphState:
    messages: List[Any] = field(default_factory=list)
    user_description: str = ""
    scene_description: str = ""
    gameplay_description: str = ""
    llm_outputs: Dict[str, str] = field(default_factory=dict)
    demo_name:str = ""
    save_dir:str = ""
    node_token_usage: Dict[str, Dict[str, int]] = field(default_factory=dict)
    node_execution_time: Dict[str, float] = field(default_factory=dict)

    def copy(self) -> "GraphState":
        return copy.deepcopy(self)

    def print(self, title: str = "[DEBUG] Current GraphState"):
        print("\n" + "=" * 60)
        print(f"{title}")
        print("=" * 60)

        for key, value in self.__dict__.items():
            if key == "messages":
                print(f"{key}:")
                if isinstance(value, list):
                    for i, msg in enumerate(value):
                        content = getattr(msg, "content", msg)
                        print(f"  [{i}] {type(msg).__name__}: {content}")
                else:
                    print(f"  (Non-list type) {value}")

            elif key == "llm_outputs":
                print(f"{key}:")
                if isinstance(value, dict):
                    for node_name, output in value.items():
                        preview = output[:200].replace("\n", " ")
                        if len(output) > 200:
                            preview += "..."
                        print(f"  {node_name}: {preview}")
                else:
                    print(f"  (Non-dict type) {value}")
            elif key == "node_token_usage":
                print("node_token_usage:")
                for node_name, usage in value.items():
                    print(f"  {node_name}: {usage}")
            elif key == "node_execution_time":
                print("node_execution_time:")
                for node_name, t in value.items():
                    print(f"  {node_name}: {t:.4f} sec")        
            else:
                print(f"{key}: {value}")
        print("=" * 60 + "\n")


# =====================================================
# Utility functions
# =====================================================
def get_message_text(msg_obj):
    if isinstance(msg_obj, (HumanMessage, AIMessage)):
        return getattr(msg_obj, "content", "")
    elif isinstance(msg_obj, dict):
        return msg_obj.get("content", "")
    return str(msg_obj)


def human_approval(stage_name: str, output: str):
    print(f"[DEBUG] Human approval called for {stage_name}")
    approval = interrupt({
        "stage": stage_name,
        "llm_output": output,
        "instruction": "Leave empty to continue to the next node. Any other input will be used as feedback to retry the current node."
    })
    print(f"[DEBUG] Received human approval input: {approval}")
    return approval


# =====================================================
# BaseLLMNode definition
# =====================================================
class BaseLLMNode:
    def __init__(
        self,
        name: str,
        prompt: str = None,
        next_node: "BaseLLMNode" = None,
        prev_node: "BaseLLMNode" = None,
        enable_feedback: bool = True,
        pre_action: callable = None,
        post_action: callable = None,
        extra_prompt_action: callable = None,
        copy_graph_state: GraphState = None,
        useLastNodeOutPut: bool = True,
        isDebug: bool = True
    ):
        self.name = name
        self.model = None
        self.prompt = prompt
        self.next_node = next_node
        self.prev_node = prev_node
        self.enable_feedback = enable_feedback
        self.pre_action = pre_action
        self.post_action = post_action
        self.extra_prompt_action = extra_prompt_action
        self.executed = False
        self.copy_graph_state = copy_graph_state
        self.isHead = False
        self.full_input = None
        self.useLastNodeOutPut = useLastNodeOutPut
        self.isDebug = isDebug

    def reset(self):
        self.executed = False
        self.full_input = None
        self.copy_graph_state = None

    # =====================================================
    # Build model input
    # =====================================================
    def build_input(self, state: GraphState) -> str:
        state.user_description = get_message_text(state.messages)
        desc = state.user_description

        if self.isHead:
            return f"【User Description】:\n{desc}"

        if self.useLastNodeOutPut:
            prev_name = self.prev_node.name if self.prev_node else "None"
            prev_output = ""
            if self.prev_node and isinstance(self.prev_node, BaseLLMNode):
                prev_output = state.llm_outputs.get(self.prev_node.name, "")

            return (
                f"【User Description】:\n{desc}\n\n"
                f"【Previous Node {prev_name} Output】:\n{prev_output}"
            )

        return f"【User Description】:\n{desc}"


    def invoke_model_with_token_tracking(self, messages, state: GraphState):
        response = self.model.invoke(messages)
        prompt_tokens = 0
        completion_tokens = 0
        total_tokens = 0

        try:
            usage = response.response_metadata.get("token_usage", {})
            prompt_tokens = usage.get("input_tokens", 0)
            completion_tokens = usage.get("output_tokens", 0)
            total_tokens = prompt_tokens + completion_tokens
        except Exception:
            pass

        if self.name not in state.node_token_usage:
            state.node_token_usage[self.name] = {
                "prompt_tokens": 0,
                "completion_tokens": 0,
                "total_tokens": 0
            }

        state.node_token_usage[self.name]["prompt_tokens"] += prompt_tokens
        state.node_token_usage[self.name]["completion_tokens"] += completion_tokens
        state.node_token_usage[self.name]["total_tokens"] += total_tokens

        if self.isDebug:
            print(f"[TOKEN] {self.name} | +{total_tokens} tokens "
                  f"(prompt={prompt_tokens}, completion={completion_tokens})")

        return response

    # =====================================================
    # Abstracted model call logic (AgentLLMNode overrides this)
    # =====================================================
    def call_model(self, full_input: str, state: GraphState):
        """Default model invocation logic"""
        if self.isDebug:
            print(f"[DEBUG] Calling model with full_input:\n{full_input}")
        response = self.invoke_model_with_token_tracking([
        {"role": "system", "content": self.prompt or ""},
        {"role": "user", "content": full_input}
    ], state)
        return response.content.strip()

    # =====================================================
    # Main execution logic
    # =====================================================
    def execute(self, state: GraphState):

        node = self.name

        if self.isDebug:
            print(f"\n[DEBUG] === Executing {node} ===")
            state.print(f"[DEBUG] {node} Input State")

        llm_outputs = state.llm_outputs
        feedback = getattr(state, f"{node}_feedback", None)
        self.full_input = self.build_input(state)

        start_time = time.perf_counter()

        if not self.executed:

            # -------- extra prompt --------
            if callable(self.extra_prompt_action):
                try:
                    if self.isDebug:
                        print(f"[DEBUG] Executing {node}'s extra_prompt_action ...")
                    extra_text = self.extra_prompt_action(self, state, self.full_input)
                    if isinstance(extra_text, str) and extra_text.strip():
                        self.full_input += (
                            f"\n\n【Additional Context】:\n{extra_text.strip()}"
                        )
                except Exception as e:
                    if self.isDebug:
                        print(f"[WARN] extra_prompt_action failed: {e}")

            # -------- pre action --------
            if callable(self.pre_action):
                try:
                    if self.isDebug:
                        print(f"[DEBUG] Executing {node}'s pre_action ...")
                    self.pre_action(self, state, self.full_input)
                except Exception as e:
                    if self.isDebug:
                        print(f"[WARN] pre_action failed: {e}")

            self.executed = True

            if feedback:
                self.full_input += f"\n{feedback}"

            # -------- LLM call --------
            output = self.call_model(self.full_input, state)
            llm_outputs[node] = output

            if self.isDebug:
                print(f"[DEBUG] {node} output (initial): {output}")

            # -------- post action --------
            if callable(self.post_action):
                try:
                    if self.isDebug:
                        print(f"[DEBUG] Executing {node}'s post_action ...")
                    self.post_action(state, output)
                except Exception as e:
                    if self.isDebug:
                        print(f"[WARN] post_action failed: {e}")

        output = llm_outputs.get(node, "(No Output)")

        end_time = time.perf_counter()
        elapsed = end_time - start_time

        if self.name not in state.node_execution_time:
            state.node_execution_time[self.name] = 0.0

        state.node_execution_time[self.name] += elapsed

        if self.isDebug:
            print(f"[TIME] {self.name} +{elapsed:.4f} sec")

        self.copy_graph_state = state.copy()

        if self.isDebug:
            self.copy_graph_state.print(f"[DEBUG] Saved state after {node}")

        if self.enable_feedback:
            approval = human_approval(node, output)
            self.executed = False
            if approval.strip() != "":
                return Command(update={f"{node}_feedback": approval}, goto=node)
        else:
            approval = None

        next_node_name = self.next_node.name if self.next_node else None

        if self.isDebug:
            print(f"[DEBUG] next_node_name = {next_node_name}")

        goto = next_node_name if not approval or approval.strip() == "" else node

        return Command(update={**self.copy_graph_state.__dict__}, goto=goto or END)


    # =====================================================
    # Helper methods
    # =====================================================
    def set_prev_node(self, node: "BaseLLMNode"):
        self.prev_node = node
        if self.isDebug:
            print(f"[DEBUG] Set previous node of {self.name} to {node.name}")
        return self

    def set_next_node(self, node: "BaseLLMNode"):
        self.next_node = node
        if self.isDebug:
            print(f"[DEBUG] Set next node of {self.name} to {node.name}")
        return self

    def set_model(self, model):
        self.model = model
        if self.isDebug:
            print(f"[DEBUG] Set LLM model of {self.name} to {model}")
        return self
        # =====================================================
    # Change prompt dynamically
    # =====================================================
    def change_prompt(self, new_prompt: str):
        """
        Change the system prompt of this node.

        Args:
            new_prompt (str): New system prompt
        """
        if not isinstance(new_prompt, str):
            raise TypeError("new_prompt must be a string")

        self.prompt = new_prompt

        if self.isDebug:
            print(f"[DEBUG] Prompt of node '{self.name}' changed.")
            preview = new_prompt[:200].replace("\n", " ")
            if len(new_prompt) > 200:
                preview += "..."
            print(f"[DEBUG] New prompt preview: {preview}")

        return self
