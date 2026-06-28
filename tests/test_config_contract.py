
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

ORDER = [
    'SceneAndGameplaySplitter',
    'EntityAbilityBehaviorPlanner',
    'ThinGameplayFlowPlanner',
    'UEApiMCPFeasibilitySearcher',
    'PuerTSRuntimeMappingPlanner',
    'TypeScriptScriptAnalyzer',
    'TypeScriptInteractiveObjectGenerator',
    'TypeScriptCodeGenerator',
    'EvaluateInstructionGenerator',
]
RUNTIME_MAPPING_PATH = 'flow/05-puerts-runtime-mapping.json'
ADJ_INPUT = 'flow/04-ue-api-mcp/adjudication/input.action_binding.json'
ADJ_DAMAGE = 'flow/04-ue-api-mcp/adjudication/damage.apply.json'


def run(cmd):
    return subprocess.run(cmd, cwd=ROOT, text=True, capture_output=True, check=True)


def eab():
    return {
        'entities': [{
            'entity_id': 'player', 'display_name': 'Player', 'summary': 'Playable character',
            'abilities': [{
                'ability_id': 'player.combat', 'display_name': 'Combat', 'summary': 'Attack enemy',
                'behaviors': [{
                    'behavior_id': 'player.combat.attack', 'display_name': 'Attack Enemy',
                    'trigger': 'attack input', 'execution': 'strike enemy', 'result': 'enemy defeated', 'source_refs': []
                }]
            }]
        }],
        'non_goals': []
    }


def thin():
    return {'flows': [{
        'flow_id': 'flow_player_combat_attack', 'entity_id': 'player', 'ability_id': 'player.combat', 'source_behavior_id': 'player.combat.attack',
        'stages': [
            {'stage': 'Input', 'contract': 'bind player attack input', 'inputs': ['attack'], 'outputs': ['requested'], 'engine_ports': ['input.action_binding']},
            {'stage': 'Damage/Resource', 'contract': 'apply damage to enemy', 'inputs': ['enemy'], 'outputs': ['defeated'], 'engine_ports': ['damage.apply']},
        ],
        'verification': ['enemy defeated']
    }]}


def mcp():
    return {'queries': [
        {'engine_port_id': 'input.action_binding', 'flow_ids': ['flow_player_combat_attack'], 'behavior_ids': ['player.combat.attack'], 'query': 'q1', 'raw_path': 'flow/04-ue-api-mcp/raw/input.action_binding.raw.json', 'adjudication_path': ADJ_INPUT, 'verdict': 'hit', 'hit_type': 'direct_hit', 'evidence_symbols': ['UE.EnhancedInputComponent.BindAction'], 'notes': 'ok'},
        {'engine_port_id': 'damage.apply', 'flow_ids': ['flow_player_combat_attack'], 'behavior_ids': ['player.combat.attack'], 'query': 'q2', 'raw_path': 'flow/04-ue-api-mcp/raw/damage.apply.raw.json', 'adjudication_path': ADJ_DAMAGE, 'verdict': 'hit', 'hit_type': 'direct_hit', 'evidence_symbols': ['UE.GameplayStatics.ApplyDamage'], 'notes': 'ok'},
    ], 'summary': {'all_required_ports_hit': True, 'blocked_engine_ports': []}}


def mapping():
    return {'runtime_mapping_path': RUNTIME_MAPPING_PATH, 'mappings': [{
        'entity_id': 'player', 'ability_id': 'player.combat', 'behavior_id': 'player.combat.attack', 'flow_id': 'flow_player_combat_attack',
        'runtime_owner': 'TypeScript/content/generated/SmokeGame.ts', 'implementation_carrier': 'template_rendered_ts', 'selected_runtime_owner': 'SmokeGameAbility',
        'engine_port_mappings': [
            {'engine_port_id': 'input.action_binding', 'adjudication_path': ADJ_INPUT, 'verdict': 'hit', 'evidence_symbols': ['UE.EnhancedInputComponent.BindAction']},
            {'engine_port_id': 'damage.apply', 'adjudication_path': ADJ_DAMAGE, 'verdict': 'hit', 'evidence_symbols': ['UE.GameplayStatics.ApplyDamage']},
        ],
        'thin_contracts': ['bind player attack input', 'apply damage to enemy'], 'ability_binding': 'adapter_call:tickSmokeGame', 'verification_evidence': ['trace']
    }], 'blocked_mappings': []}


