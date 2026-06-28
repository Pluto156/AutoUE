SCHEMA: EntityAbilityBehaviorPlanner

Convert the scene/gameplay split into definition-layer gameplay structure only.

Required JSON shape:
{
  "entities": [
    {
      "entity_id": "stable.entity.id",
      "display_name": "Human readable name",
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
              "trigger": "When it happens",
              "execution": "What happens",
              "result": "Observable result",
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
- This node is definition-only.
- Do not choose TypeScript files.
- Do not choose engine_ports.
- Do not write implementation slots or template inputs.

Granularity guidance:
- Prefer one behavior per decisive gameplay action.
- Do not duplicate reciprocal object-state behaviors for the same player action unless the object has a separate required action.
- For this Phase2 migration, keep the behavior set small enough that every behavior can be traced end-to-end.
