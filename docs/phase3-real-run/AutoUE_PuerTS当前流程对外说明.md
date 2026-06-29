# AutoUE / PuerTS 当前流程对外说明

> 这是一份对外解释版文档。
>
> 目标不是讲内部修复历史，而是把当前这套流程说清楚：用户给一个玩法需求后，AutoUE 每个节点做什么、输出什么、怎么检查，最后怎么落到 AIDev 的 PuerTS / TypeScript 运行里。
>
> 本轮场景口径：**复用 AIDev 现有 ThirdPerson 基础关卡，不新建 umap。** 玩法实体、战斗逻辑、冰冻陷阱、摄像头反馈等运行内容由 TypeScript / Blueprint 接入到这个已有关卡里。

---

# 当前流程

```Markdown
1. 用户输入需求
   ↓
2. 拆场景和玩法：先分清“看起来是什么”和“要玩什么”
   ↓
3. 拆实体、能力、行为：把需求拆成玩家、敌人、陷阱、出口、摄像头等对象
   ↓
4. 拆薄玩法流：把每个行为拆成输入、判断、状态变化、反馈、清理
   ↓
5. MCP 查询 UE / PuerTS API 可实现性：只查需要的 engine port，不问大而泛的玩法问题
   ↓
6. PuerTS 运行映射：说明每个行为在当前项目里由哪个 TS 文件、adapter、runtime helper 承接
   ↓
7. 生成 TypeScript 代码：生成能力文件、交互文件、运行时公共层、GameMode / Character 适配层
   ↓
8. 生成验证指令：把“应该检查什么”也写成机器可读的 instructions
   ↓
9. 导入 AIDev，编译 TS，生成 TypeScript Blueprint，启动 UE / PIE 真跑验证
```

一句话：

> 前半段负责“把需求拆清楚”，中段负责“确认 UE/PuerTS 能不能做”，后半段负责“生成代码并在 AIDev 里真跑”。

---

# 问题

## 1. 需求太自然语言，直接写代码会发散

**问题：**

用户一句“做一个横版战斗房间，加一个冰冻陷阱”，里面其实混着很多东西：角色、敌人、陷阱、出口、摄像头、冻结状态、战斗状态、胜利条件。

如果直接让代码节点写，很容易变成一个大脚本：输入、敌人、陷阱、开门、摄像头全塞在一个函数里。

**当前做法：**

先固定拆成三层：

```Plain
实体 Entity：游戏里有什么东西
能力 Ability：这个东西能做什么
行为 Behavior：这个能力具体发生的一件事
```

例如：

```Plain
玩家 player
  水平移动能力 player.horizontal_movement
    - 左右移动 player.horizontal_movement.move_left_right
  近战攻击能力 player.short_melee_attack
    - 打中敌人 player.short_melee_attack.strike_enemy
  冰冻状态 player.frozen_state
    - 进入冰冻 player.frozen_state.become_frozen
    - 解冻 player.frozen_state.thaw

冰冻陷阱 ice_trap
  冻结触发能力 ice_trap.freeze_trigger
    - 玩家踩到后冻结 ice_trap.freeze_trigger.freeze_overlapping_player
```

这样后面的 MCP 查询、代码生成、验证都能对着同一个 ID 走，不靠口头理解。

---

## 2. MCP 查询容易问偏

**问题：**

不能问 MCP：

```Plain
怎么做一个冰冻陷阱？
怎么做一个横版战斗系统？
```

这种问题太大，MCP 返回的往往是相关 API，不一定是当前要用的 API。

**当前做法：**

只查 engine port，也就是“引擎能力口”。

例如冰冻陷阱不是问“冰冻陷阱怎么做”，而是拆成：

```Plain
检测玩家是否进入范围 -> primitive.on_component_begin_overlap / overlap query
冻结玩家移动       -> character_movement.disable_movement
一段时间后恢复     -> timer.set_timer / set_movement_mode
显示冰冻表现       -> niagara.spawn_system_attached
摄像头震动         -> player_controller.client_start_camera_shake
```

这样 MCP 的责任很窄：只证明 UE / PuerTS 侧有没有可用 API 候选。

---

## 3. 代码不能再变成一个“大脚本”

**问题：**

如果一个函数里同时做移动、攻击、敌人受伤、陷阱冻结、开门、摄像头表现，短期能跑，长期一定难维护。

