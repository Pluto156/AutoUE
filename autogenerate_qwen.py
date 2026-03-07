import os
import shutil
from dotenv import load_dotenv
load_dotenv()
from datetime import datetime
from typing import Any, Dict, List
from dataclasses import dataclass, field

from langchain_community.chat_models import ChatTongyi

from langgraph.graph import StateGraph
from langchain_core.messages import HumanMessage

from core.BaseLLMGraph import BaseLLMGraph
from core.BaseLLMNode import GraphState

from custom_nodes.scene_and_gameplay_splitter import create_scene_and_gameplay_splitter
from custom_nodes.scene_formalizer import create_scene_formalizer
from custom_nodes.key_element_extractor import create_key_element_extractor
from custom_nodes.retrieve_model import create_retrieve_model_node
from custom_nodes.module_analyzer import create_module_analyzer
from custom_nodes.module_code_generator import create_module_code_generator
from custom_nodes.interactive_object_analyzer import create_interactive_object_analyzer
from custom_nodes.interactive_object_code_generator import create_interactive_object_code_generator
from custom_nodes.pcg_graph_composer import create_pcg_graph_composer
from custom_nodes.pcg_planner import create_pcg_planner
from custom_nodes.evaluate_instruction_generator import create_evaluate_instruction_generator
from model_description.batch_render_gltf import render_gltf


# =========================
# Path configuration
# =========================
INPUT_DIR = os.getenv("INPUT_DIR") 
OUTPUT_DIR = os.getenv("OUTPUT_DIR")
COPY_DIRS = os.getenv("COPY_DIRS", "").split(";")

# =========================
# Demo finish log file
# =========================
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEMO_FINISH_LOG_PATH = os.path.join(SCRIPT_DIR, "demo_finish_log.txt")


# =========================
# Robust txt reader
# =========================
def read_prompt_text(txt_path: str) -> str:
    with open(txt_path, "rb") as f:
        raw = f.read()

    if raw.startswith(b"\xff\xfe"):
        text = raw.decode("utf-16-le")
    elif raw.startswith(b"\xfe\xff"):
        text = raw.decode("utf-16-be")
    else:
        text = raw.decode("utf-8", errors="ignore")

    return text.replace("\ufeff", "").strip()


# =========================
# Copy all required directories
# =========================
def copy_all_dirs_to_output(demo_output_dir: str):
    for src_dir in COPY_DIRS:
        if not os.path.exists(src_dir):
            print(f"[WARN] Source directory does not exist, skipping: {src_dir}")
            continue

        dst_dir = os.path.join(demo_output_dir, os.path.basename(src_dir))
        print(f"[DEBUG] Copying {src_dir} -> {dst_dir}")

        if os.path.exists(dst_dir):
            shutil.rmtree(dst_dir)

        shutil.copytree(src_dir, dst_dir)


# =========================
# Copy prompt txt to MyPCG/eval/Prompt.txt
# =========================
def copy_prompt_to_eval(txt_path: str, demo_output_dir: str):
    eval_dir = os.path.join(demo_output_dir, "MyPCG", "eval")
    os.makedirs(eval_dir, exist_ok=True)

    target_path = os.path.join(eval_dir, "Prompt.txt")

    if os.path.exists(target_path):
        os.remove(target_path)

    shutil.copy(txt_path, target_path)
    print(f"[DEBUG] Prompt copied to {target_path}")


# =========================
# Save llm_outputs
# =========================
def save_llm_outputs(graph_state: GraphState, demo_output_dir: str):
    llm_outputs = getattr(graph_state, "llm_outputs", None)
    if not llm_outputs:
        print("[WARN] No llm_outputs found in GraphState")
        return

    output_dir = os.path.join(demo_output_dir, "llm_outputs")
    os.makedirs(output_dir, exist_ok=True)

    for key, value in llm_outputs.items():
        file_path = os.path.join(output_dir, f"{key}.txt")
        with open(file_path, "w", encoding="utf-8") as f:
            f.write(value)

        print(f"[DEBUG] Saved llm_output: {file_path}")


