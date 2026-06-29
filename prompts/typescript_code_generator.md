SCHEMA: TypeScriptCodeGenerator

Select TypeScript/PuerTS templates and fill template parameters for runtime/ability files.

Allowed templates:
- ability_module
- runtime_bootstrap
- aid_runtime_orchestrator
- aid_character_adapter
- aid_gamemode_adapter
- aid_camera_setup
- scene_manifest_helper

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
- Additionally emit support template_inputs for a runnable AIDev bridge:
  - aid_runtime_orchestrator at TypeScript/content/generated/AutoUEGeneratedRuntime.ts using export_name runAutoUEGeneratedRuntime
  - aid_character_adapter at TypeScript/AutoUEGeneratedCharacterAdapter.ts using export_name AutoUEGeneratedCharacterAdapter
  - aid_gamemode_adapter at TypeScript/AutoUEGeneratedGameModeAdapter.ts using export_name AutoUEGeneratedGameModeAdapter
  - aid_camera_setup at TypeScript/content/generated/AutoUEGeneratedCameraHelper.ts using export_name setupAutoUEGeneratedCamera
  - scene_manifest_helper at TypeScript/content/generated/AutoUEGeneratedSceneManifest.ts using export_name getAutoUEGeneratedSceneManifest
- Support template_inputs still need entity_id, behavior_id, flow_id, runtime_mapping_path, interface_name, action_label, target_label, and result_label; use the most central gameplay behavior as the trace anchor.
- The AIDev bridge support templates are responsible for runnable runtime orchestration, including application-layer input harness tags (`AUTOUE_INPUT_RIGHT_1S`, `AUTOUE_INPUT_ATTACK`), one-shot ice-trap freeze/rearm behavior, freeze VFX visibility, side-camera setup, camera feedback shake, and snapshot-friendly state tags. Do not create extra LLM nodes for these bridge concerns.
- Include flow_id and runtime_mapping_path from the mapping.
- consumed_interactive_files must reference interactive object generated paths.