**当前做法：**

使用轻量公共层，不搞大框架，但把职责分开：

```Plain
能力文件：每个行为一个小模块，只处理自己这件事
交互文件：把行为包装成可调用入口
AutoUEGeneratedRuntime.ts：放共享状态和运行时调度
AutoUEGeneratedCameraHelper.ts：放摄像头相关辅助逻辑
AutoUEGeneratedSceneManifest.ts：放生成场景需要的实体/位置/标签配置
AutoUEGeneratedCharacterAdapter.ts：把玩家 Pawn 和输入/行为接起来
AutoUEGeneratedGameModeAdapter.ts：负责初始化房间、敌人、陷阱、出口和总流程
```

这不是引入复杂框架，而是把“谁负责什么”拆清楚。

---

## 4. 场景要说清楚是“创建”还是“复用”

**问题：**

如果不明确，很容易误以为每次都生成一个新关卡。

**当前做法：**

本轮明确是：

```Plain
复用已有关卡：/Game/ThirdPerson/Lvl_ThirdPerson.Lvl_ThirdPerson
不新建新的 .umap
不把旧玩法场景当成新生成场景
```

AutoUE 生成的是运行时玩法内容：玩家适配、敌人、陷阱、出口、摄像头反馈和状态逻辑。它们通过 TypeScript Blueprint / GameMode 在现有关卡启动时接入。

---

## 5. 验证不能只看“文件生成了”

**问题：**

只生成文件不代表能玩。只编译通过也不代表战斗闭环真的发生。

**当前做法：**

分三层检查：

```Plain
静态检查：节点输出文件、JSON 结构、MCP 命中、映射完整
编译检查：TypeScript 编译通过，Blueprint 生成成功
运行检查：UE Editor / PIE 真启动，玩家移动、攻击、冻结、解冻、开门、到达出口都有运行状态和日志
```

最终验收看的是“闭环有没有跑通”，不是只看某个节点有没有输出文本。

---

# 解决方案

当前方案可以理解成一条“从自然语言到 UE 可运行玩法”的流水线。

核心原则是：

```Plain
自然语言需求
  先变成稳定结构
  再变成薄玩法流
  再查 UE/PuerTS 能力
  再映射到当前项目运行结构
  再生成代码
  最后用真实运行结果检查
```

它不是让一个大模型一次性把所有代码写完，而是把工作拆给 9 个节点，每个节点有自己的输入、输出和检查点。

当前启用的 9 个节点是：

```Plain
1. SceneAndGameplaySplitter
2. EntityAbilityBehaviorPlanner
3. ThinGameplayFlowPlanner
4. UEApiMCPFeasibilitySearcher
5. PuerTSRuntimeMappingPlanner
6. TypeScriptScriptAnalyzer
7. TypeScriptInteractiveObjectGenerator
8. TypeScriptCodeGenerator
9. EvaluateInstructionGenerator
```

被禁用的旧方向包括：

```Plain
RetrieveModel
PCGGraphComposer
PCGPlanner
ModuleCodeGenerator
InteractiveObjectCodeGenerator
```

也就是说，当前这条线不走模型检索、不走 PCG 图生成、不走旧 C++/旧交互对象代码生成；当前重点是 **PuerTS / TypeScript 玩法生成和 AIDev 接入验证**。

---

# 案例：带冰冻陷阱的横版战斗房间

本轮用于贯穿流程的需求可以概括成：

```Plain
做一个很小的横版战斗房间。
玩家可以左右移动，可以短距离冲刺，可以用一次近战攻击击败一个近战敌人。
房间里有一个冰冻陷阱，玩家踩到后会短暂冻结，出现冰冻表现，并触发摄像头反馈。
击败敌人后出口解锁，玩家到达出口后房间完成。
不要背包、任务、升级、远程武器、复杂 UI、多房间。
```

这个案例覆盖了几个关键问题：

```Plain
基础移动
近战攻击
敌人受伤/死亡
陷阱触发
玩家冻结/解冻
VFX 反馈
摄像头反馈
出口锁定/解锁
胜利条件
```

下面按节点说清楚。

---

## 01. SceneAndGameplaySplitter：先分清“场景”和“玩法”

**它做什么：**

把用户需求拆成两类：

