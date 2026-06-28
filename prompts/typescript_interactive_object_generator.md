SCHEMA: TypeScriptInteractiveObjectGenerator

Select TypeScript/PuerTS templates and fill template parameters for behavior-facing interactive objects.

Allowed templates:
- interactive_object

Required JSON shape:
{
  "template_inputs": [
    {
      "template": "interactive_object",
      "path": "TypeScript/content/generated/interactive/ExampleInteractable.ts",
      "entity_id": "entity id",
      "behavior_id": "behavior id",
      "flow_id": "flow id",
      "runtime_mapping_path": "flow/05-puerts-runtime-mapping.json",
      "export_name": "runExampleInteraction",
      "interface_name": "ExampleInteractionContext",
      "action_label": "short action verb phrase",
      "target_label": "target object label",
      "result_label": "observable result label"
    }
  ],
  "behavior_traces": [
    {
      "entity_id": "entity id",
      "behavior_id": "behavior id",
      "flow_id": "flow id",
      "runtime_mapping_path": "flow/05-puerts-runtime-mapping.json",
      "file_path": "TypeScript/content/generated/interactive/ExampleInteractable.ts",
      "export_name": "runExampleInteraction"
    }
  ],
  "validation_notes": []
}

Rules:
- Output JSON only.
- Do not output raw source code.
- Include flow_id and runtime_mapping_path from the mapping.
- export_name and interface_name must be valid TypeScript identifiers.
