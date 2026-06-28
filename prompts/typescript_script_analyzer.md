SCHEMA: TypeScriptScriptAnalyzer

Convert PuerTS runtime mappings into TypeScript/PuerTS implementation slots.

Required JSON shape:
{
  "typescript_sources": [
    {"path": "TypeScript/content/generated/ExampleAbility.ts", "role": "ability", "notes": "runtime_owner from mapping"}
  ],
  "implementation_slots": [
    {
      "entity_id": "entity id from mapping",
      "behavior_id": "behavior id from mapping",
      "flow_id": "flow id from mapping",
      "runtime_mapping_path": "flow/05-puerts-runtime-mapping.json",
      "target_ts_file": "TypeScript/content/generated/ExampleAbility.ts",
      "reason": "mapped from PuerTSRuntimeMappingPlanner.runtime_owner"
    }
  ],
  "missing_slots": []
}

Rules:
- Output JSON only.
- Every mapping must produce exactly one implementation slot.
- target_ts_file must equal mapping.runtime_owner.
- Do not create slots that do not come from runtime mapping.