```Plain
场景：一个小房间、横版视角、一个敌人、一个冰冻陷阱、一个出口
玩法：玩家移动、攻击、踩陷阱冻结、击败敌人、出口解锁、到达出口完成
```

**为什么要有这一步：**

因为“房间里有什么”和“这些东西怎么互动”不能混在一起。混在一起后，后面生成代码会很容易不知道是在摆场景，还是在写玩法状态机。

**主要输出：**

```Plain
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\llm_outputs\SceneAndGameplaySplitter.txt
```

**检查方式：**

看它是否把场景要素和玩法要素拆开，而不是直接写实现代码。

---

## 02. EntityAbilityBehaviorPlanner：拆实体、能力、行为

**它做什么：**

把玩法拆成稳定结构：游戏里有哪些实体，每个实体有什么能力，每个能力有哪些具体行为。

**本轮实际拆出来：**

```Plain
1. player
   - player.horizontal_movement
     - player.horizontal_movement.move_left_right
   - player.short_melee_attack
     - player.short_melee_attack.strike_enemy
   - player.brief_dash
     - player.brief_dash.dash_horizontally
   - player.frozen_state
     - player.frozen_state.become_frozen
     - player.frozen_state.thaw

2. patrolling_melee_enemy
   - patrolling_melee_enemy.patrol
     - patrolling_melee_enemy.patrol.move_back_and_forth
   - patrolling_melee_enemy.melee_pressure
     - patrolling_melee_enemy.melee_pressure.threaten_close_player
   - patrolling_melee_enemy.health
     - patrolling_melee_enemy.health.take_melee_damage
     - patrolling_melee_enemy.health.become_defeated

3. ice_trap
   - ice_trap.freeze_trigger
     - ice_trap.freeze_trigger.freeze_overlapping_player

4. freeze_vfx
   - freeze_vfx.freeze_feedback
     - freeze_vfx.freeze_feedback.appear_on_freeze

5. side_camera
   - side_camera.freeze_feedback_shake
     - side_camera.freeze_feedback_shake.shake_on_freeze

6. locked_exit
   - locked_exit.enemy_gate
     - locked_exit.enemy_gate.remain_locked
     - locked_exit.enemy_gate.unlock_after_enemy_defeat
   - locked_exit.room_completion
     - locked_exit.room_completion.complete_on_reach
```

**数量：**

```Plain
实体：6 个
能力：12 个
行为：15 个
```

**主要输出：**

```Plain
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\llm_outputs\EntityAbilityBehaviorPlanner.txt
```

**检查方式：**

检查点不是“写得像不像”，而是：

```Plain
每个行为是否有稳定 ID
是否覆盖用户需求里的关键交互
是否明确排除了不需要的系统，比如背包、任务、升级、远程武器、复杂 UI
```

---

## 03. ThinGameplayFlowPlanner：把行为拆成薄玩法流

**它做什么：**

把每个行为变成一个很薄的执行合同。

比如“玩家踩到冰冻陷阱后冻结”不直接写代码，而是先拆成：

```Plain
Input：检测玩家是否和陷阱重叠
Ability/Action：给玩家设置 frozen 状态
State Change：玩家暂时不能移动/冲刺/攻击
Feedback：显示冰冻 VFX，触发摄像头反馈
Cleanup：计时结束后解除 frozen 状态，恢复移动
```

**本轮结果：**

```Plain
15 个 behavior -> 15 个 flow
```

**主要输出：**

```Plain
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\flow\03-thin-gameplay-flow.json
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\llm_outputs\ThinGameplayFlowPlanner.txt
```

**检查方式：**

检查每个 behavior 是否都有对应 flow，例如：

```Plain
player.horizontal_movement.move_left_right -> flow.player.horizontal_movement.move_left_right
player.short_melee_attack.strike_enemy -> flow.player.short_melee_attack.strike_enemy
ice_trap.freeze_trigger.freeze_overlapping_player -> flow.ice_trap.freeze_trigger.freeze_overlapping_player
side_camera.freeze_feedback_shake.shake_on_freeze -> flow.side_camera.freeze_feedback_shake.shake_on_freeze
locked_exit.room_completion.complete_on_reach -> flow.locked_exit.room_completion.complete_on_reach
```

---

## 04. UEApiMCPFeasibilitySearcher：只查 UE / PuerTS API 能力口

**它做什么：**

