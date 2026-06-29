export interface LockedExitUnlockInteractionContext {
  actorName?: string;
  targetName?: string;
  state?: Record<string, unknown>;
  events?: string[];
}

const ENTITY_ID = "locked_exit";
const BEHAVIOR_ID = "locked_exit.enemy_gate.unlock_after_enemy_defeat";
const FLOW_ID = "flow.locked_exit.enemy_gate.unlock_after_enemy_defeat";
const RUNTIME_MAPPING_PATH = "flow/05-puerts-runtime-mapping.json";
const ACTION_LABEL = "unlock after defeat";
const TARGET_LABEL = "locked exit";
const RESULT_LABEL = "exit becomes reachable";

export function runLockedExitUnlockInteraction(context: LockedExitUnlockInteractionContext = {}): string {
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
