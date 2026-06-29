export interface FreezeVfxFreezeFeedbackAppearOnFreezeAbilityContext {
  actorName?: string;
  targetName?: string;
  state?: Record<string, unknown>;
  events?: string[];
}

const ENTITY_ID = "freeze_vfx";
const BEHAVIOR_ID = "freeze_vfx.freeze_feedback.appear_on_freeze";
const FLOW_ID = "flow.freeze_vfx.freeze_feedback.appear_on_freeze";
const RUNTIME_MAPPING_PATH = "flow/05-puerts-runtime-mapping.json";
const ACTION_LABEL = "show freeze effect";
const TARGET_LABEL = "player";
const RESULT_LABEL = "freeze VFX appears";

export function runFreezeVfxFreezeFeedbackAppearOnFreezeAbility(context: FreezeVfxFreezeFeedbackAppearOnFreezeAbilityContext = {}): string {
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