读取第 03 步里的 flow，把里面需要引擎支持的能力抽成 engine port，然后用 MCP 查询 UE / PuerTS API 候选。

**本轮查了什么：**

```Plain
enhanced_input.bind_action -> hit，direct_hit
pawn.add_movement_input -> hit，indirect_hit
kismet.sphere_trace_single -> hit，direct_hit
gameplay_statics.apply_damage -> hit，direct_hit
character.launch_character -> hit，indirect_hit
character_movement.disable_movement -> hit，indirect_hit
timer.set_timer -> hit，indirect_hit
character_movement.set_movement_mode -> hit，indirect_hit
actor.set_actor_location -> hit，direct_hit
primitive.on_component_begin_overlap -> hit，indirect_hit
actor.on_take_any_damage -> hit，indirect_hit
actor.set_actor_tick_enabled -> hit，direct_hit
niagara.spawn_system_attached -> hit，direct_hit
player_controller.client_start_camera_shake -> hit，indirect_hit
```

**结果：**

```Plain
查询项：14 个
全部命中：是
阻塞项：0 个
```

**主要输出：**

```Plain
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\flow\04-ue-api-mcp\raw\*.raw.json
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\flow\04-ue-api-mcp\adjudication\*.json
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\flow\04-ue-api-mcp\summary.json
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\llm_outputs\UEApiMCPFeasibilitySearcher.txt
```

**检查方式：**

```Plain
每个 engine port 是否有 raw 查询记录
每个 engine port 是否有 adjudication 判断
summary.json 里 blocked_engine_ports 是否为空
不能用“玩法描述”替代 engine port 查询
```

---

## 05. PuerTSRuntimeMappingPlanner：把玩法流映射到当前项目运行结构

**它做什么：**

回答一个更落地的问题：

```Plain
这些 flow 在 AIDev 里到底由谁承接？
是 CharacterAdapter？GameModeAdapter？Runtime helper？Camera helper？还是单独能力文件？
```

例如：

```Plain
玩家左右移动
  -> CharacterAdapter 接输入
  -> 调用 PlayerHorizontalMovementMoveLeftRightAbility.ts
  -> Runtime 记录位置变化和输入日志

玩家近战攻击
  -> CharacterAdapter 接攻击输入
  -> PlayerShortMeleeAttackStrikeEnemyAbility.ts 处理命中
  -> Runtime 改 enemyHealth / EnemyDefeated / ExitUnlocked

冰冻陷阱
  -> GameModeAdapter 初始化陷阱实体
  -> IceTrapFreezeTriggerFreezeOverlappingPlayerAbility.ts 处理触发
  -> Runtime 设置 playerFrozen
  -> FreezeVFX 和 CameraHelper 负责表现反馈

出口
  -> LockedExitEnemyGateUnlockAfterEnemyDefeatAbility.ts 处理解锁
  -> LockedExitRoomCompletionCompleteOnReachAbility.ts 处理到达出口完成
```

**本轮结果：**

```Plain
映射数量：15 个
blocked：0 个
implementation_carrier：template_rendered_ts
```

**主要输出：**

```Plain
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\flow\05-puerts-runtime-mapping.json
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\llm_outputs\PuerTSRuntimeMappingPlanner.txt
```

**检查方式：**

```Plain
15 个 flow 是否都有 runtime mapping
是否明确 implementation_carrier
是否明确 adapter/helper/ability 谁负责
是否存在 blocked 或 fallback
```

---

## 06. TypeScriptScriptAnalyzer：把映射变成 TS 生成计划

**它做什么：**

把第 05 步的 runtime mapping 再翻译成代码生成节点能用的计划。

人话说，它负责把“应该接在哪里”变成“应该生成哪些 TS 模块、导出哪些函数、需要哪些调用痕迹”。

**主要输出：**

```Plain
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\llm_outputs\TypeScriptScriptAnalyzer.txt
```

**检查方式：**

看生成计划是否覆盖：

```Plain
ability module export
interactive adapter export
engine ports mapped
runtime mapping path
expected TS files
```

---

## 07. TypeScriptInteractiveObjectGenerator：生成交互入口文件

**它做什么：**

给每个行为生成一个可被验证和调用的交互包装文件。

这些文件不应该承载全部游戏逻辑，它更像“接口层”：外部要验证某个行为时，可以通过这个入口调用对应能力。

**主要输出：**

