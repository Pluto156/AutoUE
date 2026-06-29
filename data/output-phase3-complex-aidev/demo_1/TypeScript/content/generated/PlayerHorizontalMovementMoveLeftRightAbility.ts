export interface PlayerHorizontalMovementMoveLeftRightAbilityContext {
  actorName?: string;
  targetName?: string;
  state?: Record<string, unknown>;
  events?: string[];
}

const ENTITY_ID = "player";
const BEHAVIOR_ID = "player.horizontal_movement.move_left_right";
const FLOW_ID = "flow.player.horizontal_movement.move_left_right";
const RUNTIME_MAPPING_PATH = "flow/05-puerts-runtime-mapping.json";
const ACTION_LABEL = "move left/right";
const TARGET_LABEL = "player";
const RESULT_LABEL = "player repositions horizontally";

export function runPlayerHorizontalMovementMoveLeftRightAbility(context: PlayerHorizontalMovementMoveLeftRightAbilityContext = {}): string {
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
