# AutoUE: Automated 3D Game Generation in Unreal Engine via Multi-Agent Systems

AutoUE is an end-to-end multi-agent system for automatically generating complete, playable 3D games in Unreal Engine from natural language descriptions. It coordinates multiple specialized agents to construct coherent scenes, synthesize robust gameplay and interaction code, and perform automated play-testing within the engine.

This repository contains the anonymous implementation accompanying our paper submission.

---

# Demo Video

<video src="https://raw.githubusercontent.com/Pluto156/AutoUE/main/assets/show.mp4" controls="controls" width="100%" style="max-width: 960px;">
  Your browser does not support the video tag.
</video>

---

# Overview

Automatically generating a complete 3D game inside a commercial engine is a complex workflow problem. It requires:

- Retrieving and organizing large-scale 3D assets  
- Constructing spatially coherent scenes  
- Generating modular, compilable C++ gameplay code  
- Implementing interactive object logic  
- Testing dynamic runtime behaviors  

The system decomposes a game description into:

- **Scene Description (DescS)** – environment layout and objects  
- **Gameplay Description (DescG)** – mechanics, interactions, and objectives  

These are processed by five coordinated agents.

All benchmark data, including 20 generated demo games, model embedding index vectors, and experimental data, as well as associated 3D assets used in our experiments, are publicly available at: [AutoUE_DataSet](https://huggingface.co/datasets/Pluto156/AutoUE_DataSet)

---

# Setup and Usage

## 1. Install Dependencies

Clone the repository and install all required Python dependencies from `requirements.txt`.

```bash
pip install -r requirements.txt
```

It is recommended to install dependencies inside a virtual environment such as `conda` or `venv`.

Example:

```bash
conda create -n autoue python=3.10
conda activate autoue
pip install -r requirements.txt
```

---

## 2. Configure API Keys

The system relies on several external services (such as LLM APIs).  
Find the `.env` file in the project root directory and configure the required API keys.

Example `.env` file:

```env
OPENAI_API_KEY=your_openai_api_key
DASHSCOPE_API_KEY=your_dashscope_api_key
OTHER_SERVICE_KEY=your_service_key
```

Make sure all required service keys are correctly configured before running the system.

---

## 3. Run AutoUE

After installing dependencies and configuring the `.env` file, run the main script:

```bash
python autogenerate_qwen.py
```


# Notes

- Ensure that all required APIs are accessible from your environment.
- Some generation steps may take time depending on the complexity of the game description.
- The system is designed to work with Unreal Engine pipelines and may require Unreal Engine related integrations.

---
