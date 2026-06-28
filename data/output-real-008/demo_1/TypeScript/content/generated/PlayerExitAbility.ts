export interface PlayerExitAbilityContext {
  actorName?: string;
  targetName?: string;
  state?: Record<string, unknown>;
  events?: string[];
}

const ENTITY_ID = "player";
const BEHAVIOR_ID = "player.exit.leave_room";
const FLOW_ID = "flow_player_exit_leave_room";
const RUNTIME_MAPPING_PATH = "flow/05-puerts-runtime-mapping.json";
const ACTION_LABEL = "leave";
const TARGET_LABEL = "room exit";
const RESULT_LABEL = "room completed";

export function runPlayerExitAbility(context: PlayerExitAbilityContext = {}): string {
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
