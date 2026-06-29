export interface IceTrapFreezeTriggerFreezeOverlappingPlayerAbilityContext {
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

export function runIceTrapFreezeTriggerFreezeOverlappingPlayerAbility(context: IceTrapFreezeTriggerFreezeOverlappingPlayerAbilityContext = {}): string {
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