def analyzer():
    return {'typescript_sources': [{'path': 'TypeScript/content/generated/SmokeGame.ts', 'role': 'ability', 'notes': 'mapping'}], 'implementation_slots': [{
        'entity_id': 'player', 'behavior_id': 'player.combat.attack', 'flow_id': 'flow_player_combat_attack', 'runtime_mapping_path': RUNTIME_MAPPING_PATH,
        'target_ts_file': 'TypeScript/content/generated/SmokeGame.ts', 'reason': 'mapping'
    }], 'missing_slots': []}


def interactive():
    return {'template_inputs': [{
        'template': 'interactive_object', 'path': 'TypeScript/content/generated/interactive/SmokeInteractable.ts', 'entity_id': 'player', 'behavior_id': 'player.combat.attack', 'flow_id': 'flow_player_combat_attack', 'runtime_mapping_path': RUNTIME_MAPPING_PATH,
        'export_name': 'runSmokeInteraction', 'interface_name': 'SmokeInteractionContext', 'action_label': 'attacks', 'target_label': 'Enemy', 'result_label': 'enemy defeated'
    }], 'behavior_traces': [{
        'entity_id': 'player', 'behavior_id': 'player.combat.attack', 'flow_id': 'flow_player_combat_attack', 'runtime_mapping_path': RUNTIME_MAPPING_PATH, 'file_path': 'TypeScript/content/generated/interactive/SmokeInteractable.ts', 'export_name': 'runSmokeInteraction'
    }], 'validation_notes': []}


def codegen():
    return {'template_inputs': [{
        'template': 'ability_module', 'path': 'TypeScript/content/generated/SmokeGame.ts', 'entity_id': 'player', 'behavior_id': 'player.combat.attack', 'flow_id': 'flow_player_combat_attack', 'runtime_mapping_path': RUNTIME_MAPPING_PATH,
        'export_name': 'tickSmokeGame', 'interface_name': 'SmokeGameContext', 'action_label': 'attacks', 'target_label': 'Enemy', 'result_label': 'enemy defeated'
    }], 'behavior_traces': [{
        'entity_id': 'player', 'behavior_id': 'player.combat.attack', 'flow_id': 'flow_player_combat_attack', 'runtime_mapping_path': RUNTIME_MAPPING_PATH, 'file_path': 'TypeScript/content/generated/SmokeGame.ts', 'export_name': 'tickSmokeGame'
    }], 'consumed_interactive_files': ['TypeScript/content/generated/interactive/SmokeInteractable.ts'], 'validation_notes': []}


def eval_plan():
    trace = {'entity_id': 'player', 'ability_id': 'player.combat', 'behavior_id': 'player.combat.attack', 'flow_id': 'flow_player_combat_attack', 'engine_port_ids': ['input.action_binding', 'damage.apply'], 'adjudication_paths': [ADJ_INPUT, ADJ_DAMAGE], 'runtime_mapping_path': RUNTIME_MAPPING_PATH, 'ts_files': ['TypeScript/content/generated/interactive/SmokeInteractable.ts', 'TypeScript/content/generated/SmokeGame.ts']}
    return {'evaluation_instructions': [{'step_id': 1, 'action': 'attack', 'target': 'Enemy', 'description': 'validate attack', 'driver': 'adapter_call', 'executor_action': 'call_behavior', 'expected': [{'type': 'state_changed', 'key': 'enemy.state', 'expected_value': 'defeated'}], 'trace': trace}], 'coverage': [trace]}


def good_outputs():
    return {
        'SceneAndGameplaySplitter': json.dumps({'scene_description': 'Room', 'gameplay_description': 'Attack enemy'}),
        'EntityAbilityBehaviorPlanner': json.dumps(eab()),
        'ThinGameplayFlowPlanner': json.dumps(thin()),
        'UEApiMCPFeasibilitySearcher': json.dumps(mcp()),
        'PuerTSRuntimeMappingPlanner': json.dumps(mapping()),
        'TypeScriptScriptAnalyzer': json.dumps(analyzer()),
        'TypeScriptInteractiveObjectGenerator': json.dumps(interactive()),
        'TypeScriptCodeGenerator': json.dumps(codegen()),
        'EvaluateInstructionGenerator': json.dumps(eval_plan()),
    }


