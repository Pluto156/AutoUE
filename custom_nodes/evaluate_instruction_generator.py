from __future__ import annotations

import json
from pathlib import Path

from core.BaseLLMNode import BaseLLMNode, GraphState
from core.phase2_validation import parse_phase2_json, validate_phase2_node_output, validate_phase2_output

EVALUATE_INSTRUCTION_GENERATOR_PROMPT = """SCHEMA: EvaluateInstructionGenerator
Generate a machine-readable static validation plan for the complete TypeScript/PuerTS chain. Return JSON only.
"""

def GetInput(node: BaseLLMNode, state: GraphState, full_input: str) -> str:
    required = {
        "SceneAndGameplaySplitter": state.llm_outputs.get("SceneAndGameplaySplitter", ""),
        "EntityAbilityBehaviorPlanner": state.llm_outputs.get("EntityAbilityBehaviorPlanner", ""),
        "ThinGameplayFlowPlanner": state.llm_outputs.get("ThinGameplayFlowPlanner", ""),
        "UEApiMCPFeasibilitySearcher": state.llm_outputs.get("UEApiMCPFeasibilitySearcher", ""),
        "PuerTSRuntimeMappingPlanner": state.llm_outputs.get("PuerTSRuntimeMappingPlanner", ""),
        "TypeScriptScriptAnalyzer": state.llm_outputs.get("TypeScriptScriptAnalyzer", ""),
        "TypeScriptInteractiveObjectGenerator": state.llm_outputs.get("TypeScriptInteractiveObjectGenerator", ""),
        "TypeScriptCodeGenerator": state.llm_outputs.get("TypeScriptCodeGenerator", ""),
    }
    missing = [name for name, value in required.items() if not value.strip()]
    if missing:
        raise RuntimeError(f"EvaluateInstructionGenerator missing upstream outputs: {missing}")
    return (
        f"Scene Description:\n{getattr(state, 'scene_description', '')}\n\n"
        f"Gameplay Description:\n{getattr(state, 'gameplay_description', '')}\n\n"
        + "\n\n".join(f"{name} JSON:\n{value}" for name, value in required.items())
        + "\n\nPhase2 emits a static adapter_call plan for the Phase3 Python harness. Use driver=adapter_call only and expected.type=static_trace_present only. Do not claim PIE/runtime pass."
    )

def SaveInstructionjson(state: GraphState, output: str) -> None:
    data = parse_phase2_json("EvaluateInstructionGenerator", validate_phase2_output("EvaluateInstructionGenerator", output))
    save_dir = getattr(state, "save_dir", "")
    if not save_dir:
        raise RuntimeError("state.save_dir is required for instructions.json emission")
    target_dir = Path(save_dir).resolve() / "MyPCG" / "eval"
    target_dir.mkdir(parents=True, exist_ok=True)
    file_path = target_dir / "instructions.json"
    file_path.write_text(json.dumps(data, ensure_ascii=False, indent=4), encoding="utf-8")
    print(f"[SUCCESS] instructions.json saved to: {file_path}")

def create_evaluate_instruction_generator() -> BaseLLMNode:
    return BaseLLMNode(
        name="EvaluateInstructionGenerator",
        prompt=EVALUATE_INSTRUCTION_GENERATOR_PROMPT,
        enable_feedback=False,
        extra_prompt_action=GetInput,
        output_validator=validate_phase2_node_output,
        post_action=SaveInstructionjson,
    )

evaluate_instruction_generator = create_evaluate_instruction_generator()
