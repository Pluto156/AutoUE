export interface IceTrapFreezePlayerInteractionContext {
  actorName?: string;
  targetName?: string;
  state?: Record<string, unknown>;
  events?: string[];
}

const ENTITY_ID = "ice_trap";
const BEHAVIOR_ID = "ice_trap.freeze_trigger.freeze_overlapping_player";
const FLOW_ID = "flow.ice_trap.freeze_trigger.freeze_overlapping_player";
const RUNTIME_MAPPING_PATH = "flow/05-puerts-runtime-mapping.json";
const ACTION_LABEL = "freeze overlapping player";
const TARGET_LABEL = "player";
const RESULT_LABEL = "player is immobilized with feedback";

export function runIceTrapFreezePlayerInteraction(context: IceTrapFreezePlayerInteractionContext = {}): string {
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
