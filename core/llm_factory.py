from __future__ import annotations

import json
import os
import shutil
import subprocess
import tempfile
import urllib.request
from dataclasses import dataclass
from typing import Any, Dict, Iterable, Mapping


@dataclass
class SimpleChatResponse:
    content: str
    response_metadata: Dict[str, Any]


class OpenAICompatibleChatModel:
    """Tiny OpenAI-compatible chat wrapper used to avoid hard-wiring one vendor."""

    def __init__(self, *, api_key: str, base_url: str, model: str, temperature: float = 0.2, timeout: int = 120):
        self.api_key = api_key
        self.base_url = base_url.rstrip("/")
        self.model = model
        self.temperature = temperature
        self.timeout = timeout

    def _normalize_messages(self, messages: Iterable[Any]) -> list[dict[str, str]]:
        result = []
        for msg in messages:
            if isinstance(msg, Mapping):
                result.append({"role": str(msg.get("role", "user")), "content": str(msg.get("content", ""))})
            else:
                role = getattr(msg, "type", None) or getattr(msg, "role", None) or "user"
                if role == "human":
                    role = "user"
                if role == "ai":
                    role = "assistant"
                result.append({"role": str(role), "content": str(getattr(msg, "content", msg))})
        return result

    def invoke(self, messages: Iterable[Any]) -> SimpleChatResponse:
        payload = {"model": self.model, "messages": self._normalize_messages(messages), "temperature": self.temperature}
        body = json.dumps(payload).encode("utf-8")
        request = urllib.request.Request(
            f"{self.base_url}/chat/completions",
            data=body,
            headers={"Content-Type": "application/json", "Authorization": f"Bearer {self.api_key}"},
            method="POST",
        )
        with urllib.request.urlopen(request, timeout=self.timeout) as response:
            data = json.loads(response.read().decode("utf-8"))
        content = data.get("choices", [{}])[0].get("message", {}).get("content", "")
        return SimpleChatResponse(content=content, response_metadata={"token_usage": data.get("usage", {})})


class CodexCliChatModel:
    """LangChain-like wrapper around `codex exec`.

    This is the real LLM path for local AutoUE adaptation. It intentionally uses
    Codex CLI instead of a direct HTTP client so model/provider/auth routing stays
    in the user's current Codex runtime.
    """

    def __init__(self, *, command: str = "codex", model: str = "", cwd: str = ".", timeout: int = 900, reasoning_effort: str = ""):
        self.command = shutil.which(command) or command
        self.model = model
        self.cwd = cwd
        self.timeout = timeout
        self.reasoning_effort = reasoning_effort

    def _normalize_messages(self, messages: Iterable[Any]) -> tuple[str, str]:
        system_parts = []
        user_parts = []
        for msg in messages:
            if isinstance(msg, Mapping):
                role = str(msg.get("role", "user"))
                content = str(msg.get("content", ""))
            else:
                role = getattr(msg, "type", None) or getattr(msg, "role", None) or "user"
                if role == "human":
                    role = "user"
                if role == "ai":
                    role = "assistant"
                content = str(getattr(msg, "content", msg))
            if role == "system":
                system_parts.append(content)
            else:
                user_parts.append(f"[{role}]\n{content}")
        return "\n\n".join(system_parts), "\n\n".join(user_parts)

    def invoke(self, messages: Iterable[Any]) -> SimpleChatResponse:
        system_text, user_text = self._normalize_messages(messages)
        prompt = (
            "You are running as an LLM subcall inside the AutoUE Python workflow.\n"
            "Return only the requested final artifact. Do not run shell commands.\n"
            "Do not add explanations unless the task explicitly asks for them.\n\n"
            f"<system_prompt>\n{system_text}\n</system_prompt>\n\n"
            f"<workflow_input>\n{user_text}\n</workflow_input>\n"
        )
        with tempfile.TemporaryDirectory(prefix="autoue-codex-") as tmp:
            out_path = os.path.join(tmp, "last-message.txt")
            cmd = [
                self.command,
                "exec",
                "--skip-git-repo-check",
                "--ephemeral",
                "--sandbox",
                "read-only",
                "--color",
                "never",
                "--output-last-message",
                out_path,
            ]
            if self.model:
                cmd.extend(["--model", self.model])
            if self.reasoning_effort:
                cmd.extend(["-c", f"model_reasoning_effort=\"{self.reasoning_effort}\""])
            if self.cwd:
                cmd.extend(["-C", self.cwd])
            cmd.append("-")
            proc = subprocess.run(
                cmd,
                input=prompt,
                text=True,
                encoding="utf-8",
                errors="replace",
                capture_output=True,
                timeout=self.timeout,
            )
            if proc.returncode != 0:
                raise RuntimeError(
                    "codex exec failed with exit code "
                    f"{proc.returncode}\nSTDOUT:\n{proc.stdout}\nSTDERR:\n{proc.stderr}"
                )
            if os.path.exists(out_path):
                content = open(out_path, "r", encoding="utf-8", errors="replace").read().strip()
            else:
                content = proc.stdout.strip()
            return SimpleChatResponse(
                content=content,
                response_metadata={"token_usage": {}, "provider": "codex_cli", "model": self.model or "codex-default"},
            )



