SCHEMA: TypeScriptCodeGenerator

Select TypeScript/PuerTS templates and fill template parameters for runtime/ability files.

Allowed templates:
- ability_module
- runtime_bootstrap

Required JSON shape:
{
  "template_inputs": [
    {
      "template": "ability_module",
      "path": "TypeScript/content/generated/ExampleAbility.ts",
      "entity_id": "entity id",
      "behavior_id": "behavior id",
      "flow_id": "flow id",
      "runtime_mapping_path": "flow/05-puerts-runtime-mapping.json",
      "export_name": "runExampleAbility",
      "interface_name": "ExampleAbilityContext",
      "action_label": "short action phrase",
      "target_label": "target label",
      "result_label": "result label"
    }
  ],
  "behavior_traces": [
    {
      "entity_id": "entity id",
      "behavior_id": "behavior id",
      "flow_id": "flow id",
      "runtime_mapping_path": "flow/05-puerts-runtime-mapping.json",
      "file_path": "TypeScript/content/generated/ExampleAbility.ts",
      "export_name": "runExampleAbility"
    }
  ],
  "consumed_interactive_files": ["TypeScript/content/generated/interactive/ExampleInteractable.ts"],
  "validation_notes": []
}

Rules:
- Output JSON only.
- Do not output raw source code.
- For every analyzer implementation slot, emit one template_input whose path equals target_ts_file.
- Include flow_id and runtime_mapping_path from the mapping.
- consumed_interactive_files must reference interactive object generated paths.
