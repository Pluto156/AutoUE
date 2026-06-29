export interface FreezeVfxAppearInteractionContext {
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

export function runFreezeVfxAppearInteraction(context: FreezeVfxAppearInteractionContext = {}): string {
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
