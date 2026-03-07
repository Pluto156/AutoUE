from core.BaseLLMNode import BaseLLMNode
from langchain.tools import Tool
from textwrap import dedent
from model_description.doc_embed.embed_pcg_docs import query_documentation, query_parameter_space
from langchain.agents import initialize_agent, AgentType
from core.BaseLLMNode import GraphState

# =========================================================
# Prompt: PCG Graph Composer Agent Prompt
# =========================================================
PCG_GRAPH_COMPOSER_PROMPT = """
You are an Unreal Engine 5 **Procedural Content Generation (PCG) Graph composition expert**.

Your core responsibility is:
Based on the Object Generation Plan produced by the KeyElementExtractor,
generate **one PCG node chain per model**, strictly following the predefined
PCG node pattern library.

The final output will be used to automatically generate PCG Graphs in Unreal Engine,
therefore it must meet production-level stability requirements.

------------------------------------------------------------
CRITICAL ONE-TO-ONE MAPPING RULE (VERY IMPORTANT)
------------------------------------------------------------

1. The "Object Generation Plan" contains a list called "models".
2. For EACH model entry in that list, you MUST generate EXACTLY ONE
   corresponding item in the output JSON array.
3. The output array length MUST be exactly equal to the number of input models.
4. The value of the output field "model" MUST EXACTLY MATCH the input
   models[i].name string (case-sensitive, no renaming).
5. You are STRICTLY FORBIDDEN to:
   - Skip any model
   - Merge multiple models into one output item
   - Generate extra output items that do not correspond to an input model

This is a STRICT one-to-one relationship.

------------------------------------------------------------
# 【⚠ STRICT RULES — Node connections MUST strictly follow the rules below】
------------------------------------------------------------

You MUST choose ONE of the following two patterns.
You are NOT allowed to create new patterns, mix patterns, or change node order.

## 1 Large Actor Placement
The node chain MUST be EXACTLY:

PCGGetLandscapeSettings →
PCGSurfaceSamplerSettings →
PCGTransformPointsSettings →
PCGSelectRandomPointsSettings →
PCGBoundsModifierSettings →
PCGSelfPruningSettings →
PCGSpawnActorSettings

## 2 Small Actor Placement with Exclusion
The node chain MUST be EXACTLY:

PCGGetLandscapeSettings →
PCGSurfaceSamplerSettings →
PCGTransformPointsSettings →
PCGSelectRandomPointsSettings →
PCGBoundsModifierSettings →
PCGSelfPruningSettings →
PCGDifferenceSettings →
PCGCollapseSettings →
PCGSpawnActorSettings

 NOT allowed:
- Adding new nodes
- Removing any node
- Changing node order
- Creating a third pattern
- Merging the two patterns
- Using any node not listed above

------------------------------------------------------------
# 【Input Content】
------------------------------------------------------------

You will receive the following two parts as input:

1. **Scene Description**
{scene_description}

2. **Object Generation Plan (JSON)**

Example:
{
  "models": [
    {
      "name": "OakTree",
      "placement_method": "surface_sampling",
      "placement_description": "Distributed at medium density on forest terrain."
    },
    {
      "name": "RockSmall",
      "placement_method": "surface_sampling_with_exclusion",
      "placement_description": "Scattered sparsely, avoiding trees."
    }
  ]
}

------------------------------------------------------------
# 【Output Requirements — STRICT】
------------------------------------------------------------

The output MUST be a strict JSON array.
Do NOT include comments, explanations, or any extra natural language.

For an input with N models, the output array MUST contain EXACTLY N items.

Each item MUST correspond to exactly ONE input model.

------------------------------------------------------------
# 【Correct One-to-One Output Example】
------------------------------------------------------------

Input models:
["OakTree", "RockSmall"]

Correct output:

[
  {
    "model": "OakTree",
    "pattern_desc": "Large Actor Placement@Medium density trees aligned to terrain",
    "nodes": [
      "PCGGetLandscapeSettings@[0,0]",
      "PCGSurfaceSamplerSettings@[200,0]",
      "PCGTransformPointsSettings@[400,0]",
      "PCGSelectRandomPointsSettings@[600,0]",
      "PCGBoundsModifierSettings@[800,0]",
      "PCGSelfPruningSettings@[1000,0]",
      "PCGSpawnActorSettings@[1200,0]"
    ]
  },
  {
    "model": "RockSmall",
    "pattern_desc": "Small Actor Placement with Exclusion@Sparse rocks avoiding trees",
    "nodes": [
      "PCGGetLandscapeSettings@[0,200]",
      "PCGSurfaceSamplerSettings@[200,200]",
      "PCGTransformPointsSettings@[400,200]",
      "PCGSelectRandomPointsSettings@[600,200]",
      "PCGBoundsModifierSettings@[800,200]",
      "PCGSelfPruningSettings@[1000,200]",
      "PCGDifferenceSettings@[1200,200]",
      "PCGCollapseSettings@[1400,200]",
      "PCGSpawnActorSettings@[1600,200]"
    ]
  }
]

------------------------------------------------------------
# 【Final Reminder】
------------------------------------------------------------

- One model → one output item
- No missing models
- No extra models
- Node order MUST strictly follow the selected pattern
- Output MUST be valid JSON only
"""

