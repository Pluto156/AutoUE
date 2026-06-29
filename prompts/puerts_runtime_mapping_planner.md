SCHEMA: PuerTSRuntimeMappingPlanner

Map thin gameplay flows and UE API MCP adjudications into concrete PuerTS runtime carriers.

Required JSON shape:
{
  "runtime_mapping_path": "flow/05-puerts-runtime-mapping.json",
  "mappings": [
    {
      "entity_id": "entity id",
      "ability_id": "ability id",
      "behavior_id": "behavior id",
      "flow_id": "flow id",
      "runtime_owner": "TypeScript/content/generated/ExampleAbility.ts",
      "implementation_carrier": "template_rendered_ts|generated_aidev_adapter|generated_runtime_orchestrator|existing_runtime_adapter|blocked",
      "selected_runtime_owner": "short runtime owner label",
      "existing_framework_candidates": ["candidate existing helper/adapter/runtime, or empty if none"],
      "why_not_existing_framework": "why generation/extension is needed, or why existing runtime is sufficient",
      "temporary_or_canonical": "temporary|canonical",
      "migration_path": "how this carrier should migrate into canonical runtime later",
      "engine_port_mappings": [
        {
          "engine_port_id": "input.action_binding",
          "adjudication_path": "flow/04-ue-api-mcp/adjudication/input.action_binding.json",
          "adapter_or_helper": "CharacterAdapter.bindInput or RuntimePorts.readInput",
          "verdict": "hit",
          "evidence_symbols": ["UE.Symbol"]
        }
      ],
      "thin_contracts": ["contract summary"],
      "ability_binding": "how this ability should be invoked from PuerTS",
      "verification_evidence": ["what later validation should check"]
    }
  ],
  "blocked_mappings": []
}

Rules:
- Output JSON only.
- Create one mapping per behavior.
- runtime_mapping_path must be exactly flow/05-puerts-runtime-mapping.json.
- runtime_owner should be the behavior-level generated TypeScript ability file under TypeScript/content/generated/.
- Do not silently put input binding, camera, scene hookup, combat, status effects, and exit logic into an unspecified handwritten runtime. Record adapter_or_helper and implementation_carrier explicitly.
- If any required engine_port is not hit, add a blocked mapping instead of inventing code.
