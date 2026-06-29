export interface SideCameraFreezeFeedbackShakeShakeOnFreezeAbilityContext {
  actorName?: string;
  targetName?: string;
  state?: Record<string, unknown>;
  events?: string[];
}

const ENTITY_ID = "side_camera";
const BEHAVIOR_ID = "side_camera.freeze_feedback_shake.shake_on_freeze";
const FLOW_ID = "flow.side_camera.freeze_feedback_shake.shake_on_freeze";
const RUNTIME_MAPPING_PATH = "flow/05-puerts-runtime-mapping.json";
const ACTION_LABEL = "shake on freeze";
const TARGET_LABEL = "side camera";
const RESULT_LABEL = "brief camera shake plays";

export function runSideCameraFreezeFeedbackShakeShakeOnFreezeAbility(context: SideCameraFreezeFeedbackShakeShakeOnFreezeAbilityContext = {}): string {
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
