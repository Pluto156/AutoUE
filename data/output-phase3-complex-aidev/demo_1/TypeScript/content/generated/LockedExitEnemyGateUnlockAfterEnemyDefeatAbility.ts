export interface LockedExitEnemyGateUnlockAfterEnemyDefeatAbilityContext {
  actorName?: string;
  targetName?: string;
  state?: Record<string, unknown>;
  events?: string[];
}

const ENTITY_ID = "locked_exit";
const BEHAVIOR_ID = "locked_exit.enemy_gate.unlock_after_enemy_defeat";
const FLOW_ID = "flow.locked_exit.enemy_gate.unlock_after_enemy_defeat";
const RUNTIME_MAPPING_PATH = "flow/05-puerts-runtime-mapping.json";
const ACTION_LABEL = "unlock after defeat";
const TARGET_LABEL = "locked exit";
const RESULT_LABEL = "exit becomes reachable";

export function runLockedExitEnemyGateUnlockAfterEnemyDefeatAbility(context: LockedExitEnemyGateUnlockAfterEnemyDefeatAbilityContext = {}): string {
  const actorName = context.actorName || 'Actor';
  const targetName = context.targetName || TARGET_LABEL;
  const message = actorName + ' ' + ACTION_LABEL + ' ' + targetName + ': ' + RESULT_LABEL;
  if (context.events) {
    context.events.push(message);
  }
  if (context.state) {
    context.state[BEHAVIOR_ID] = { entityId: ENTITY_ID, flowId: FLOW_ID, runtimeMappingPath: RUNTIME_MAPPING_PATH, result: RESULT_LABEL };
  }
  return message;
}
