from __future__ import annotations

from core.BaseLLMNode import BaseLLMNode, GraphState
from core.phase2_validation import validate_phase2_node_output
from custom_nodes.phase2_file_writer import write_files_from_output

TYPESCRIPT_INTERACTIVE_OBJECT_GENERATOR_PROMPT = """SCHEMA: TypeScriptInteractiveObjectGenerator
Select TypeScript/PuerTS interactive_object templates and fill template_inputs. Return JSON only.
"""

def build_interactive_context(node: BaseLLMNode, state: GraphState, full_input: str) -> None:
    required = {
        "EntityAbilityBehaviorPlanner": state.llm_outputs.get("EntityAbilityBehaviorPlanner", ""),
        "PuerTSRuntimeMappingPlanner": state.llm_outputs.get("PuerTSRuntimeMappingPlanner", ""),
        "TypeScriptScriptAnalyzer": state.llm_outputs.get("TypeScriptScriptAnalyzer", ""),
    }
    missing = [name for name, value in required.items() if not value.strip()]
    if missing:
        raise RuntimeError(f"TypeScriptInteractiveObjectGenerator missing upstream outputs: {missing}")
    node.full_input = "\n\n".join(f"{name} JSON:\n{value}" for name, value in required.items()) + (
        "\n\nGenerate interactive object template_inputs. Include flow_id and runtime_mapping_path from the mapping."
    )

def write_interactive_files(state: GraphState, output: str) -> None:
    write_files_from_output(state, "TypeScriptInteractiveObjectGenerator", output)

def create_typescript_interactive_object_generator() -> BaseLLMNode:
    return BaseLLMNode(
        name="TypeScriptInteractiveObjectGenerator",
        prompt=TYPESCRIPT_INTERACTIVE_OBJECT_GENERATOR_PROMPT,
        pre_action=build_interactive_context,
        output_validator=validate_phase2_node_output,
        post_action=write_interactive_files,
        enable_feedback=False,
    )
