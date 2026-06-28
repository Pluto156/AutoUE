export interface PlayerCombatAttackEnemyInteractionContext {
  actorName?: string;
  targetName?: string;
  state?: Record<string, unknown>;
  events?: string[];
}

const ENTITY_ID = "player";
const BEHAVIOR_ID = "player.combat.attack_enemy";
const FLOW_ID = "flow_player_combat_attack_enemy";
const RUNTIME_MAPPING_PATH = "flow/05-puerts-runtime-mapping.json";
const ACTION_LABEL = "attack";
const TARGET_LABEL = "enemy";
const RESULT_LABEL = "enemy defeated";

export function runPlayerCombatAttackEnemyInteraction(context: PlayerCombatAttackEnemyInteractionContext = {}): string {
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