```Plain
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\TypeScript\content\generated\interactive\*.ts
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\llm_outputs\TypeScriptInteractiveObjectGenerator.txt
```

**示例文件：**

```Plain
PlayerHorizontalMovementMoveLeftRightInteractable.ts
PlayerShortMeleeAttackStrikeEnemyInteractable.ts
IceTrapFreezeTriggerFreezeOverlappingPlayerInteractable.ts
SideCameraFreezeFeedbackShakeShakeOnFreezeInteractable.ts
LockedExitRoomCompletionCompleteOnReachInteractable.ts
```

**检查方式：**

```Plain
每个 behavior 是否都有 interactive 文件
interactive 文件是否调用对应 ability export
是否保留 trace，能回查到 behavior_id / flow_id / engine_ports
```

---

## 08. TypeScriptCodeGenerator：生成实际 TypeScript 实现代码

**它做什么：**

这是当前负责生成具体 TS 实现的节点。

它不是只生成一个函数，而是基于前面节点的结果和模板生成一组文件：

```Plain
能力文件：每个行为一个 Ability.ts
运行时文件：AutoUEGeneratedRuntime.ts
摄像头辅助：AutoUEGeneratedCameraHelper.ts
场景清单：AutoUEGeneratedSceneManifest.ts
玩家适配层：AutoUEGeneratedCharacterAdapter.ts
GameMode 适配层：AutoUEGeneratedGameModeAdapter.ts
```

**重要说明：**

当前这版里，`AutoUEGeneratedCharacterAdapter.ts` 和 `AutoUEGeneratedGameModeAdapter.ts` 也属于 TypeScriptCodeGenerator 的生成产物。

也就是说，对外解释时可以说：

```Plain
具体实现代码，包括 AIDev 运行需要的适配层，由 TypeScriptCodeGenerator 根据模板和前置节点产物生成。
```

**本轮主要输出：**

```Plain
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\TypeScript\AutoUEGeneratedCharacterAdapter.ts
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\TypeScript\AutoUEGeneratedGameModeAdapter.ts
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\TypeScript\content\generated\AutoUEGeneratedRuntime.ts
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\TypeScript\content\generated\AutoUEGeneratedCameraHelper.ts
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\TypeScript\content\generated\AutoUEGeneratedSceneManifest.ts
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\TypeScript\content\generated\*Ability.ts
```

**本轮能力文件示例：**

```Plain
PlayerHorizontalMovementMoveLeftRightAbility.ts
PlayerShortMeleeAttackStrikeEnemyAbility.ts
PlayerBriefDashDashHorizontallyAbility.ts
PlayerFrozenStateBecomeFrozenAbility.ts
PlayerFrozenStateThawAbility.ts
PatrollingMeleeEnemyHealthTakeMeleeDamageAbility.ts
PatrollingMeleeEnemyHealthBecomeDefeatedAbility.ts
IceTrapFreezeTriggerFreezeOverlappingPlayerAbility.ts
FreezeVfxFreezeFeedbackAppearOnFreezeAbility.ts
SideCameraFreezeFeedbackShakeShakeOnFreezeAbility.ts
LockedExitEnemyGateUnlockAfterEnemyDefeatAbility.ts
LockedExitRoomCompletionCompleteOnReachAbility.ts
```

**检查方式：**

```Plain
是否生成 15 个 behavior 对应的 Ability.ts
是否生成 Runtime / CameraHelper / SceneManifest
是否生成 CharacterAdapter / GameModeAdapter
生成代码是否能被 AIDev 的 tsconfig 编译
生成代码是否能被 TypeScript Blueprint 加载
```

---

## 09. EvaluateInstructionGenerator：生成验证指令

**它做什么：**

把“怎么验收”也变成产物。

它不会只写一句“测试通过”，而是把每个行为应该检查什么写到 `instructions.json`。

例如玩家攻击行为的验证会包含：

```Plain
应该存在 ability module export
应该存在 interactive adapter export
应该映射到 kismet.sphere_trace_single 和 gameplay_statics.apply_damage
应该能追溯到对应 flow、MCP adjudication、runtime mapping 和 TS 文件
```

**本轮结果：**

```Plain
验证指令：15 条
覆盖 behavior：15 个
```

**主要输出：**

```Plain
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\MyPCG\eval\instructions.json
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\MyPCG\eval\Prompt.txt
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1\llm_outputs\EvaluateInstructionGenerator.txt
```

