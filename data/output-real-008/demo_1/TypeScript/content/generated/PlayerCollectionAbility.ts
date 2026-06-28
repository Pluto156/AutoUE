export interface PlayerCollectionAbilityContext {
  actorName?: string;
  targetName?: string;
  state?: Record<string, unknown>;
  events?: string[];
}

const ENTITY_ID = "player";
const BEHAVIOR_ID = "player.collection.collect_reward";
const FLOW_ID = "flow_player_collection_collect_reward";
const RUNTIME_MAPPING_PATH = "flow/05-puerts-runtime-mapping.json";
const ACTION_LABEL = "collect";
const TARGET_LABEL = "reward";
const RESULT_LABEL = "reward collected";

export function runPlayerCollectionAbility(context: PlayerCollectionAbilityContext = {}): string {
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