# =========================
# Build Graph
# =========================
def build_graph():
    model = ChatTongyi(
        model="qwen-plus",
        temperature=0.2
    )

    scene_and_gameplay_splitter = create_scene_and_gameplay_splitter()
    scene_formalizer = create_scene_formalizer()
    key_element_extractor = create_key_element_extractor()
    retrieve_model_node = create_retrieve_model_node()
    module_analyzer = create_module_analyzer()
    module_code_generator = create_module_code_generator()
    interactive_object_analyzer = create_interactive_object_analyzer()
    interactive_object_code_generator = create_interactive_object_code_generator()
    pcg_graph_composer = create_pcg_graph_composer()
    pcg_planner = create_pcg_planner()
    evaluate_instruction_generator = create_evaluate_instruction_generator()

    baseLLMGraph = BaseLLMGraph(model=model)
    (
        baseLLMGraph
        .AddNode(scene_and_gameplay_splitter)
        .AddNode(scene_formalizer)
        .AddNode(key_element_extractor)
        .AddNode(retrieve_model_node)
        .AddNode(module_analyzer)
        .AddNode(module_code_generator)
        .AddNode(interactive_object_analyzer)
        .AddNode(interactive_object_code_generator)
        .AddNode(pcg_graph_composer)
        .AddNode(pcg_planner)
        .AddNode(evaluate_instruction_generator)
    )

    graph = StateGraph(GraphState, name="SceneAnalyserGraph")

    graph.add_node(scene_and_gameplay_splitter.name, scene_and_gameplay_splitter.execute)
    graph.add_node(scene_formalizer.name, scene_formalizer.execute)
    graph.add_node(key_element_extractor.name, key_element_extractor.execute)
    graph.add_node(retrieve_model_node.name, retrieve_model_node.execute)
    graph.add_node(module_analyzer.name, module_analyzer.execute)
    graph.add_node(module_code_generator.name, module_code_generator.execute)
    graph.add_node(interactive_object_analyzer.name, interactive_object_analyzer.execute)
    graph.add_node(interactive_object_code_generator.name, interactive_object_code_generator.execute)
    graph.add_node(pcg_graph_composer.name, pcg_graph_composer.execute)
    graph.add_node(pcg_planner.name, pcg_planner.execute)
    graph.add_node(evaluate_instruction_generator.name, evaluate_instruction_generator.execute)

    graph.set_entry_point(scene_and_gameplay_splitter.name)

    return graph.compile()

# =========================
# main
# =========================
def main():
    
    if not os.getenv("DASHSCOPE_API_KEY"):
        raise RuntimeError("DASHSCOPE_API_KEY not found")

    print("[DEBUG] Qwen API key loaded")

    txt_files = [
        f for f in os.listdir(INPUT_DIR)
        if f.endswith(".txt") and f[:-4].isdigit()
    ]
    txt_files.sort(key=lambda x: int(x[:-4]))

    for txt_file in txt_files:
        demo_id = txt_file[:-4]
        txt_path = os.path.join(INPUT_DIR, txt_file)

        print(f"\n[DEBUG] ===== Processing demo_{demo_id} =====")

        prompt = read_prompt_text(txt_path)
        if not prompt:
            print(f"[WARN] Empty prompt, skipping: {txt_file}")
            continue

        scene_analyser = build_graph()
        print("[DEBUG] Graph rebuilt for this demo")

        initial_state = GraphState(
            messages=[HumanMessage(content=prompt)]
        )

        print("[DEBUG] Start Graph Execution")
        final_state: GraphState = scene_analyser.invoke(initial_state)

        demo_output_dir = os.path.join(OUTPUT_DIR, f"demo_{demo_id}")
        os.makedirs(demo_output_dir, exist_ok=True)

        copy_all_dirs_to_output(demo_output_dir)
        copy_prompt_to_eval(txt_path, demo_output_dir)

        save_llm_outputs(final_state, demo_output_dir)

        finish_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        log_line = f"demo_{demo_id} finished at {finish_time}\n"

        print(f"[DEBUG] ⏱ {log_line.strip()}")

        with open(DEMO_FINISH_LOG_PATH, "a", encoding="utf-8") as f:
            f.write(log_line)

        print(f"[DEBUG] demo_{demo_id} output completed")

    render_gltf()


if __name__ == "__main__":
    main()
