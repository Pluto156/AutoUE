# Phase3 complex rerun validation - 2026-06-29

## Result

Phase3 is **done under the revised game-proto-style runtime standard**.

Important wording: this is not an OS-level physical-keyboard claim. The final validation uses a 06D-style application-layer PIE harness: Python adds explicit `AUTOUE_INPUT_*` tags to the generated pawn, and the generated PuerTS runtime consumes those tags inside `ReceiveTick`. That keeps the check inside real UE/PIE/PuerTS runtime while avoiding the editor-window focus problem from the failed 2026-06-28 attempt.

## What changed after the failed 2026-06-28 run

No extra LLM/gameplay nodes were added. The active workflow remains 9 nodes.

The fix was moved into the existing `TypeScriptCodeGenerator` support templates:

- `templates/typescript/aid_runtime_orchestrator.ts.tmpl`
  - application-layer input tags: `AUTOUE_INPUT_RIGHT_1S`, `AUTOUE_INPUT_LEFT`, `AUTOUE_INPUT_ATTACK`, etc.
  - ice trap one-shot trigger plus re-arm radius, so the player does not soft-lock inside the trap.
  - generated freeze VFX component and visible/hidden state.
  - generated state tags: `AUTOUE_GENERATED_PLAYER_FROZEN`, `AUTOUE_GENERATED_ENEMY_DEFEATED`, `AUTOUE_GENERATED_EXIT_UNLOCKED`, `AUTOUE_GENERATED_ROOM_COMPLETE`.
- `templates/typescript/aid_camera_setup.ts.tmpl`
  - fixed side camera to true Y-side view: `sideOffsetY=1650`, `yaw=90`.
  - exported `updateAutoUEGeneratedSideCamera()` so camera follows the pawn every tick and applies shake in world space.
- `templates/typescript/scene_manifest_helper.ts.tmpl`
  - scene manifest v2 includes Player, Enemy, IceTrap, FreezeVFX, SideCamera, Exit and harness tags.
- `tests/test_config_contract.py`
  - deterministic template contract check now covers input harness, trap re-arm, freeze VFX, state snapshot, and side-camera Y-offset implementation.

## Generation evidence

- AutoUE input: `D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\input-phase3-complex-aidev\1.txt`
- AutoUE output root: `D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1`
- Active nodes: 9
  - `SceneAndGameplaySplitter`
  - `EntityAbilityBehaviorPlanner`
  - `ThinGameplayFlowPlanner`
  - `UEApiMCPFeasibilitySearcher`
  - `PuerTSRuntimeMappingPlanner`
  - `TypeScriptScriptAnalyzer`
  - `TypeScriptInteractiveObjectGenerator`
  - `TypeScriptCodeGenerator`
  - `EvaluateInstructionGenerator`
- Phase2/runtime validators:
  - `tools\validate_phase2_outputs.py --root data\output-phase3-complex-aidev\demo_1 --write-report`: PASS
  - `tools\validate_runtime_results.py --root data\output-phase3-complex-aidev\demo_1`: PASS
- Runtime validator instruction count: `15`
- Generated behavior count: `15`
- MCP engine ports include input, movement, overlap, damage, frozen movement, VFX, camera shake and exit overlap ports.

## AIDev staging evidence

- AIDev clean/import backup: `D:\UE5.7.4\AIDev\_codex_backups\20260629-000534-phase3-complex-camera-fixed`
- Staged generated TS:
  - `D:\UE5.7.4\AIDev\TypeScript\AutoUEGeneratedCharacterAdapter.ts`
  - `D:\UE5.7.4\AIDev\TypeScript\AutoUEGeneratedGameModeAdapter.ts`
  - `D:\UE5.7.4\AIDev\TypeScript\content\generated\AutoUEGeneratedRuntime.ts`
  - `D:\UE5.7.4\AIDev\TypeScript\content\generated\AutoUEGeneratedCameraHelper.ts`
  - `D:\UE5.7.4\AIDev\TypeScript\content\generated\AutoUEGeneratedSceneManifest.ts`
- TypeScript compile:
  - `D:\UE5.7.4\AIDev\node_modules\.bin\tsc.cmd -p D:\UE5.7.4\AIDev\tsconfig.json`: PASS