class ScriptedSmokeChatModel:
    """Deterministic local model for workflow smoke tests; never calls network."""

    def invoke(self, messages: Iterable[Any]) -> SimpleChatResponse:
        normalized = OpenAICompatibleChatModel(api_key="x", base_url="http://127.0.0.1", model="x")._normalize_messages(messages)
        system = "\n".join(m["content"] for m in normalized if m["role"] == "system")
        port_input = "input.action_binding"
        port_damage = "damage.apply"
        raw_input = "flow/04-ue-api-mcp/raw/input.action_binding.raw.json"
        raw_damage = "flow/04-ue-api-mcp/raw/damage.apply.raw.json"
        adj_input = "flow/04-ue-api-mcp/adjudication/input.action_binding.json"
        adj_damage = "flow/04-ue-api-mcp/adjudication/damage.apply.json"
        runtime_mapping_path = "flow/05-puerts-runtime-mapping.json"
        trace = {
            "entity_id": "player",
            "ability_id": "player.combat",
            "behavior_id": "player.combat.attack",
            "flow_id": "flow_player_combat_attack",
            "engine_port_ids": [port_input, port_damage],
            "adjudication_paths": [adj_input, adj_damage],
            "runtime_mapping_path": runtime_mapping_path,
            "ts_files": ["TypeScript/content/generated/interactive/SmokeInteractable.ts", "TypeScript/content/generated/SmokeGame.ts"],
        }
        if "SCHEMA: EvaluateInstructionGenerator" in system:
            content = json.dumps({
                "evaluation_instructions": [
                    {
                        "step_id": 1,
                        "action": "attack",
                        "target": "Enemy",
                        "description": "Statically validate the scripted smoke adapter_call trace without launching PIE.",
                        "driver": "adapter_call",
                        "executor_action": "call_behavior",
                        "expected": [
                            {"type": "static_trace_present", "key": "ability_module_export", "expected_value": "tickSmokeGame"},
                            {"type": "static_trace_present", "key": "interactive_adapter_export", "expected_value": "runSmokeInteraction"},
                            {"type": "static_trace_present", "key": "engine_ports_mapped", "expected_value": [port_input, port_damage]},
                        ],
                        "trace": trace,
                    },
                ],
                "coverage": [trace],
            })
        elif "SCHEMA: TypeScriptCodeGenerator" in system:
            base = {"entity_id": "player", "behavior_id": "player.combat.attack", "flow_id": "flow_player_combat_attack", "runtime_mapping_path": runtime_mapping_path, "action_label": "attacks", "target_label": "Enemy", "result_label": "enemy defeated"}
            support = [
                {"template": "aid_runtime_orchestrator", "path": "TypeScript/content/generated/AutoUEGeneratedRuntime.ts", "export_name": "runAutoUEGeneratedRuntime", "interface_name": "AutoUEGeneratedRuntimeContext"},
                {"template": "aid_character_adapter", "path": "TypeScript/AutoUEGeneratedCharacterAdapter.ts", "export_name": "AutoUEGeneratedCharacterAdapter", "interface_name": "AutoUEGeneratedCharacterAdapterContext"},
                {"template": "aid_gamemode_adapter", "path": "TypeScript/AutoUEGeneratedGameModeAdapter.ts", "export_name": "AutoUEGeneratedGameModeAdapter", "interface_name": "AutoUEGeneratedGameModeAdapterContext"},
                {"template": "aid_camera_setup", "path": "TypeScript/content/generated/AutoUEGeneratedCameraHelper.ts", "export_name": "setupAutoUEGeneratedCamera", "interface_name": "AutoUEGeneratedCameraOptions"},
                {"template": "scene_manifest_helper", "path": "TypeScript/content/generated/AutoUEGeneratedSceneManifest.ts", "export_name": "getAutoUEGeneratedSceneManifest", "interface_name": "AutoUEGeneratedSceneManifestContext"},
            ]
            template_inputs = [{"template": "ability_module", "path": "TypeScript/content/generated/SmokeGame.ts", **base, "export_name": "tickSmokeGame", "interface_name": "SmokeGameContext"}]
            template_inputs.extend({**item, **base} for item in support)
            content = json.dumps({
                "template_inputs": template_inputs,
                "behavior_traces": [{"entity_id": "player", "behavior_id": "player.combat.attack", "flow_id": "flow_player_combat_attack", "runtime_mapping_path": runtime_mapping_path, "file_path": "TypeScript/content/generated/SmokeGame.ts", "export_name": "tickSmokeGame"}],
                "consumed_interactive_files": ["TypeScript/content/generated/interactive/SmokeInteractable.ts"],
                "validation_notes": ["scripted smoke runtime and AIDev bridge templates"],
            })
        elif "SCHEMA: TypeScriptInteractiveObjectGenerator" in system:
            content = json.dumps({
                "template_inputs": [{"template": "interactive_object", "path": "TypeScript/content/generated/interactive/SmokeInteractable.ts", "entity_id": "player", "behavior_id": "player.combat.attack", "flow_id": "flow_player_combat_attack", "runtime_mapping_path": runtime_mapping_path, "export_name": "runSmokeInteraction", "interface_name": "SmokeInteractionContext", "action_label": "attacks", "target_label": "Enemy", "result_label": "enemy defeated"}],
                "behavior_traces": [{"entity_id": "player", "behavior_id": "player.combat.attack", "flow_id": "flow_player_combat_attack", "runtime_mapping_path": runtime_mapping_path, "file_path": "TypeScript/content/generated/interactive/SmokeInteractable.ts", "export_name": "runSmokeInteraction"}],
                "validation_notes": ["scripted smoke interactive object template"],
            })
        elif "SCHEMA: TypeScriptScriptAnalyzer" in system:
            content = json.dumps({
                "typescript_sources": [{"path": "TypeScript/content/generated/SmokeGame.ts", "role": "ability", "notes": "runtime_owner from smoke mapping"}],
                "implementation_slots": [{"entity_id": "player", "behavior_id": "player.combat.attack", "flow_id": "flow_player_combat_attack", "runtime_mapping_path": runtime_mapping_path, "target_ts_file": "TypeScript/content/generated/SmokeGame.ts", "reason": "scripted smoke runtime mapping"}],
                "missing_slots": [],
            })
        elif "SCHEMA: PuerTSRuntimeMappingPlanner" in system:
            content = json.dumps({
                "runtime_mapping_path": runtime_mapping_path,
                "mappings": [{
                    "entity_id": "player",
                    "ability_id": "player.combat",
                    "behavior_id": "player.combat.attack",
                    "flow_id": "flow_player_combat_attack",
                    "runtime_owner": "TypeScript/content/generated/SmokeGame.ts",
                    "implementation_carrier": "template_rendered_ts",
                    "selected_runtime_owner": "SmokeGameAbility",
                    "existing_framework_candidates": ["TypeScriptCodeGenerator templates", "AIDev TypeScript Blueprint adapter"],
                    "why_not_existing_framework": "scripted smoke uses generated templates to prove the bridge contract",
                    "temporary_or_canonical": "temporary",
                    "migration_path": "replace smoke runtime with canonical generated AIDev bridge after Phase3",
                    "engine_port_mappings": [
                        {"engine_port_id": port_input, "adjudication_path": adj_input, "adapter_or_helper": "CharacterAdapter.bindInput", "verdict": "hit", "evidence_symbols": ["UE.EnhancedInputComponent.BindAction"]},
                        {"engine_port_id": port_damage, "adjudication_path": adj_damage, "adapter_or_helper": "RuntimePorts.applyDamage", "verdict": "hit", "evidence_symbols": ["UE.GameplayStatics.ApplyDamage"]},
                    ],
                    "thin_contracts": ["bind attack input", "apply damage to enemy"],
                    "ability_binding": "adapter_call:tickSmokeGame",
                    "verification_evidence": ["adapter_call can invoke generated ability", "MCP adjudications exist for required ports"],
                }],
                "blocked_mappings": [],
            })
        elif "SCHEMA: UEApiMCPFeasibilitySearcher" in system:
            content = json.dumps({
                "queries": [
                    {"engine_port_id": port_input, "flow_ids": ["flow_player_combat_attack"], "behavior_ids": ["player.combat.attack"], "query": "Unreal Engine PuerTS gameplay API for input.action_binding: bind player attack input", "raw_path": raw_input, "adjudication_path": adj_input, "verdict": "hit", "hit_type": "direct_hit", "evidence_symbols": ["UE.EnhancedInputComponent.BindAction"], "notes": "MCP fixture returns an input binding symbol."},
                    {"engine_port_id": port_damage, "flow_ids": ["flow_player_combat_attack"], "behavior_ids": ["player.combat.attack"], "query": "Unreal Engine PuerTS gameplay API for damage.apply: apply damage to enemy", "raw_path": raw_damage, "adjudication_path": adj_damage, "verdict": "hit", "hit_type": "direct_hit", "evidence_symbols": ["UE.GameplayStatics.ApplyDamage"], "notes": "MCP fixture returns a damage application symbol."},
                ],
                "summary": {"all_required_ports_hit": True, "blocked_engine_ports": []},
            })
        elif "SCHEMA: ThinGameplayFlowPlanner" in system:
            content = json.dumps({
                "flows": [{
                    "flow_id": "flow_player_combat_attack",
                    "entity_id": "player",
                    "ability_id": "player.combat",
                    "source_behavior_id": "player.combat.attack",
                    "stages": [
                        {"stage": "Input", "contract": "bind player attack input", "inputs": ["attack input"], "outputs": ["attack requested"], "engine_ports": [port_input]},
                        {"stage": "Damage/Resource", "contract": "apply damage to enemy", "inputs": ["enemy target"], "outputs": ["enemy defeated"], "engine_ports": [port_damage]},
                    ],
                    "verification": ["enemy state becomes defeated"],
                }]
            })
        elif "SCHEMA: EntityAbilityBehaviorPlanner" in system:
            content = json.dumps({
                "entities": [{"entity_id": "player", "display_name": "Player", "summary": "Playable character", "abilities": [{"ability_id": "player.combat", "display_name": "Combat", "summary": "Attack behavior", "behaviors": [{"behavior_id": "player.combat.attack", "display_name": "Attack Enemy", "trigger": "attack input", "execution": "strike the enemy", "result": "enemy is defeated", "source_refs": []}]}]}],
                "non_goals": [],
            })
        elif "SCHEMA: SceneAndGameplaySplitter" in system:
            content = json.dumps({"scene_description": "A tiny side-scroller test room.", "gameplay_description": "The player moves, attacks one enemy, collects a reward, and exits."})
        else:
            content = json.dumps({"ok": True, "note": "scripted smoke output"})
        return SimpleChatResponse(content=content, response_metadata={"token_usage": {"input_tokens": 0, "output_tokens": 0}})