**检查方式：**

```Plain
instructions.json 是否有 15 条 evaluation_instructions
coverage 是否覆盖 15 个 behavior
每条验证是否能回链到 flow / MCP / runtime mapping / TS 文件
```

---

# 场景复用说明

本轮不是新建关卡。

当前 AIDev 绑定的是已有基础关卡：

```Plain
GameDefaultMap=/Game/ThirdPerson/Lvl_ThirdPerson.Lvl_ThirdPerson
EditorStartupMap=/Game/ThirdPerson/Lvl_ThirdPerson.Lvl_ThirdPerson
```

新玩法通过新的 GameMode Blueprint 接入：

```Plain
GlobalDefaultGameMode=/Game/Blueprints/TypeScript/AutoUEGeneratedGameModeAdapter.AutoUEGeneratedGameModeAdapter_C
```

也就是说：

```Plain
关卡壳子：复用 ThirdPerson level
玩法内容：AutoUE 生成 TS
运行入口：AutoUEGeneratedGameModeAdapter
玩家控制：AutoUEGeneratedCharacterAdapter
```

最终落到 AIDev 的关键路径是：

```Plain
D:\UE5.7.4\AIDev\TypeScript\AutoUEGeneratedCharacterAdapter.ts
D:\UE5.7.4\AIDev\TypeScript\AutoUEGeneratedGameModeAdapter.ts
D:\UE5.7.4\AIDev\TypeScript\content\generated\AutoUEGeneratedRuntime.ts
D:\UE5.7.4\AIDev\TypeScript\content\generated\AutoUEGeneratedCameraHelper.ts
D:\UE5.7.4\AIDev\TypeScript\content\generated\AutoUEGeneratedSceneManifest.ts
D:\UE5.7.4\AIDev\Content\Blueprints\TypeScript\AutoUEGeneratedCharacterAdapter.uasset
D:\UE5.7.4\AIDev\Content\Blueprints\TypeScript\AutoUEGeneratedGameModeAdapter.uasset
```

---

# 这个案例里，复杂交互是怎么被拆开的

以“冰冻陷阱”为例。

用户看到的是一句话：

```Plain
玩家踩到冰冻陷阱后会被短暂冻结，并出现冰冻表现和摄像头反馈。
```

流程里会拆成下面这些东西：

## 实体层

```Plain
ice_trap：陷阱本体
player：被影响的玩家
freeze_vfx：冰冻视觉反馈
side_camera：摄像头反馈
```

## 能力层

```Plain
ice_trap.freeze_trigger：陷阱能触发冻结
player.frozen_state：玩家有冻结/解冻状态
freeze_vfx.freeze_feedback：冰冻时能显示效果
side_camera.freeze_feedback_shake：冰冻时能触发摄像头反馈
```

## 行为层

```Plain
ice_trap.freeze_trigger.freeze_overlapping_player：检测玩家踩到陷阱并冻结
player.frozen_state.become_frozen：玩家进入冻结状态
freeze_vfx.freeze_feedback.appear_on_freeze：显示冰冻特效
side_camera.freeze_feedback_shake.shake_on_freeze：摄像头反馈
player.frozen_state.thaw：计时结束后恢复
```

## MCP 层

```Plain
检测触发：primitive.on_component_begin_overlap / overlap query
冻结移动：character_movement.disable_movement
恢复移动：character_movement.set_movement_mode
延迟恢复：timer.set_timer
冰冻表现：niagara.spawn_system_attached
摄像头反馈：player_controller.client_start_camera_shake
```

## 代码层

```Plain
IceTrapFreezeTriggerFreezeOverlappingPlayerAbility.ts
PlayerFrozenStateBecomeFrozenAbility.ts
FreezeVfxFreezeFeedbackAppearOnFreezeAbility.ts
SideCameraFreezeFeedbackShakeShakeOnFreezeAbility.ts
PlayerFrozenStateThawAbility.ts
AutoUEGeneratedRuntime.ts
AutoUEGeneratedCameraHelper.ts
AutoUEGeneratedGameModeAdapter.ts
```

## 运行层

```Plain
玩家进入陷阱范围
  -> Runtime 标记 playerFrozen=true
  -> 玩家移动/攻击被阻止
  -> FreezeVFX visible=true
  -> CameraHelper 触发侧视角/震动反馈
  -> timer 到期
  -> playerFrozen=false
  -> FreezeVFX visible=false
  -> 玩家恢复移动
```

