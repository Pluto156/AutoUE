export interface PatrollingMeleeEnemyMeleePressureThreatenClosePlayerAbilityContext {
  actorName?: string;
  targetName?: string;
  state?: Record<string, unknown>;
  events?: string[];
}

const ENTITY_ID = "patrolling_melee_enemy";
const BEHAVIOR_ID = "patrolling_melee_enemy.melee_pressure.threaten_close_player";
const FLOW_ID = "flow.patrolling_melee_enemy.melee_pressure.threaten_close_player";
const RUNTIME_MAPPING_PATH = "flow/05-puerts-runtime-mapping.json";
const ACTION_LABEL = "threaten close player";
const TARGET_LABEL = "player";
const RESULT_LABEL = "close melee threat is active";

export function runPatrollingMeleeEnemyMeleePressureThreatenClosePlayerAbility(context: PatrollingMeleeEnemyMeleePressureThreatenClosePlayerAbilityContext = {}): string {
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
