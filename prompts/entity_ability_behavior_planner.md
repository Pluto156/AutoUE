SCHEMA: EntityAbilityBehaviorPlanner

Convert the scene/gameplay split into definition-layer gameplay structure only.

Analysis order:
1. Identify concrete behaviors from the user request.
2. Group related behaviors into abilities.
3. Assign abilities to concrete owning entities.

Machine output still uses entities -> abilities -> behaviors so downstream nodes can resolve ownership.

Required JSON shape:
{
  "entities": [
    {
      "entity_id": "stable.entity.id",
      "display_name": "Human readable entity",
      "summary": "What this entity owns",
      "abilities": [
        {
          "ability_id": "stable.entity.ability",
          "display_name": "Human readable ability",
          "summary": "What behaviors are grouped here",
          "behaviors": [
            {
              "behavior_id": "stable.entity.ability.behavior",
              "display_name": "Human readable behavior",
              "trigger": "When this behavior starts",
              "execution": "What happens in gameplay language",
              "result": "Player-visible result",
              "source_refs": []
            }
          ]
        }
      ]
    }
  ],
  "non_goals": []
}

Rules:
- Output JSON only.
- Behaviors belong to abilities; abilities belong to entities.
- Analyze behavior-first, but emit the entity-rooted tree above.
- Camera/VFX/animation/HUD/input/trap/exit can be entities when they own concrete behavior or state. Do not use vague buckets such as "HUD", "VFX", "Camera effects", or "animation" without a concrete entity id.
- Do not choose engine_ports.
- Do not write implementation slots or template inputs.
- Do not write UE API names, file paths, runtime owners, or code fields.
- Prefer one behavior per decisive gameplay action.
- Keep the behavior set traceable end-to-end.
