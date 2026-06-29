export interface PlayerShortMeleeAttackStrikeEnemyAbilityContext {
  actorName?: string;
  targetName?: string;
  state?: Record<string, unknown>;
  events?: string[];
}

const ENTITY_ID = "player";
const BEHAVIOR_ID = "player.short_melee_attack.strike_enemy";
const FLOW_ID = "flow.player.short_melee_attack.strike_enemy";
const RUNTIME_MAPPING_PATH = "flow/05-puerts-runtime-mapping.json";
const ACTION_LABEL = "strike enemy";
const TARGET_LABEL = "patrolling melee enemy";
const RESULT_LABEL = "enemy loses health";

export function runPlayerShortMeleeAttackStrikeEnemyAbility(context: PlayerShortMeleeAttackStrikeEnemyAbilityContext = {}): string {
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
