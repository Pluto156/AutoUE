---
doc_type: phase_card
task: autoue-puerts-workflow-adaptation
phase: 3
status: done
last_updated: 2026-06-29
---

# Phase3 · 真实流程运行闭环

## 当前裁决

Phase3 已完成，最终证据以 2026-06-29 的复杂系统重跑为准：`test/Phase3-complex-rerun-20260629.md`。

完成口径不是“手写中间层能跑”，也不是“物理键盘 Win32 注入成功”。本轮保留 9 个 active 节点，把 runtime adapter / combat / camera / scene hookup 能力收回到 `TypeScriptCodeGenerator` 的确定性支撑模板中，并在真实 UE Editor / PIE / PuerTS runtime 里用 application-layer input harness 跑通。

## Phase3 done 标准

Phase3 必须真实跑一遍流程后才能标 done。最低完成口径：

```text
Phase2 输出产物
→ staged 到真实 UE/PuerTS 可运行位置
→ 启动或连接 UE Editor/PIE
→ 在真实运行时加载/调用生成的 TS/PuerTS 逻辑
→ 执行至少一条完整玩法流程
→ 收集 runtime 证据
→ validate_runtime_results.py 或等价 gate 判定 pass/fail
```

“真实跑一遍流程”至少要覆盖一条 demo 流程，例如：

```text
player move / attack / collect / exit 中至少一条 end-to-end 流程
```

最终证据不能只来自静态文件扫描；必须有真实运行时产生的日志、summary、截图/录屏或等价可复现证据。


## 最终完成证据：复杂系统节点生成 + AIDev/PIE 真实运行

本轮 run 见：`test/Phase3-complex-rerun-20260629.md`。

关键证据：

- AutoUE 输出：`D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1`
- AIDev 清空/导入备份：`D:\UE5.7.4\AIDev\_codex_backups\20260629-000534-phase3-complex-camera-fixed`
- runtime samples：`test/phase3-complex-runtime-samples-20260629.txt`
- screenshot：`test/phase3-complex-pie-camera-20260629.png`

通过项：

| 验收项 | 结果 | 证据 |
|---|---|---|
| AutoUE 9 active nodes 从零复杂重跑 | PASS | `data/output-phase3-complex-aidev/demo_1/llm_outputs/*.txt` |
| Phase2 validator | PASS | `validate_phase2_outputs.py --write-report`，15 behaviors / 15 instructions |
| runtime validator | PASS | `validate_runtime_results.py`，`instruction_count=15` |
| AIDev generated-code baseline 清理 | PASS | backup `20260629-000534-phase3-complex-camera-fixed` |
| AIDev tsc | PASS | `tsc.cmd -p D:\UE5.7.4\AIDev\tsconfig.json` |
| TypeScript Blueprint | PASS | `AutoUEGeneratedCharacterAdapter.uasset` / `AutoUEGeneratedGameModeAdapter.uasset` |
| UE Editor / PIE | PASS | generated GameMode/Pawn loaded in PIE |
| movement harness | PASS | pawn x `0.0 -> 69.82` |
| ice trap / freeze / VFX | PASS | frozen/trap tags + FreezeVFX visible true then false |
| side camera / camera shake | PASS | camera loc `(x, -1650, 388)`, yaw `90`, `CameraShakeTriggered=1` |
| melee combat | PASS | `EnemyHit enemyHealth=1` then `EnemyDefeated=1 ExitUnlocked=1` |
| exit completion | PASS | `AUTOUE_GENERATED_ROOM_COMPLETE` and `ExitReached=1 RoomComplete=1` |
| screenshot | PASS | `phase3-complex-pie-camera-20260629.png`, non-black ratio `1.0` |

限制说明：物理键盘输入仍不是本 Phase 的完成声明；本 Phase 采用 game-proto-style application-layer harness 作为真实 UE runtime 验证入口。


## 历史证据：手写中间层真实 AIDev 集成证明（已被最终节点生成证据取代）

本轮 run id：`20260628-181719-autoue-phase3-real`。

证据目录：`D:\UE5.7.4\AIDev\_codex_backups\20260628-181719-autoue-phase3-real`。

核心 summary：

- `D:\UE5.7.4\AIDev\_codex_backups\20260628-181719-autoue-phase3-real\phase3-real-aidev-summary.md`
- `D:\UE5.7.4\AIDev\_codex_backups\20260628-181719-autoue-phase3-real\phase3-real-aidev-summary.json`

真实流程：

```text
AIDev 清到无旧 Phase4/5/6 TS/JS/TS Blueprint baseline
→ AutoUE 9 个 active LLM 节点真实生成
→ Phase2 validator pass
→ runtime validator pass
→ 产物 staged 到 D:\UE5.7.4\AIDev\TypeScript\content\autoue_phase3_real
→ tsc 编译 pass
→ TypeScript Blueprint 生成/绑定
→ UE Editor 启动
→ PIE 启动
→ 真实键盘输入 D / J / D
→ player move / enemy defeated / exit reached / room complete
```

