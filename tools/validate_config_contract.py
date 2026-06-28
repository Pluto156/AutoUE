from __future__ import annotations
import argparse,json,re,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path: sys.path.insert(0,str(ROOT))
from core.phase2_validation import PHASE2_NODE_ORDER
ABS_WIN=re.compile(r"(?i)(?<![A-Z])[A-Z]:[\\/](?![\\/])")
PHASE2_EXPECTED_BANNED={"RetrieveModel","PCGGraphComposer","PCGPlanner","ModuleCodeGenerator","InteractiveObjectCodeGenerator"}
PHASE2_LEGACY_INACTIVE=PHASE2_EXPECTED_BANNED|{"SceneFormalizer","KeyElementExtractor","ModuleAnalyzer","InteractiveObjectAnalyzer"}
def load_json(rel):
    with (ROOT/rel).open('r',encoding='utf-8') as f: return json.load(f)
def walk_strings(v):
    if isinstance(v,str): yield v
    elif isinstance(v,dict):
        for x in v.values(): yield from walk_strings(x)
    elif isinstance(v,list):
        for x in v: yield from walk_strings(x)
def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--workflow',default='config/workflows/default.json'); ap.add_argument('--phase',choices=['phase1','phase2'],default='phase1'); args=ap.parse_args(); errors=[]
    env=(ROOT/'.env').read_text(encoding='utf-8') if (ROOT/'.env').exists() else ''
    for i,line in enumerate(env.splitlines(),1):
        if ABS_WIN.search(line): errors.append(f'.env:{i}: hard-coded absolute Windows path: {line}')
    for rel in ['config/local.example.json','config/llm-profiles.example.json',args.workflow]:
        data=load_json(rel)
        for s in walk_strings(data):
            if ABS_WIN.search(s): errors.append(f'{rel}: hard-coded absolute Windows path: {s}')
    workflow=load_json(args.workflow); enabled=[n.get('name') for n in workflow.get('nodes',[]) if n.get('enabled',True)]
    if len(enabled)!=len(set(enabled)): errors.append(f'duplicate enabled nodes: {enabled}')
    for n in workflow.get('nodes',[]):
        prompt=n.get('prompt_file')
        if n.get('enabled',True) and prompt and not (ROOT/prompt).exists(): errors.append(f"missing prompt file for {n.get('name')}: {prompt}")
    if args.phase=='phase2':
        banned=set(workflow.get('banned_nodes',[])); missing=sorted(PHASE2_EXPECTED_BANNED-banned)
        if missing: errors.append(f'phase2 workflow must explicitly ban nodes: {missing}')
        leaked=sorted((banned|PHASE2_LEGACY_INACTIVE).intersection(enabled))
        if leaked: errors.append(f'phase2 banned/legacy nodes enabled: {leaked}')
        if enabled!=PHASE2_NODE_ORDER: errors.append(f'phase2 enabled node order mismatch: expected {PHASE2_NODE_ORDER}, got {enabled}')
    if errors:
        print(json.dumps({'result':'fail','errors':errors},indent=2,ensure_ascii=False)); return 1
    print(json.dumps({'result':'pass','workflow':workflow.get('name'),'enabled_nodes':enabled},indent=2,ensure_ascii=False)); return 0
if __name__=='__main__': raise SystemExit(main())