这就是“复杂交互”在当前方案里的处理方式：不是写成一个大函数，而是一路拆成实体、能力、行为、flow、engine port、runtime mapping、TS 文件、验证点。

---

# 战斗系统是怎么写的

当前案例里的战斗系统是最小闭环，不是完整商业战斗框架。

它包含：

```Plain
玩家能移动到敌人附近
玩家能发起一次短近战攻击
攻击检测敌人是否在近距离范围内
命中后敌人扣血
敌人血量归零后进入 defeated
敌人 defeated 后出口解锁
玩家到达出口后房间完成
```

拆分方式是：

```Plain
输入/控制：AutoUEGeneratedCharacterAdapter.ts
战斗状态：AutoUEGeneratedRuntime.ts
攻击能力：PlayerShortMeleeAttackStrikeEnemyAbility.ts
敌人受伤：PatrollingMeleeEnemyHealthTakeMeleeDamageAbility.ts
敌人死亡：PatrollingMeleeEnemyHealthBecomeDefeatedAbility.ts
出口解锁：LockedExitEnemyGateUnlockAfterEnemyDefeatAbility.ts
房间完成：LockedExitRoomCompletionCompleteOnReachAbility.ts
```

运行结果看的是状态变化：

```Plain
EnemyHit enemyHealth=1
EnemyDefeated=1
ExitUnlocked=1
ExitReached=1
RoomComplete=1
```

---

# 摄像头交互是否做了

做了。

摄像头相关不放在战斗大函数里，而是单独拆成：

```Plain
实体：side_camera
能力：side_camera.freeze_feedback_shake
行为：side_camera.freeze_feedback_shake.shake_on_freeze
代码：SideCameraFreezeFeedbackShakeShakeOnFreezeAbility.ts
公共辅助：AutoUEGeneratedCameraHelper.ts
```

运行检查里记录到：

```Plain
Camera loc=(x, -1650, 388)
Camera yaw=90
sideOffsetY=1650
CameraShakeTriggered=1
```

并且有 PIE 截图作为结果证据：

```Plain
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\test\phase3-complex-pie-camera-20260629.png
```

---

# 检查清单

## 1. 工作流配置检查

```PowerShell
.\.venv\Scripts\python.exe autogenerate_qwen.py --dry-run-config --workflow config\workflows\puerts_ts.json --input-dir data\input-phase3-complex-aidev --output-dir data\output-phase3-complex-aidev
```

检查点：

```Plain
9 个 active 节点启用
禁用节点没有被误跑
prompt / factory 配置能解析
```

---

## 2. AutoUE 全流程生成

```PowerShell
.\.venv\Scripts\python.exe autogenerate_qwen.py --workflow config\workflows\puerts_ts.json --input-dir data\input-phase3-complex-aidev --output-dir data\output-phase3-complex-aidev --run-runtime-validation
```

检查点：

```Plain
9 个节点都有输出
不接受跳节点
输出目录存在 demo_1
```

最终输出根目录：

```Plain
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1
```

---

## 3. 节点输出结构检查

```PowerShell
.\.venv\Scripts\python.exe tools\validate_phase2_outputs.py --root data\output-phase3-complex-aidev\demo_1 --write-report
```

检查点：

```Plain
03-thin-gameplay-flow.json 存在，15 个 flow
04-ue-api-mcp/summary.json 存在，blocked 为空
05-puerts-runtime-mapping.json 存在，15 个 mapping
TypeScript 生成文件存在
Evaluate instructions 存在
```

---

## 4. Runtime validation 检查

```PowerShell
.\.venv\Scripts\python.exe tools\validate_runtime_results.py --root data\output-phase3-complex-aidev\demo_1
```

检查点：

```Plain
evaluation_instructions：15 条
coverage：15 个 behavior
每条验证能追溯到 flow / MCP / mapping / TS 文件
```

---

## 5. 自动化测试检查

```PowerShell
.\.venv\Scripts\python.exe -m pytest tests -q
```

本轮结果：

```Plain
26 passed
```

---

## 6. AIDev TypeScript 编译检查

```PowerShell
D:\UE5.7.4\AIDev\node_modules\.bin\tsc.cmd -p D:\UE5.7.4\AIDev\tsconfig.json
```

