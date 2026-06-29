from __future__ import annotations

from core.BaseLLMNode import BaseLLMNode, GraphState
from core.phase2_validation import validate_phase2_node_output

TYPESCRIPT_SCRIPT_ANALYZER_PROMPT = """SCHEMA: TypeScriptScriptAnalyzer
Convert PuerTS runtime mappings into behavior-level TypeScript/PuerTS implementation slots. Support bridge files are generated later by TypeScriptCodeGenerator. Return JSON only.
"""

def build_ts_context(node: BaseLLMNode, state: GraphState, full_input: str) -> None:
    required = {
        "EntityAbilityBehaviorPlanner": state.llm_outputs.get("EntityAbilityBehaviorPlanner", ""),
        "PuerTSRuntimeMappingPlanner": state.llm_outputs.get("PuerTSRuntimeMappingPlanner", ""),
    }
    missing = [name for name, value in required.items() if not value.strip()]
    if missing:
        raise RuntimeError(f"TypeScriptScriptAnalyzer missing upstream outputs: {missing}")
    node.full_input = "\n\n".join(f"{name} JSON:\n{value}" for name, value in required.items()) + (
        "\n\nFor every mapping, emit exactly one implementation slot whose target_ts_file equals mapping.runtime_owner."
    )

def create_typescript_script_analyzer() -> BaseLLMNode:
    return BaseLLMNode(
        name="TypeScriptScriptAnalyzer",
        prompt=TYPESCRIPT_SCRIPT_ANALYZER_PROMPT,
        pre_action=build_ts_context,
        output_validator=validate_phase2_node_output,
        enable_feedback=False,
    )
