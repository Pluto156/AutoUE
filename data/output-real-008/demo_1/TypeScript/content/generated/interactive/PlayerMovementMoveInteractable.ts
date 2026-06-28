export interface PlayerMovementMoveInteractionContext {
  actorName?: string;
  targetName?: string;
  state?: Record<string, unknown>;
  events?: string[];
}

const ENTITY_ID = "player";
const BEHAVIOR_ID = "player.movement.move";
const FLOW_ID = "flow_player_movement_move";
const RUNTIME_MAPPING_PATH = "flow/05-puerts-runtime-mapping.json";
const ACTION_LABEL = "move through";
const TARGET_LABEL = "side-scroller room";
const RESULT_LABEL = "player position changes";

export function runPlayerMovementMoveInteraction(context: PlayerMovementMoveInteractionContext = {}): string {
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
