const ENTITY_ID = "player";
const BEHAVIOR_ID = "player.combat.attack";
const FLOW_ID = "flow_player_combat_attack";
const RUNTIME_MAPPING_PATH = "flow/05-puerts-runtime-mapping.json";

export interface AutoUEGeneratedSceneManifestContext {
  runId?: string;
}

export function getAutoUEGeneratedSceneManifest(_context: AutoUEGeneratedSceneManifestContext = {}): Record<string, unknown> {
  return {
    schema_version: 'autoue-generated-scene-manifest/v2',
    source_behavior_id: BEHAVIOR_ID,
    source_entity_id: ENTITY_ID,
    flow_id: FLOW_ID,
    runtime_mapping_path: RUNTIME_MAPPING_PATH,
    harness_input_tags: [
      'AUTOUE_INPUT_RIGHT',
      'AUTOUE_INPUT_RIGHT_1S',
      'AUTOUE_INPUT_RIGHT_3S',
      'AUTOUE_INPUT_LEFT',
      'AUTOUE_INPUT_DASH',
      'AUTOUE_INPUT_ATTACK',
      'AUTOUE_INPUT_RESET'
    ],
    actors: [
      { actor_name: 'AutoUEGenerated_Player', type: 'player', tags: ['AUTOUE_GENERATED_PLAYER'], purpose: 'controllable character and runtime state owner' },
      { actor_name: 'AutoUEGenerated_Enemy', type: 'enemy', tags: ['AUTOUE_GENERATED_ENEMY'], purpose: 'patrolling melee enemy with health and defeated state' },
      { actor_name: 'AutoUEGenerated_IceTrap', type: 'hazard', tags: ['AUTOUE_GENERATED_ICE_TRAP'], purpose: 'one-shot freeze trap that rearms only after player leaves radius' },
      { actor_name: 'AutoUEGenerated_FreezeVFX', type: 'feedback', tags: ['AUTOUE_GENERATED_FREEZE_VFX'], purpose: 'visible while player is frozen' },
      { actor_name: 'AutoUEGenerated_SideCamera', type: 'camera', tags: ['AUTOUE_GENERATED_SIDE_CAMERA'], purpose: 'side camera view target for gameplay and screenshot alignment' },
      { actor_name: 'AutoUEGenerated_Exit', type: 'exit', tags: ['AUTOUE_GENERATED_EXIT'], purpose: 'locked room exit unlocked after enemy defeat' }
    ]
  };
}
