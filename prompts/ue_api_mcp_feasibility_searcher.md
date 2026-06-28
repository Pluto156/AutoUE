SCHEMA: UEApiMCPFeasibilitySearcher

You receive thin flows plus raw UE API MCP search records already written by Python. Your job is only to adjudicate whether each engine_port can be implemented.

Required JSON shape:
{
  "queries": [
    {
      "engine_port_id": "input.action_binding",
      "flow_ids": ["flow id"],
      "behavior_ids": ["behavior id"],
      "query": "copy query from raw_records",
      "raw_path": "flow/04-ue-api-mcp/raw/input.action_binding.raw.json",
      "adjudication_path": "flow/04-ue-api-mcp/adjudication/input.action_binding.json",
      "verdict": "hit",
      "hit_type": "direct_hit|indirect_hit|none",
      "evidence_symbols": ["UE.Symbol"],
      "notes": "why this can or cannot support the port"
    }
  ],
  "summary": {
    "all_required_ports_hit": true,
    "blocked_engine_ports": []
  }
}

Rules:
- Output JSON only.
- Copy engine_port_id, flow_ids, behavior_ids, query, raw_path, and adjudication_path exactly from raw_records.
- verdict must be hit only when the raw result contains usable UE/PuerTS API evidence.
- If verdict is miss, hit_type must be none.
- Phase2 done requires all required ports to be hit; do not hide misses.

Adjudication guidance:
- Use direct_hit when a candidate symbol directly names the requested operation.
- Use indirect_hit when the top candidates are usable UE/PuerTS types, components, helper functions, or member evidence that can carry the port even if no exact operation symbol appears.
- If mcp_summary.status is ok and candidate_symbols is non-empty, prefer hit with hit_type=direct_hit or indirect_hit and list the best candidate symbols.
- Use miss only when there are no candidate symbols or the candidates are clearly unrelated to the engine_port contract.
