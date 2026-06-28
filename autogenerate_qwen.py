from __future__ import annotations

import argparse
import json
import shutil
from datetime import datetime
from pathlib import Path

from dotenv import load_dotenv
load_dotenv()

from core.config import (
    iter_enabled_nodes,
    load_runtime_config,
    load_workflow_config,
    resolve_configured_path,
    resolve_copy_dirs,
    repo_path,
)
from core.llm_factory import create_llm
from core.config import load_llm_profiles
from core.runtime_validation import run_runtime_validation, runtime_validation_config


SCRIPT_DIR = Path(__file__).resolve().parent
DEMO_FINISH_LOG_PATH = SCRIPT_DIR / "demo_finish_log.txt"


def copy_all_dirs_to_output(demo_output_dir: Path, copy_dirs: list[Path]):
    for src_dir in copy_dirs:
        if not src_dir.exists():
            print(f"[WARN] Source directory does not exist, skipping: {src_dir}")
            continue
        dst_dir = demo_output_dir / src_dir.name
        print(f"[DEBUG] Copying {src_dir} -> {dst_dir}")
        if dst_dir.exists():
            shutil.rmtree(dst_dir)
        shutil.copytree(src_dir, dst_dir)


def copy_prompt_to_eval(txt_path: Path, demo_output_dir: Path):
    eval_dir = demo_output_dir / "MyPCG" / "eval"
    eval_dir.mkdir(parents=True, exist_ok=True)
    target_path = eval_dir / "Prompt.txt"
    if target_path.exists():
        target_path.unlink()
    shutil.copy(txt_path, target_path)
    print(f"[DEBUG] Prompt copied to {target_path}")


def save_llm_outputs(graph_state, demo_output_dir: Path, dir_name: str = "llm_outputs"):
    llm_outputs = graph_state.get("llm_outputs") if isinstance(graph_state, dict) else getattr(graph_state, "llm_outputs", None)
    if not llm_outputs:
        print("[WARN] No llm_outputs found in GraphState")
        return
    output_dir = demo_output_dir / dir_name
    output_dir.mkdir(parents=True, exist_ok=True)
    for key, value in llm_outputs.items():
        file_path = output_dir / f"{key}.txt"
        file_path.write_text(value, encoding="utf-8")
        print(f"[DEBUG] Saved llm_output: {file_path}")


def is_runtime_validation_enabled(args, runtime_config: dict) -> bool:
    runtime_section = runtime_config.get("runtime_validation", {})
    config_enabled = bool(runtime_section.get("enabled", False)) if isinstance(runtime_section, dict) else False
    return bool(getattr(args, "run_runtime_validation", False) or config_enabled)


def run_runtime_validation_for_demo(demo_output_dir: Path) -> dict:
    return run_runtime_validation(demo_output_dir, write_outputs=True)


def build_graph(runtime_config: dict | None = None, workflow_config: dict | None = None, *, llm_profile: str | None = None):
    from langgraph.graph import StateGraph
    from core.BaseLLMGraph import BaseLLMGraph
    from core.BaseLLMNode import GraphState
    from core.workflow_loader import create_node_from_spec

    runtime_config = runtime_config or load_runtime_config()
    workflow_config = workflow_config or load_workflow_config(None, runtime_config)
    profiles = load_llm_profiles()
    selected_profile = llm_profile or runtime_config.get("llm_profile") or profiles.get("default_profile")
    model = create_llm(selected_profile, profiles)

    base_graph = BaseLLMGraph(model=model)
    nodes = []
    for spec in iter_enabled_nodes(workflow_config):
        node_profile = selected_profile if llm_profile else (spec.get("llm_profile") or selected_profile)
        node_model = create_llm(node_profile, profiles) if node_profile != selected_profile else model
        node = create_node_from_spec(spec)
        node.set_model(node_model)
        base_graph.AddNode(node)
        nodes.append(node)

    if not nodes:
        raise RuntimeError("Workflow has no enabled nodes")

    graph = StateGraph(GraphState, name=workflow_config.get("name", "AutoUEGraph"))
    for node in nodes:
        graph.add_node(node.name, node.execute)
    graph.set_entry_point(nodes[0].name)
    return graph.compile(), nodes


def dry_run_config(args) -> int:
    runtime = load_runtime_config(args.config)
    workflow = load_workflow_config(args.workflow, runtime)
    input_dir = Path(args.input_dir).resolve() if args.input_dir else resolve_configured_path(runtime, "input_dir")
    output_dir = Path(args.output_dir).resolve() if args.output_dir else resolve_configured_path(runtime, "output_dir")
    enabled = list(iter_enabled_nodes(workflow))
    prompt_files = [repo_path(n["prompt_file"]) for n in enabled if n.get("prompt_file")]
    missing_prompts = [str(p) for p in prompt_files if not p.exists()]
    summary = {
        "runtime_config": runtime,
        "workflow_name": workflow.get("name"),
        "enabled_nodes": [n.get("name") for n in enabled],
        "input_dir": str(input_dir),
        "output_dir": str(output_dir),
        "runtime_validation": runtime_validation_config(enabled=is_runtime_validation_enabled(args, runtime)),
        "missing_prompts": missing_prompts,
    }
    print(json.dumps(summary, indent=2, ensure_ascii=False))
    return 2 if missing_prompts else 0