def test_phase2_banned_nodes_are_disabled():
    result = run([sys.executable, 'tools/validate_config_contract.py', '--workflow', 'config/workflows/puerts_ts.json', '--phase', 'phase2'])
    data = json.loads(result.stdout)
    assert data['result'] == 'pass'
    assert data['enabled_nodes'] == ORDER
    for name in ['RetrieveModel', 'PCGGraphComposer', 'PCGPlanner', 'ModuleCodeGenerator', 'InteractiveObjectCodeGenerator']:
        assert name not in data['enabled_nodes']


def test_dry_run_config_has_prompts():
    result = run([sys.executable, 'autogenerate_qwen.py', '--dry-run-config', '--workflow', 'config/workflows/puerts_ts.json'])
    data = json.loads(result.stdout)
    assert data['missing_prompts'] == []
    assert data['enabled_nodes'] == ORDER


def test_phase2_workflow_uses_task_specific_codex_profiles():
    workflow = json.loads((ROOT / 'config' / 'workflows' / 'puerts_ts.json').read_text(encoding='utf-8'))
    by_name = {node['name']: node for node in workflow['nodes']}
    assert by_name['UEApiMCPFeasibilitySearcher']['llm_profile'] == 'codex_cli_fast'
    assert by_name['PuerTSRuntimeMappingPlanner']['llm_profile'] == 'codex_cli_planning'
    assert by_name['TypeScriptCodeGenerator']['llm_profile'] == 'codex_cli_codegen'


def test_ts_generators_reject_raw_content_output():
    from core.phase2_validation import Phase2ValidationError, validate_phase2_output
    bad = {'files': [{'path': 'TypeScript/content/generated/Bad.ts', 'content': 'export const raw = true;'}], 'behavior_traces': [], 'validation_notes': []}
    try:
        validate_phase2_output('TypeScriptCodeGenerator', json.dumps(bad))
    except Phase2ValidationError as exc:
        assert 'raw files/content' in str(exc)
    else:
        raise AssertionError('validator accepted raw content output')


def test_planner_rejects_implementation_decisions():
    from core.phase2_validation import Phase2ValidationError, validate_phase2_output
    bad = eab()
    bad['entities'][0]['abilities'][0]['behaviors'][0]['target_ts_file'] = 'TypeScript/content/generated/X.ts'
    try:
        validate_phase2_output('EntityAbilityBehaviorPlanner', json.dumps(bad))
    except Phase2ValidationError as exc:
        assert 'definition-only' in str(exc)
    else:
        raise AssertionError('planner accepted implementation fields')


def test_thin_flow_requires_engine_ports():
    from core.phase2_validation import Phase2ValidationError, validate_phase2_output
    bad = thin()
    bad['flows'][0]['stages'][0]['engine_ports'] = []
    bad['flows'][0]['stages'][1]['engine_ports'] = []
    try:
        validate_phase2_output('ThinGameplayFlowPlanner', json.dumps(bad))
    except Phase2ValidationError as exc:
        assert 'engine_port' in str(exc)
    else:
        raise AssertionError('thin flow accepted no engine ports')


def test_mcp_miss_blocks_phase2_done():
    from core.phase2_validation import Phase2ValidationError, validate_phase2_output_set
    outputs = good_outputs()
    bad = mcp()
    bad['queries'][0]['verdict'] = 'miss'
    bad['queries'][0]['hit_type'] = 'none'
    outputs['UEApiMCPFeasibilitySearcher'] = json.dumps(bad)
    try:
        validate_phase2_output_set(outputs)
    except Phase2ValidationError as exc:
        assert 'must be hit' in str(exc)
    else:
        raise AssertionError('phase2 accepted MCP miss')


def test_runtime_mapping_blocked_blocks_phase2_done():
    from core.phase2_validation import Phase2ValidationError, validate_phase2_output_set
    outputs = good_outputs()
    bad = mapping()
    bad['blocked_mappings'] = [{'behavior_id': 'player.combat.attack'}]
    outputs['PuerTSRuntimeMappingPlanner'] = json.dumps(bad)
    try:
        validate_phase2_output_set(outputs)
    except Phase2ValidationError as exc:
        assert 'blocked_mappings' in str(exc)
    else:
        raise AssertionError('phase2 accepted blocked mapping')


def test_analyzer_requires_mapping_trace():
    from core.phase2_validation import Phase2ValidationError, validate_phase2_output
    bad = analyzer()
    del bad['implementation_slots'][0]['runtime_mapping_path']
    try:
        validate_phase2_output('TypeScriptScriptAnalyzer', json.dumps(bad))
    except Phase2ValidationError as exc:
        assert 'runtime_mapping_path' in str(exc)
    else:
        raise AssertionError('analyzer accepted slot without runtime mapping')


