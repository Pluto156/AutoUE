export interface LockedExitRoomCompletionCompleteOnReachAbilityContext {
  actorName?: string;
  targetName?: string;
  state?: Record<string, unknown>;
  events?: string[];
}

const ENTITY_ID = "locked_exit";
const BEHAVIOR_ID = "locked_exit.room_completion.complete_on_reach";
const FLOW_ID = "flow.locked_exit.room_completion.complete_on_reach";
const RUNTIME_MAPPING_PATH = "flow/05-puerts-runtime-mapping.json";
const ACTION_LABEL = "complete on reach";
const TARGET_LABEL = "unlocked exit";
const RESULT_LABEL = "room is completed";

export function runLockedExitRoomCompletionCompleteOnReachAbility(context: LockedExitRoomCompletionCompleteOnReachAbilityContext = {}): string {
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
