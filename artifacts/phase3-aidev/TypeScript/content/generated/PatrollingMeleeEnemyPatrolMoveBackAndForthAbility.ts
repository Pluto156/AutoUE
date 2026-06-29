export interface PatrollingMeleeEnemyPatrolMoveBackAndForthAbilityContext {
  actorName?: string;
  targetName?: string;
  state?: Record<string, unknown>;
  events?: string[];
}

const ENTITY_ID = "patrolling_melee_enemy";
const BEHAVIOR_ID = "patrolling_melee_enemy.patrol.move_back_and_forth";
const FLOW_ID = "flow.patrolling_melee_enemy.patrol.move_back_and_forth";
const RUNTIME_MAPPING_PATH = "flow/05-puerts-runtime-mapping.json";
const ACTION_LABEL = "patrol back and forth";
const TARGET_LABEL = "patrolling melee enemy";
const RESULT_LABEL = "enemy creates moving hazard";

export function runPatrollingMeleeEnemyPatrolMoveBackAndForthAbility(context: PatrollingMeleeEnemyPatrolMoveBackAndForthAbilityContext = {}): string {
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
