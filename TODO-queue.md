# TODO Queue

## LLMHttpServer plugin migration decision

- status: pending
- current decision: banned from Python workflow adaptation and PuerTS Phase2 flow
- reason: AutoUE's own LLMHttpServer plugin is unrelated to the immediate Python workflow smoke and may conflict with the target OpenAI-compatible service path.
- revisit when: the PuerTS TS generation loop is stable and we decide whether any runtime LLM call should move into UE.