def _env_or_default(profile: Mapping[str, Any], env_key: str, default_key: str = "", required: bool = False) -> str:
    env_name = profile.get(env_key)
    value = os.getenv(env_name, "") if env_name else ""
    if not value and default_key:
        value = str(profile.get(default_key, ""))
    if required and not value:
        raise RuntimeError(f"Missing required environment variable: {env_name}")
    return value


def create_llm(profile_name: str, profiles_config: Mapping[str, Any]):
    profiles = profiles_config.get("profiles", {})
    if profile_name not in profiles:
        raise KeyError(f"LLM profile not found: {profile_name}")
    profile = profiles[profile_name]
    provider = profile.get("provider")
    temperature = float(profile.get("temperature", 0.2))
    if provider == "scripted_smoke":
        return ScriptedSmokeChatModel()

    if provider == "codex_cli":
        command = os.getenv(profile.get("command_env", "CODEX_CLI_PATH"), "") or profile.get("command", "codex")
        model = os.getenv(profile.get("model_env", "CODEX_MODEL"), "") or profile.get("default_model", "")
        cwd = os.getenv(profile.get("cwd_env", "CODEX_WORKDIR"), "") or profile.get("cwd", ".")
        timeout = int(os.getenv(profile.get("timeout_env", "CODEX_AGENT_TIMEOUT_SEC"), "") or profile.get("timeout", 900))
        reasoning_effort = os.getenv(profile.get("reasoning_effort_env", "CODEX_REASONING_EFFORT"), "") or profile.get("reasoning_effort", "")
        return CodexCliChatModel(command=command, model=model, cwd=cwd, timeout=timeout, reasoning_effort=reasoning_effort)

    if provider == "tongyi":
        api_env = profile.get("api_key_env", "DASHSCOPE_API_KEY")
        if not os.getenv(api_env, ""):
            raise RuntimeError(f"Missing required environment variable: {api_env}")
        from langchain_community.chat_models import ChatTongyi
        return ChatTongyi(model=profile.get("model", "qwen-plus"), temperature=temperature)
    if provider == "openai_compatible":
        api_key = _env_or_default(profile, "api_key_env", required=True)
        base_url = _env_or_default(profile, "base_url_env", "default_base_url", required=True)
        model = _env_or_default(profile, "model_env", "default_model", required=True)
        return OpenAICompatibleChatModel(api_key=api_key, base_url=base_url, model=model, temperature=temperature)
    raise ValueError(f"Unsupported LLM provider: {provider}")