def test_phase2_output_set_accepts_good_trace_chain():
    from core.phase2_validation import validate_phase2_output_set
    result = validate_phase2_output_set(good_outputs())
    assert 'player.combat.attack' in result['evidence']['behavior_trace_coverage']
    assert 'input.action_binding' in result['evidence']['engine_port_ids']


def test_phase2_output_set_rejects_unknown_behavior_trace():
    from core.phase2_validation import Phase2ValidationError, validate_phase2_output_set
    outputs = good_outputs()
    bad = analyzer()
    bad['implementation_slots'][0]['behavior_id'] = 'enemy.unknown.behavior'
    outputs['TypeScriptScriptAnalyzer'] = json.dumps(bad)
    try:
        validate_phase2_output_set(outputs)
    except Phase2ValidationError as exc:
        assert 'unknown behavior_id' in str(exc)
    else:
        raise AssertionError('output set accepted unknown behavior')


def test_phase2_output_set_rejects_unrendered_analyzer_target():
    from core.phase2_validation import Phase2ValidationError, validate_phase2_output_set
    outputs = good_outputs()
    bad = analyzer()
    bad['implementation_slots'][0]['target_ts_file'] = 'TypeScript/content/generated/NotRendered.ts'
    outputs['TypeScriptScriptAnalyzer'] = json.dumps(bad)
    try:
        validate_phase2_output_set(outputs)
    except Phase2ValidationError as exc:
        assert 'target_ts_file must equal runtime_owner' in str(exc) or 'must render every analyzer' in str(exc)
    else:
        raise AssertionError('output set accepted analyzer target that codegen did not render')


def test_phase2_output_set_rejects_eval_missing_adjudication_trace():
    from core.phase2_validation import Phase2ValidationError, validate_phase2_output_set
    outputs = good_outputs()
    bad = eval_plan()
    bad['coverage'][0]['adjudication_paths'] = [ADJ_INPUT]
    outputs['EvaluateInstructionGenerator'] = json.dumps(bad)
    try:
        validate_phase2_output_set(outputs)
    except Phase2ValidationError as exc:
        assert 'adjudication_paths' in str(exc)
    else:
        raise AssertionError('output set accepted eval trace missing adjudication')


def test_template_renderer_writes_from_template_not_model_content(tmp_path):
    from core.BaseLLMNode import GraphState
    from custom_nodes.phase2_file_writer import write_files_from_output
    state = GraphState(save_dir=str(tmp_path))
    write_files_from_output(state, 'TypeScriptInteractiveObjectGenerator', json.dumps(interactive()))
    out = tmp_path / 'TypeScript' / 'content' / 'generated' / 'interactive' / 'SmokeInteractable.ts'
    text = out.read_text(encoding='utf-8')
    assert 'export function runSmokeInteraction' in text
    assert 'FLOW_ID' in text
    assert 'RUNTIME_MAPPING_PATH' in text


def _has_current_smoke(root: Path) -> bool:
    return all((root / 'llm_outputs' / f'{node}.txt').exists() for node in ORDER)


def test_existing_phase2_output_validator_passes_when_current_smoke_present():
    root = ROOT / 'data' / 'output-smoke' / 'demo_1'
    if not root.exists() or not _has_current_smoke(root):
        return
    result = run([sys.executable, 'tools/validate_phase2_outputs.py', '--root', str(root)])
    data = json.loads(result.stdout)
    assert data['result'] == 'pass'


def test_existing_smoke_output_validator_passes_when_current_smoke_present():
    root = ROOT / 'data' / 'output-smoke' / 'demo_1'
    if not root.exists() or not _has_current_smoke(root):
        return
    result = run([sys.executable, 'tools/validate_smoke_outputs.py', '--root', str(root)])
    data = json.loads(result.stdout)
    assert data['result'] == 'pass'


def test_mcp_config_can_fallback_to_codex_config_or_env(monkeypatch):
    from core.mcp_client import load_ue_api_mcp_config
    monkeypatch.setenv('AUTOUE_UE_API_MCP_COMMAND', 'python')
    monkeypatch.setenv('AUTOUE_UE_API_MCP_ARGS', '["-m", "ue_api_search_mcp.server"]')
    cfg = load_ue_api_mcp_config({})
    assert cfg.command == 'python'
    assert cfg.args == ['-m', 'ue_api_search_mcp.server']