def run_workflow(args) -> int:
    from langchain_core.messages import HumanMessage
    from core.BaseLLMNode import GraphState
    from core.workflow_loader import read_prompt_text

    runtime = load_runtime_config(args.config)
    workflow = load_workflow_config(args.workflow, runtime)
    input_dir = Path(args.input_dir).resolve() if args.input_dir else resolve_configured_path(runtime, "input_dir")
    output_dir = Path(args.output_dir).resolve() if args.output_dir else resolve_configured_path(runtime, "output_dir")
    llm_outputs_dir_name = runtime.get("paths", {}).get("llm_outputs_dir_name", "llm_outputs")
    post_actions = runtime.get("post_actions", {})
    copy_dirs = resolve_copy_dirs(runtime)
    run_runtime = is_runtime_validation_enabled(args, runtime)
    runtime_failures: list[dict] = []

    if not input_dir.exists():
        raise FileNotFoundError(f"input_dir does not exist: {input_dir}")
    output_dir.mkdir(parents=True, exist_ok=True)

    txt_files = [p for p in input_dir.iterdir() if p.suffix == ".txt" and p.stem.isdigit()]
    txt_files.sort(key=lambda p: int(p.stem))
    if not txt_files:
        print(f"[WARN] No numeric .txt prompts found in {input_dir}")
        return 0

    for txt_path in txt_files:
        demo_id = txt_path.stem
        print(f"\n[DEBUG] ===== Processing demo_{demo_id} =====")
        prompt = read_prompt_text(txt_path)
        if not prompt:
            print(f"[WARN] Empty prompt, skipping: {txt_path.name}")
            continue

        scene_analyser, _nodes = build_graph(runtime, workflow, llm_profile=args.llm_profile)
        print("[DEBUG] Graph rebuilt for this demo")
        demo_output_dir = output_dir / f"demo_{demo_id}"
        demo_output_dir.mkdir(parents=True, exist_ok=True)
        initial_state = GraphState(messages=[HumanMessage(content=prompt)], save_dir=str(demo_output_dir))
        print("[DEBUG] Start Graph Execution")
        final_state: GraphState = scene_analyser.invoke(initial_state)

        if post_actions.get("copy_dirs", True):
            copy_all_dirs_to_output(demo_output_dir, copy_dirs)
        if post_actions.get("copy_prompt_to_eval", True):
            copy_prompt_to_eval(txt_path, demo_output_dir)
        save_llm_outputs(final_state, demo_output_dir, llm_outputs_dir_name)

        if run_runtime:
            runtime_summary = run_runtime_validation_for_demo(demo_output_dir)
            print(f"[DEBUG] runtime validation result for demo_{demo_id}: {runtime_summary.get('result')}")
            if runtime_summary.get("result") != "pass":
                runtime_failures.append({"demo_id": demo_id, "errors": runtime_summary.get("errors", [])})

        finish_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        log_line = f"demo_{demo_id} finished at {finish_time}\n"
        print(f"[DEBUG] {log_line.strip()}")
        DEMO_FINISH_LOG_PATH.open("a", encoding="utf-8").write(log_line)
        print(f"[DEBUG] demo_{demo_id} output completed")

    if post_actions.get("render_gltf", False) and not args.skip_render:
        from model_description.batch_render_gltf import render_gltf
        render_gltf()
    if runtime_failures:
        print(json.dumps({"runtime_validation": "fail", "failures": runtime_failures}, ensure_ascii=False, indent=2))
        return 1
    return 0


def parse_args():
    parser = argparse.ArgumentParser(description="Run AutoUE workflow with config-driven paths, LLM profiles, nodes, and prompts.")
    parser.add_argument("--config", help="Runtime config JSON. Defaults to config/local.json if present, else local.example.json.")
    parser.add_argument("--workflow", help="Workflow JSON. Defaults to config value or AUTOUE_WORKFLOW.")
    parser.add_argument("--llm-profile", help="Override runtime LLM profile.")
    parser.add_argument("--input-dir", help="Override input prompt directory.")
    parser.add_argument("--output-dir", help="Override output directory.")
    parser.add_argument("--dry-run-config", action="store_true", help="Validate config/workflow/prompt wiring without calling LLM.")
    parser.add_argument("--run-runtime-validation", action="store_true", help="Run Phase3 Python runtime validation after each demo output is written.")
    parser.add_argument("--skip-render", action="store_true", help="Do not run optional GLTF rendering post-action.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.dry_run_config:
        return dry_run_config(args)
    return run_workflow(args)


if __name__ == "__main__":
    raise SystemExit(main())
