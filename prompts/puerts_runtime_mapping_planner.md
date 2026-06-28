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
      "implementation_carrier": "template_rendered_ts",
      "selected_runtime_owner": "short runtime owner label",
      "engine_port_mappings": [
        {
          "engine_port_id": "input.action_binding",
          "adjudication_path": "flow/04-ue-api-mcp/adjudication/input.action_binding.json",
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
- runtime_owner must be a generated TypeScript file and later TypeScriptScriptAnalyzer must use it as target_ts_file.
- If any required engine_port is not hit, add a blocked mapping instead of inventing code.
