from __future__ import annotations

import json
import os
import re
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Mapping

try:
    import tomllib
except Exception:  # pragma: no cover
    tomllib = None  # type: ignore[assignment]

@dataclass
class MCPServerConfig:
    command: str
    args: list[str]
    env: dict[str, str]
    timeout_sec: int = 120

class MCPClientError(RuntimeError):
    pass


def _from_codex_config(server_name: str = "ue-api-search") -> MCPServerConfig | None:
    if tomllib is None:
        return None
    config_path = Path(os.getenv("CODEX_CONFIG", Path.home() / ".codex" / "config.toml"))
    if not config_path.exists():
        return None
    data = tomllib.loads(config_path.read_text(encoding="utf-8"))
    server = data.get("mcp_servers", {}).get(server_name)
    if not isinstance(server, dict):
        return None
    command = str(server.get("command", "")).strip()
    if not command:
        return None
    return MCPServerConfig(
        command=command,
        args=[str(x) for x in server.get("args", [])],
        env={str(k): str(v) for k, v in dict(server.get("env", {})).items()},
        timeout_sec=int(server.get("startup_timeout_sec", 120) or 120),
    )


def load_ue_api_mcp_config(runtime_config: Mapping[str, Any] | None = None) -> MCPServerConfig:
    runtime_config = runtime_config or {}
    raw = runtime_config.get("mcp", {}).get("ue_api_search", {}) if isinstance(runtime_config.get("mcp", {}), Mapping) else {}
    command = os.getenv("AUTOUE_UE_API_MCP_COMMAND", "") or str(raw.get("command", "")).strip()
    args = json.loads(os.getenv("AUTOUE_UE_API_MCP_ARGS", "[]")) if os.getenv("AUTOUE_UE_API_MCP_ARGS", "") else [str(x) for x in raw.get("args", [])]
    env = {str(k): str(v) for k, v in dict(raw.get("env", {})).items()} if isinstance(raw.get("env", {}), Mapping) else {}
    if os.getenv("AUTOUE_UE_API_MCP_ENV_JSON", ""):
        env.update({str(k): str(v) for k, v in json.loads(os.getenv("AUTOUE_UE_API_MCP_ENV_JSON", "{}")).items()})
    timeout = int(os.getenv("AUTOUE_UE_API_MCP_TIMEOUT_SEC", "") or raw.get("timeout_sec", 0) or 0)
    if command:
        return MCPServerConfig(command=command, args=args, env=env, timeout_sec=timeout or 120)
    codex = _from_codex_config()
    if codex:
        return codex
    raise MCPClientError("UE API MCP server is not configured. Configure AUTOUE_UE_API_MCP_COMMAND or [mcp_servers.ue-api-search] in ~/.codex/config.toml.")


class StdioMCPClient:
    """Minimal MCP stdio client for the Python MCP SDK used by ue-api-search.

    The installed MCP SDK transports JSON-RPC as one UTF-8 JSON object per line.
    This intentionally does not use Content-Length framing.
    """

    def __init__(self, config: MCPServerConfig):
        self.config = config
        self.proc: subprocess.Popen[str] | None = None
        self._next_id = 1

    def __enter__(self) -> "StdioMCPClient":
        env = os.environ.copy()
        env.update(self.config.env)
        self.proc = subprocess.Popen(
            [self.config.command, *self.config.args],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=env,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        self.request("initialize", {"protocolVersion": "2024-11-05", "capabilities": {}, "clientInfo": {"name": "autoue-phase2", "version": "0.1"}})
        self.notify("notifications/initialized", {})
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        if not self.proc:
            return
        try:
            if self.proc.stdin:
                self.proc.stdin.close()
            self.proc.terminate()
            self.proc.wait(timeout=5)
        except Exception:
            try:
                self.proc.kill()
            except Exception:
                pass
        self.proc = None

    def _send(self, payload: Mapping[str, Any]) -> None:
        if not self.proc or not self.proc.stdin:
            raise MCPClientError("MCP process is not running")
        self.proc.stdin.write(json.dumps(payload, ensure_ascii=False) + "\n")
        self.proc.stdin.flush()

    def _read_message(self) -> dict[str, Any]:
        if not self.proc or not self.proc.stdout:
            raise MCPClientError("MCP process is not running")
        deadline = time.monotonic() + self.config.timeout_sec
        while True:
            if self.proc.poll() is not None:
                stderr = self.proc.stderr.read() if self.proc.stderr else ""
                raise MCPClientError(f"MCP server exited with code {self.proc.returncode}: {stderr}")
            if time.monotonic() > deadline:
                raise MCPClientError("timed out waiting for MCP response line")
            line = self.proc.stdout.readline()
            if not line:
                time.sleep(0.01)
                continue
            line = line.strip()
            if not line:
                continue
            try:
                return json.loads(line)
            except Exception as exc:
                raise MCPClientError(f"invalid MCP JSON line: {line[:200]}") from exc

    def request(self, method: str, params: Mapping[str, Any] | None = None) -> dict[str, Any]:
        request_id = self._next_id
        self._next_id += 1
        self._send({"jsonrpc": "2.0", "id": request_id, "method": method, "params": dict(params or {})})
        while True:
            message = self._read_message()
            if message.get("id") != request_id:
                continue
            if "error" in message:
                raise MCPClientError(f"MCP request {method} failed: {message['error']}")
            return message.get("result", {})

    def notify(self, method: str, params: Mapping[str, Any] | None = None) -> None:
        self._send({"jsonrpc": "2.0", "method": method, "params": dict(params or {})})

    def call_tool(self, name: str, arguments: Mapping[str, Any]) -> dict[str, Any]:
        return self.request("tools/call", {"name": name, "arguments": dict(arguments)})


def extract_mcp_tool_payload(result: Mapping[str, Any]) -> Any:
    if "structuredContent" in result:
        return result["structuredContent"]
    content = result.get("content")
    if isinstance(content, list) and content:
        first = content[0]
        if isinstance(first, Mapping) and first.get("type") == "text":
            text = str(first.get("text", ""))
            try:
                return json.loads(text)
            except Exception:
                return text
    return result


def call_ue_api_semantic_search(query: str, *, limit: int = 5, runtime_config: Mapping[str, Any] | None = None) -> dict[str, Any]:
    config = load_ue_api_mcp_config(runtime_config)
    with StdioMCPClient(config) as client:
        result = client.call_tool("ue_api_semantic_search", {"query": query, "limit": limit})
    return {"tool": "ue_api_semantic_search", "query": query, "limit": limit, "raw_result": result, "payload": extract_mcp_tool_payload(result)}


def safe_engine_port_filename(engine_port_id: str) -> str:
    safe = re.sub(r"[^A-Za-z0-9_.-]+", "_", engine_port_id.strip()).strip("._-")
    return (safe or "engine_port")[:120]
