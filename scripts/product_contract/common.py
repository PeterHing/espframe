"""Shared helpers for product contract validators."""

from __future__ import annotations

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent.parent
WEB_SRC_DIR = ROOT / "docs" / "webserver" / "src"
WEB_TEMPLATE = ROOT / "docs" / "webserver" / "src" / "app.template.js"
WEB_APP = ROOT / "docs" / "public" / "webserver" / "app.js"
TIME_YAML = ROOT / "common" / "addon" / "time.yaml"


def rel(path: Path) -> str:
    return str(path.relative_to(ROOT))


def read(path: Path, errors: list[str]) -> str:
    if not path.is_file():
        errors.append(f"Missing file: {rel(path)}")
        return ""
    return path.read_text()


def read_web_source(errors: list[str]) -> str:
    files = [WEB_TEMPLATE] + sorted(
        path for path in WEB_SRC_DIR.glob("*.js")
        if path.name != WEB_TEMPLATE.name
    )
    return "\n".join(read(path, errors) for path in files)


def require_contains(text: str, needle: str, label: str, errors: list[str]) -> None:
    if needle not in text:
        errors.append(f"{label} is missing {needle!r}")


def extract_js_json_var(text: str, var_name: str, errors: list[str]) -> object | None:
    match = re.search(rf"\bvar {re.escape(var_name)} = (.*?);", text)
    if not match:
        errors.append(f"Generated web app is missing {var_name}")
        return None
    try:
        return json.loads(match.group(1))
    except json.JSONDecodeError as exc:
        errors.append(f"Generated web app {var_name} is not valid JSON: {exc}")
        return None


def valid_entity_string(value: object) -> bool:
    if not isinstance(value, str) or "/" not in value:
        return False
    domain, name = value.split("/", 1)
    return bool(domain.strip() and name.strip())


def check_web_entity_default_type(metadata: dict, domain: str, label: str, errors: list[str]) -> None:
    if "default" not in metadata:
        return
    value = metadata.get("default")
    if domain == "switch":
        if not isinstance(value, bool):
            errors.append(f"{label} switch default must be true or false")
    elif domain == "number":
        if not isinstance(value, (int, float)) or isinstance(value, bool):
            errors.append(f"{label} number default must be numeric")
    elif domain in {"select", "text", "text_sensor"}:
        if not isinstance(value, str):
            errors.append(f"{label} {domain} default must be a string")
