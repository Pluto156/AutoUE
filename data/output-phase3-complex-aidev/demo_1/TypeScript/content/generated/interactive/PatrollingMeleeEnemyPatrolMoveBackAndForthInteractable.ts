export interface EnemyPatrolInteractionContext {
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

export function runEnemyPatrolInteraction(context: EnemyPatrolInteractionContext = {}): string {
  const actorName = context.actorName || 'Actor';
  const targetName = context.targetName || TARGET_LABEL;
  const eventText = actorName + ' performs ' + ACTION_LABEL + ' on ' + targetName;
  if (context.events) {
    context.events.push(eventText);
  }
  if (context.state) {
    context.state[BEHAVIOR_ID] = RESULT_LABEL;
    context.state[ENTITY_ID + ':lastAction'] = ACTION_LABEL;
    context.state[BEHAVIOR_ID + ':flow'] = FLOW_ID;
    context.state[BEHAVIOR_ID + ':runtimeMapping'] = RUNTIME_MAPPING_PATH;
  }
  return RESULT_LABEL;
}