- Generated TypeScript Blueprints:
  - `D:\UE5.7.4\AIDev\Content\Blueprints\TypeScript\AutoUEGeneratedCharacterAdapter.uasset`
  - `D:\UE5.7.4\AIDev\Content\Blueprints\TypeScript\AutoUEGeneratedGameModeAdapter.uasset`
- Config binding:
  - `D:\UE5.7.4\AIDev\Config\DefaultEngine.ini`
  - `GlobalDefaultGameMode=/Game/Blueprints/TypeScript/AutoUEGeneratedGameModeAdapter.AutoUEGeneratedGameModeAdapter_C`

## UE/PIE runtime evidence

Evidence files:

- PIE samples: `D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\test\phase3-complex-runtime-samples-20260629.txt`
- PIE screenshot: `D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\test\phase3-complex-pie-camera-20260629.png`
- AIDev log: `D:\UE5.7.4\AIDev\Saved\Logs\AIDev.log`

Key runtime samples:

| Step | Evidence |
|---|---|
| PIE loaded generated GameMode/Pawn | `gm=/Game/Blueprints/TypeScript/AutoUEGeneratedGameModeAdapter..._C`, `pawn_class=/Game/Blueprints/TypeScript/AutoUEGeneratedCharacterAdapter..._C` |
| Initial scene entities | RoomFloor, Enemy, IceTrap, Exit, FreezeVFX, SideCamera components exist |
| Side camera fixed | initial camera loc `(0.0, -1650.0, 388.15)`, rot `(0.0, 90.0, 0.0)`, ortho `1450.0` |
| Move input changes position | after `AUTOUE_INPUT_RIGHT_1S`, pawn x `0.0 -> 69.82` |
| Trap/freeze/VFX | tags include `AUTOUE_GENERATED_PLAYER_FROZEN`, `AUTOUE_GENERATED_TRAP_SPENT`; `AutoUEGenerated_FreezeVFX visible=True` |
| Thaw | frozen tag removed and `AutoUEGenerated_FreezeVFX visible=False` |
| Move to melee range | pawn x reaches `373.99`, enemy x `360.0` |
| First attack | log contains `EnemyHit enemyHealth=1` |
| Second attack | tags include `AUTOUE_GENERATED_ENEMY_DEFEATED`, `AUTOUE_GENERATED_EXIT_UNLOCKED`; enemy mesh visible `False` |
| Exit complete | pawn x `999.88`, tags include `AUTOUE_GENERATED_ROOM_COMPLETE` |
| Screenshot | saved PNG size `913542` bytes, image size `1280x679`, non-black ratio `1.0` |

Key runtime log lines:

```text
[AUTOUE_GENERATED] CameraReady viewTarget=true sideOffsetY=1650 yaw=90
[AUTOUE_GENERATED] IceTrapTriggered frozen=1 duration=1.25 distance=90 triggerCount=1
[AUTOUE_GENERATED] CameraShakeTriggered=1
[AUTOUE_GENERATED] MoveBlocked reason=frozen
[AUTOUE_GENERATED] IceTrapRearmed distance=138
[AUTOUE_GENERATED] MeleeAttack distance=14 radius=185
[AUTOUE_GENERATED] EnemyHit enemyHealth=1
[AUTOUE_GENERATED] EnemyDefeated=1 ExitUnlocked=1
[AUTOUE_GENERATED] ExitReached=1 RoomComplete=1 distance=161
```

## Non-blocking project/editor warnings

`AIDev.log` still contains unrelated UE/editor startup warnings, including missing optional profiler DLLs, AgentIntegrationKit struct initialization warnings, a water collision profile warning, and an old map dependency warning for `Stage1SideGameMode`. They did not block this run: the generated GameMode/Pawn loaded, generated runtime ticked, state tags changed, and the full gameplay loop completed.

## Conclusion

This run proves the current 9-node AutoUE flow can generate a compact but non-trivial PuerTS/TypeScript system, stage it into AIDev from a clean generated-code baseline, compile it, create TypeScript Blueprints, launch UE Editor/PIE, drive gameplay through a runtime input harness, and verify movement, trap/freeze/VFX, camera shake/side camera, melee defeat, exit unlock, and room completion.