关键验收表：

| 验收项 | 结果 | 证据 |
|---|---|---|
| baseline 无旧 TS/JS/TS Blueprint | PASS | summary 中 `old_phase_artifacts_count=0` |
| AutoUE 9 active nodes 真跑 | PASS | `phase2-validator-result.json` validated nodes = 9 |
| runtime validator | PASS | `runtime-validator-result.json` |
| AIDev TS 编译 | PASS | `tsc.cmd -p D:\UE5.7.4\AIDev\tsconfig.json` |
| TypeScript Blueprint | PASS | `Content\Blueprints\TypeScript\AutoUEPhase3CharacterAdapter.uasset`、`AutoUEPhase3GameModeAdapter.uasset` |
| GameMode 绑定 | PASS | `/Game/Blueprints/TypeScript/AutoUEPhase3GameModeAdapter.AutoUEPhase3GameModeAdapter_C` |
| PIE GameMode/Pawn | PASS | `fresh-pie-samples-after-tag-fix\02-initial-sample.json` |
| 真实移动输入 | PASS | `D` 800ms 后 pawn x: `0 -> 501.747` |
| 真实攻击输入 | PASS | `J` 150ms 后 tags=`AUTOUE_PHASE3_ENEMY_DEFEATED`，enemy component hidden |
| exit/room complete | PASS | `D` 600ms 后 distance_to_exit=`119.728`，tags 包含 `AUTOUE_PHASE3_EXIT_REACHED` 与 `AUTOUE_PHASE3_ROOM_COMPLETE` |
| runtime 日志 | PASS | `autoue-phase3-key-runtime-log.txt` 包含 `MoveInput`、`EnemyDefeated=1`、`ExitReached=1`、`RoomComplete=1` |
| 截图 | PASS | `fresh-pie-samples-after-tag-fix\06-screenshot-result.json` |

注意：计划中的 `D:\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe` 与当前插件 BuildId 不匹配，本轮使用已验证兼容的 `D:\UE-src-5.7\Engine\Binaries\Win64\UnrealEditor.exe`。

## 已完成子项：Python harness

已完成但只作为前置工具：

- `repo/AutoUE/core/runtime_validation.py`
- `tools/run_runtime_validation.py --root <demo> --write-summary`
- `tools/validate_runtime_results.py --root <demo>`
- `autogenerate_qwen.py --run-runtime-validation`
- `scripted_smoke` 与 `data/output-smoke/demo_1` 已改成 `adapter_call + static_trace_present` 合同。
- 单测覆盖 good path、缺 `instructions.json`、缺 TS 文件、缺 MCP adjudication、缺 runtime mapping、unknown driver、unknown expected type、runner fail 返回非零。

这些结果保留为 Phase3 的 runner/harness 基础，但不能再写成 Phase3 done。

## 已验证但不够 done 的命令

工作目录：`D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE`

| 验收项 | 结果 | Phase3 口径 |
|---|---|---|
| `autogenerate_qwen.py --dry-run-config --workflow config\workflows\puerts_ts.json` | PASS | 只证明配置摘要 |
| `tools\run_runtime_validation.py --root data\output-smoke\demo_1 --write-summary` | PASS | 只证明静态 harness |
| `tools\run_runtime_validation.py --root data\output-real-008\demo_1 --write-summary` | PASS | 只证明静态 harness |
| `autogenerate_qwen.py --workflow config\workflows\puerts_ts.json --llm-profile scripted_smoke --output-dir data\output-phase3-smoke --run-runtime-validation` | PASS | 只证明 runner 集成静态 harness |
| `pytest tests -q` | `25 passed` | 回归证据 |
| `py_compile` | PASS | 静态编译证据 |

## 已完成前的缺口记录

- 已完成：generated TS/PuerTS staged 到 `D:\UE5.7.4\AIDev\TypeScript\content\autoue_phase3_real`。
- 已完成：UE Editor/PIE 通过 `D:\UE-src-5.7\Engine\Binaries\Win64\UnrealEditor.exe` 启动并采样。
- 已完成：真实 UE/PuerTS runtime 加载 `AutoUEPhase3CharacterAdapter_C` / `AutoUEPhase3GameModeAdapter_C`。
- 已完成：真实 runtime summary/log/screenshot 已写入证据目录。
- 存在范围降级：`AutoUEPhase3Runtime.ts`、Character/GameMode Adapter、战斗状态、摄像机和 UE 接入层由主 agent 手写；因此不能标 Phase3 done。

## 不接受的 done 口径

- 只跑 Python `runtime_validation.py`。
- 只跑 `validate_runtime_results.py` 且 summary 来源仍是静态 harness。
- 只看 generated `.ts` 文件是否存在。
- 只跑 scripted smoke。
- 只跑 `output-real-008` 的静态 trace。
- 只说“后续可以启动 PIE”。

