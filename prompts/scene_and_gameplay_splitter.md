SCHEMA: SceneAndGameplaySplitter

Split the user's request into scene_description and gameplay_description.

Required JSON shape:
{
  "scene_description": "Concrete environment, visible objects, spatial layout, mood. Empty string if absent.",
  "gameplay_description": "Player actions, interactions, mechanics, objectives. Empty string if absent."
}

Rules:
- Output JSON only. The first character must be { and the last character must be }.
- Do not invent mechanics beyond the user input.
- Do not include implementation files, asset retrieval steps, procedural asset graphs, or server/plugin dependencies.
