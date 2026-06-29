export interface SideCameraShakeOnFreezeInteractionContext {
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

export function runSideCameraShakeOnFreezeInteraction(context: SideCameraShakeOnFreezeInteractionContext = {}): string {
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