## 下一步

Phase3 未完成。下一步必须消除手写中间层，把 AIDev runtime adapter / GameMode adapter / 场景接入代码纳入 AutoUE 节点或确定性 runner 产物：

1. 新增或扩展节点，让 AutoUE 输出 `AutoUEPhase3Runtime.ts`、Character Adapter、GameMode Adapter 或等价 runtime 接入代码。
2. 从 clean baseline 重跑 AutoUE，只允许复制/编译节点产物，不允许主 agent 手写 gameplay runtime。
3. 重新生成 Blueprint、绑定 GameMode、启动 UE/PIE，并用真实输入验证移动/攻击/exit。
4. 只有这条链路通过，Phase3 才能重新标 done。


## 修正记录

- 手写中间层回退说明：`D:\UE5.7.4\AIDev\_codex_backups\20260628-181719-autoue-phase3-real\phase3-manual-bridge-retraction.md`

## 2026-06-28 Phase3 重跑方案更新：少节点、强契约、复杂系统

### 方案裁决

不采用“新增多个 Runtime/Combat/Camera/SceneHookup LLM 节点”的方案 B。Phase3 后续按下面方式推进：

```text
保留 9 个 active 节点
扩展现有节点 schema / prompt / templates
把 UE/PIE 执行与 gate 放到 deterministic runner
不允许主 agent 手写 gameplay runtime 后冒充节点产物
```

### 必须补进现有节点的能力

1. `EntityAbilityBehaviorPlanner`
   - 分析顺序改成 `行为 → 能力 → 实体`。
   - 机器结构仍输出 `entities[] → abilities[] → behaviors[]`。
   - 新增正式产物：`flow/02-structure.json`。

2. `PuerTSRuntimeMappingPlanner`
   - 每个 mapping 必须包含：

```text
implementation_carrier
existing_framework_candidates
why_not_existing_framework
temporary_or_canonical
migration_path
engine_port_mappings[].adapter_or_helper
verification_evidence
```

3. `TypeScriptScriptAnalyzer` / `TypeScriptCodeGenerator`
   - 不再只生成 ability 文件。
   - 必须能按第 5 步 mapping 生成：

```text
ability module
interactive object / trigger helper
AIDev runtime orchestrator
CharacterAdapter
GameModeAdapter
camera setup / view target helper
scene manifest helper
```

4. Deterministic runner / validator
   - 负责 clean baseline、复制产物、tsc、蓝图生成、GameMode 绑定、UE/PIE、真实输入、采样与 gate。
   - 输出至少包括：

```text
06C-manifest.json
06D-scene-probe-strict.json
08-runtime-summary.json
flow-gate.json
flow-check-report.md
```

### Clean baseline 后的复杂系统重跑输入

```text
Create a compact side-scroller combat room. The player can move left and right, perform one short melee attack, and briefly dash. There is one patrolling melee enemy with health and defeated state. Add one ice trap in the room: when the player overlaps it, the player becomes frozen for a short time, movement is blocked, a freeze VFX appears, and the side camera gives a small feedback shake. The exit is locked until the enemy is defeated, then reaching the exit completes the room. Keep it one room and do not add inventory, quests, upgrades, ranged weapons, or complex UI.
```

### 重跑前必须清理

从 AIDev clean baseline 开始，至少清理以下旧产物并备份：

```text
D:\UE5.7.4\AIDev\TypeScript\content\autoue_phase3_real
D:\UE5.7.4\AIDev\TypeScript\AutoUEPhase3*.ts
D:\UE5.7.4\AIDev\Content\JavaScript\content\autoue_phase3_real
D:\UE5.7.4\AIDev\Content\JavaScript\AutoUEPhase3*
D:\UE5.7.4\AIDev\Content\Blueprints\TypeScript\AutoUEPhase3*
```

并把 `DefaultEngine.ini` 重置到无旧 TS GameMode 绑定后再运行生成链。

### 新的 done 判定

只有下面全部成立，Phase3 才能重新标 done：

- 复杂系统 prompt 从 clean baseline 重新生成。
- 节点输出包含 `02/03/04/05` 可追溯证据。
- adapter/runtime/camera/scene hookup 由节点模板或 deterministic runner 产出，不由主 agent 手写。
- `tsc` 通过。
- TypeScript Blueprint 生成成功。
- GameMode 绑定到新生成的 GameModeAdapter。
- PIE 中真实输入能触发：移动、冲刺或移动受限、近战、敌人 defeated、冰冻状态、解冻、出口完成。
- 摄像机 ViewTarget 与玩家/关键 actor 可见性有 `06D` 或等价严格 probe 证据。
- `flow-gate.json.process_verdict == process-pass` 且 `allowed_next_step == done`。

若任一项失败，Phase3 保持 `implementing`，并按失败所在步骤回退，不允许降级成 done。