检查点：

```Plain
生成 TS 能通过 AIDev tsconfig 编译
没有 TypeScript 类型错误阻塞
```

---

## 7. Blueprint 接入检查

生成的 TS 蓝图资产：

```Plain
D:\UE5.7.4\AIDev\Content\Blueprints\TypeScript\AutoUEGeneratedCharacterAdapter.uasset
D:\UE5.7.4\AIDev\Content\Blueprints\TypeScript\AutoUEGeneratedGameModeAdapter.uasset
```

检查点：

```Plain
GameMode 加载 AutoUEGeneratedGameModeAdapter_C
Pawn 加载 AutoUEGeneratedCharacterAdapter_C
DefaultEngine.ini 指向新的 GameMode Blueprint
```

---

## 8. UE / PIE 运行检查

运行检查不是只看日志，而是采样实际状态。

本轮关键采样：

```Plain
Pawn X: 0.0 -> 69.82 -> 373.99 -> 999.88
```

说明玩家位置发生变化。

冰冻陷阱：

```Plain
AUTOUE_GENERATED_PLAYER_FROZEN
FreezeVFX visible=True
FreezeVFX visible=False after thaw
```

战斗：

```Plain
EnemyHit enemyHealth=1
EnemyDefeated=1
ExitUnlocked=1
```

出口：

```Plain
ExitReached=1
RoomComplete=1
AUTOUE_GENERATED_ROOM_COMPLETE
```

摄像头：

```Plain
sideOffsetY=1650
CameraShakeTriggered=1
```

运行证据：

```Plain
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\test\Phase3-complex-rerun-20260629.md
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\test\phase3-complex-runtime-samples-20260629.txt
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\test\phase3-complex-pie-camera-20260629.png
```

---

# 最终产物路径

## AutoUE 输出目录

```Plain
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\repo\AutoUE\data\output-phase3-complex-aidev\demo_1
```

## AIDev 生成代码

```Plain
D:\UE5.7.4\AIDev\TypeScript\AutoUEGeneratedCharacterAdapter.ts
D:\UE5.7.4\AIDev\TypeScript\AutoUEGeneratedGameModeAdapter.ts
D:\UE5.7.4\AIDev\TypeScript\content\generated\AutoUEGeneratedRuntime.ts
D:\UE5.7.4\AIDev\TypeScript\content\generated\AutoUEGeneratedCameraHelper.ts
D:\UE5.7.4\AIDev\TypeScript\content\generated\AutoUEGeneratedSceneManifest.ts
D:\UE5.7.4\AIDev\TypeScript\content\generated\*Ability.ts
D:\UE5.7.4\AIDev\TypeScript\content\generated\interactive\*Interactable.ts
```

## AIDev Blueprint 资产

```Plain
D:\UE5.7.4\AIDev\Content\Blueprints\TypeScript\AutoUEGeneratedCharacterAdapter.uasset
D:\UE5.7.4\AIDev\Content\Blueprints\TypeScript\AutoUEGeneratedGameModeAdapter.uasset
```

## 验证结果

```Plain
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\test\Phase3-complex-rerun-20260629.md
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\test\phase3-complex-runtime-samples-20260629.txt
D:\ClaudeTasks\active\autoue-puerts-workflow-adaptation\test\phase3-complex-pie-camera-20260629.png
```

## 备份目录

```Plain
D:\UE5.7.4\AIDev\_codex_backups\20260629-000534-phase3-complex-camera-fixed
```

---

# 对外一句话版本

当前 AutoUE / PuerTS 流程已经不是“让 AI 直接写一坨 UE 脚本”，而是把用户需求先拆成实体、能力、行为，再拆成薄玩法流，只查询必要 UE/PuerTS API，然后映射到 AIDev 当前运行结构，最后由 TypeScript 代码生成节点生成能力代码、运行时公共层和 GameMode / Character 适配层，并通过 TS 编译、Blueprint 接入、UE / PIE 运行采样来验证玩法闭环。

本轮示例中，系统复用了 AIDev 现有 ThirdPerson 关卡，在其中接入了一个横版战斗房间：玩家能移动、冲刺、攻击敌人，踩冰冻陷阱会冻结并触发 VFX 和摄像头反馈，击败敌人后出口解锁，到达出口后房间完成。
