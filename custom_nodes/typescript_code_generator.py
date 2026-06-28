from __future__ import annotations

from core.BaseLLMNode import BaseLLMNode, GraphState
from core.phase2_validation import validate_phase2_node_output
from custom_nodes.phase2_file_writer import write_files_from_output

TYPESCRIPT_CODE_GENERATOR_PROMPT = """SCHEMA: TypeScriptCodeGenerator
Select TypeScript/PuerTS ability/runtime templates for every analyzer target. Return JSON only.
"""

def build_generation_context(node: BaseLLMNode, state: GraphState, full_input: str) -> None:
    required = {
        "PuerTSRuntimeMappingPlanner": state.llm_outputs.get("PuerTSRuntimeMappingPlanner", ""),
        "TypeScriptScriptAnalyzer": state.llm_outputs.get("TypeScriptScriptAnalyzer", ""),
        "TypeScriptInteractiveObjectGenerator": state.llm_outputs.get("TypeScriptInteractiveObjectGenerator", ""),
    }
    missing = [name for name, value in required.items() if not value.strip()]
    if missing:
        raise RuntimeError(f"TypeScriptCodeGenerator missing upstream outputs: {missing}")
    node.full_input = "\n\n".join(f"{name} JSON:\n{value}" for name, value in required.items()) + (
        "\n\nFor every implementation slot, emit one template_input whose path equals target_ts_file. "
        "Include flow_id and runtime_mapping_path from the mapping."
    )

def write_generated_ts(state: GraphState, output: str) -> None:
    write_files_from_output(state, "TypeScriptCodeGenerator", output)

def create_typescript_code_generator() -> BaseLLMNode:
    return BaseLLMNode(
        name="TypeScriptCodeGenerator",
        prompt=TYPESCRIPT_CODE_GENERATOR_PROMPT,
        pre_action=build_generation_context,
        output_validator=validate_phase2_node_output,
        post_action=write_generated_ts,
        enable_feedback=False,
    )