# =========================================================
# Tool wrapper functions
# =========================================================

def make_tools():
    """Wrap query_documentation / query_parameter_space as LangChain Tools"""
    def query_docs_tool(input_str: str):
        try:
            node_name, query = input_str.split(":", 1)
        except ValueError:
            return "Invalid input format. Expected: <NodeName>:<Query>"
        return query_documentation(node_name.strip(), query.strip())

    def query_params_tool(input_str: str):
        try:
            node_name, query = input_str.split(":", 1)
        except ValueError:
            return "Invalid input format. Expected: <NodeName>:<Query>"
        return query_parameter_space(node_name.strip(), query.strip())

    return [
        Tool(
            name="query_documentation",
            func=query_docs_tool,
            description=dedent("""\
                Query official documentation or functionality of a specific PCG node.
                Input format: "<NodeName>:<Query>"
            """),
        ),
        Tool(
            name="query_parameter_space",
            func=query_params_tool,
            description=dedent("""\
                Query the parameter space, adjustable ranges, and typical usage of a PCG node.
                Input format: "<NodeName>:<Query>"
            """),
        ),
    ]


def inject_node_docs(basellmnode, state, current_input):
    """
    1. The main model predicts which node names need to be understood
    2. A small agent queries documentation
    3. Returns concatenated text
    """
    llm_model = basellmnode.model
    tools = make_tools()

    prompt_for_nodes = f"""
Please predict all PCG node names that may be involved in this scene generation:
Scene description: {state.scene_description}
Object generation plan: {state.llm_outputs.get(basellmnode.prev_node.name, "")}
Output ONLY a Python list of node names.
"""

    predicted_nodes_str = llm_model(prompt_for_nodes)

    try:
        node_names = eval(predicted_nodes_str)
        if not isinstance(node_names, list):
            node_names = []
    except Exception:
        node_names = []

    agent = initialize_agent(
        tools=tools,
        llm=llm_model,
        agent_type=AgentType.ZERO_SHOT_REACT_DESCRIPTION,
        verbose=False
    )

    info_list = []
    for node in node_names:
        doc = agent.run(f"{node}:Function description")
        params = agent.run(f"{node}:Parameter description")
        info_list.append(
            f"Node: {node}\n"
            f"Function description: {doc}\n"
            f"Parameter description: {params}\n"
        )

    return "\n".join(info_list)


def GetInput(node: BaseLLMNode, state: GraphState, full_input: str):
    node.full_input = state.scene_description
    node.full_input += "\n" + state.llm_outputs.get("KeyElementExtractor", "")


def create_pcg_graph_composer() -> BaseLLMNode:
    return BaseLLMNode(
        name="PcgGraphComposer",
        prompt=PCG_GRAPH_COMPOSER_PROMPT,
        enable_feedback=False,
        pre_action=GetInput,
    )


pcg_graph_composer = BaseLLMNode(
    name="PcgGraphComposer",
    prompt=PCG_GRAPH_COMPOSER_PROMPT,
    enable_feedback=False,
    pre_action=GetInput,
)
