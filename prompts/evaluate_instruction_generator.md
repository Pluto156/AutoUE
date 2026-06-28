SCHEMA: EvaluateInstructionGenerator

Generate a Phase2 static validation plan that the Phase3 Python runtime harness can validate without launching UE/PIE.

Required JSON shape:
{
  "evaluation_instructions": [
    {
      "step_id": 1,
      "action": "attack",
      "target": "enemy",
      "description": "what static adapter_call trace to validate",
      "driver": "adapter_call",
      "executor_action": "call_behavior",
      "expected": [
        {"type": "static_trace_present", "key": "ability_module_export", "expected_value": "runExampleAbility"},
        {"type": "static_trace_present", "key": "interactive_adapter_export", "expected_value": "runExampleInteraction"},
        {"type": "static_trace_present", "key": "engine_ports_mapped", "expected_value": ["input.action_binding"]}
      ],
      "trace": {
        "entity_id": "entity id",
        "ability_id": "ability id",
        "behavior_id": "behavior id",
        "flow_id": "flow id",
        "engine_port_ids": ["input.action_binding"],
        "adjudication_paths": ["flow/04-ue-api-mcp/adjudication/input.action_binding.json"],
        "runtime_mapping_path": "flow/05-puerts-runtime-mapping.json",
        "ts_files": [
          "TypeScript/content/generated/ExampleAbility.ts",
          "TypeScript/content/generated/interactive/ExampleInteractable.ts"
        ]
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
      "ts_files": [
        "TypeScript/content/generated/ExampleAbility.ts",
        "TypeScript/content/generated/interactive/ExampleInteractable.ts"
      ]
    }
  ]
}

Rules:
- Output JSON only.
- Use driver=adapter_call only.
- Use expected.type=static_trace_present only.
- Supported static_trace_present keys are ability_module_export, interactive_adapter_export, and engine_ports_mapped.
- Every trace must include entity_id, ability_id, behavior_id, flow_id, engine_port_ids, adjudication_paths, runtime_mapping_path, and rendered ts_files.
- This is not a PIE gameplay test; do not claim UE Editor launch, PIE launch, player input injection, or real runtime pass/fail.
