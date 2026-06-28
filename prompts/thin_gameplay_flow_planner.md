SCHEMA: ThinGameplayFlowPlanner

Turn every behavior into a thin gameplay flow. This node describes gameplay execution contracts and engine capability ports only.

Required JSON shape:
{
  "flows": [
    {
      "flow_id": "stable_flow_id",
      "entity_id": "entity id from planner",
      "ability_id": "ability id from planner",
      "source_behavior_id": "behavior id from planner",
      "stages": [
        {
          "stage": "Input|Ability/Action|SpatialQuery/HitQuery|Damage/Resource|Event/Result|Feedback/HUD|Cleanup|Custom",
          "contract": "what this stage must do",
          "inputs": [],
          "outputs": [],
          "engine_ports": ["input.action_binding"]
        }
      ],
      "verification": []
    }
  ]
}

Rules:
- Output JSON only.
- Create exactly one flow for every behavior.
- Do not choose TypeScript files.
- Do not write code.
- engine_ports are query seeds for UE API MCP. Use stable snake/dot ids such as input.action_binding, collision.line_trace, damage.apply.
- Every flow must have at least one engine_port.
- Choose only 1-2 essential engine_ports per flow; prefer the narrowest ports needed to prove implementability.

Recommended engine_port style:
- Prefer UE/PuerTS-like API intents that can be searched directly, e.g. pawn.add_movement_input, enhanced_input.bind_action, kismet.line_trace_single, gameplay_statics.apply_damage, primitive.on_component_begin_overlap, actor.destroy, actor.set_actor_hidden_in_game, gameplay_statics.open_level.
- Avoid vague ports such as gameplay.event_dispatch unless no more specific engine capability is needed.

Phase2 port boundary:
- For reward collection and exit activation, primitive.on_component_begin_overlap is enough to prove the engine activation boundary.
- Do not add actor.set_actor_hidden_in_game, actor.destroy, collision.set_enabled, or other post-result visual/state cleanup ports for collection/exit in Phase2; represent those result states in the TS runtime mapping and validation plan.
- For combat, prefer kismet.line_trace_single or kismet.sphere_trace_single plus gameplay_statics.apply_damage.
- For movement, prefer pawn.add_movement_input or character_movement.component only if movement is a decisive behavior.
