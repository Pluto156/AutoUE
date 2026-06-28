SCHEMA: EvaluateInstructionGenerator

Generate a Phase2 static validation plan for the complete TypeScript/PuerTS chain.

Required JSON shape:
{
  "evaluation_instructions": [
    {
      "step_id": 1,
      "action": "attack",
      "target": "enemy",
      "description": "what to validate",
      "driver": "adapter_call",
      "executor_action": "call_behavior",
      "expected": [{"type": "state_changed", "key": "example", "expected_value": "value"}],
      "trace": {
        "entity_id": "entity id",
        "ability_id": "ability id",
        "behavior_id": "behavior id",
        "flow_id": "flow id",
        "engine_port_ids": ["input.action_binding"],
        "adjudication_paths": ["flow/04-ue-api-mcp/adjudication/input.action_binding.json"],
        "runtime_mapping_path": "flow/05-puerts-runtime-mapping.json",
        "ts_files": ["TypeScript/content/generated/ExampleAbility.ts"]
      }
    }
  ],
  "coverage": [
    {
      "entity_id": "entity id",
      "ability_id": "ability id",
      "behavior_id": "behavior id",
      "flow_id": "flow id",
      "engine_port_ids": ["input.action_binding"],
      "adjudication_paths": ["flow/04-ue-api-mcp/adjudication/input.action_binding.json"],
      "runtime_mapping_path": "flow/05-puerts-runtime-mapping.json",
      "ts_files": ["TypeScript/content/generated/ExampleAbility.ts"]
    }
  ]
}

Rules:
- Output JSON only.
- Phase2 does not launch PIE and does not claim runtime pass/fail.
- Use driver=adapter_call or static_trace_only.
- Every trace must include entity_id, ability_id, behavior_id, flow_id, engine_port_ids, adjudication_paths, runtime_mapping_path, and rendered ts_files.
