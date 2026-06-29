export interface PlayerMoveLeftRightInteractionContext {
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

export function runPlayerMoveLeftRightInteraction(context: PlayerMoveLeftRightInteractionContext = {}): string {
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
