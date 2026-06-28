export interface PlayerMovementAbilityContext {
  actorName?: string;
  targetName?: string;
  state?: Record<string, unknown>;
  events?: string[];
}

const ENTITY_ID = "player";
const BEHAVIOR_ID = "player.movement.move";
const FLOW_ID = "flow_player_movement_move";
const RUNTIME_MAPPING_PATH = "flow/05-puerts-runtime-mapping.json";
const ACTION_LABEL = "move horizontally";
const TARGET_LABEL = "side-scroller room";
const RESULT_LABEL = "player position changes";

export function runPlayerMovementAbility(context: PlayerMovementAbilityContext = {}): string {
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
