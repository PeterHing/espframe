#!/usr/bin/env python3
"""Shared Espframe product metadata loader.

The product JSON is intentionally small and dependency-free so release scripts,
local checks, and CI workflows can all read the same device and setting data.
"""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parent.parent
PRODUCT_PATH = ROOT / "product" / "espframe.json"
DOCS_SETTINGS_TABLE_COLUMNS = {"Control", "Default", "Description", "Format", "Setting", "Type"}


def load_product(path: Path = PRODUCT_PATH) -> dict[str, Any]:
    try:
        data = json.loads(path.read_text())
    except FileNotFoundError as exc:
        raise RuntimeError(f"Product metadata not found: {path}") from exc
    except json.JSONDecodeError as exc:
        raise RuntimeError(f"Product metadata is not valid JSON: {exc}") from exc

    if not isinstance(data.get("project"), dict):
        raise RuntimeError("Product metadata must contain a project object")
    if not isinstance(data.get("devices"), list) or not data["devices"]:
        raise RuntimeError("Product metadata must contain at least one device")
    if not isinstance(data.get("settings"), list):
        raise RuntimeError("Product metadata must contain a settings list")
    return data


def project_value(key: str, default: str = "") -> str:
    value = load_product()["project"].get(key, default)
    return str(value)


def devices_by_slug() -> dict[str, dict[str, Any]]:
    devices = {}
    for device in load_product()["devices"]:
        slug = str(device.get("slug", "")).strip()
        if not slug:
            raise RuntimeError("Every product device needs a slug")
        if slug in devices:
            raise RuntimeError(f"Duplicate product device slug: {slug}")
        devices[slug] = device
    return devices


def public_base_url(product: dict[str, Any] | None = None) -> str:
    data = product if product is not None else load_product()
    return str(data["project"].get("public_base_url", "")).rstrip("/")


def public_url(path: str = "", product: dict[str, Any] | None = None) -> str:
    base_url = public_base_url(product)
    clean_path = path.lstrip("/")
    if not clean_path:
        return f"{base_url}/"
    return f"{base_url}/{clean_path}"


def device_public_manifest_urls(product: dict[str, Any] | None = None) -> dict[str, dict[str, str]]:
    data = product if product is not None else load_product()
    return {
        str(device["slug"]): {
            "stable": public_url(str(device["public_manifest"]), data),
            "beta": public_url(str(device["public_beta_manifest"]), data),
        }
        for device in data["devices"]
    }


def default_public_manifest_urls(product: dict[str, Any] | None = None) -> dict[str, str]:
    data = product if product is not None else load_product()
    first_device = data["devices"][0]
    return device_public_manifest_urls(data)[str(first_device["slug"])]


def build_yaml_stem(build_yaml: str) -> str:
    name = Path(build_yaml).name
    if name.endswith(".factory.yaml"):
        return name[: -len(".factory.yaml")]
    if name.endswith(".yaml"):
        return name[: -len(".yaml")]
    return name


def build_yaml_device_name(build_yaml: str) -> str:
    path = ROOT / build_yaml
    text = path.read_text()
    match = re.search(r'(?m)^  name: "([^"]+)"$', text)
    if not match:
        raise RuntimeError(f"{path.relative_to(ROOT)} is missing substitutions.name")
    return match.group(1)


def release_matrix_devices(product: dict[str, Any] | None = None) -> list[dict[str, str]]:
    data = product if product is not None else load_product()
    result: list[dict[str, str]] = []
    for device in data["devices"]:
        build_yaml = str(device["build_yaml"])
        result.append(
            {
                "slug": str(device["slug"]),
                "yaml": build_yaml_stem(build_yaml),
                "build_name": build_yaml_device_name(build_yaml),
                "chip": str(device["chip"]),
            }
        )
    return result


def settings() -> list[dict[str, Any]]:
    return list(load_product()["settings"])


def docs_settings_tables(product: dict[str, Any] | None = None) -> dict[Path, dict[str, dict[str, Any]]]:
    data = product if product is not None else load_product()
    tables = data["project"].get("docs_settings_tables", {})
    if not isinstance(tables, dict):
        return {}
    return {
        ROOT / str(path): {
            str(block_id): dict(block)
            for block_id, block in table_blocks.items()
            if isinstance(block, dict)
        }
        for path, table_blocks in tables.items()
        if isinstance(table_blocks, dict)
    }


def backup_export_groups(product: dict[str, Any] | None = None) -> list[str]:
    data = product if product is not None else load_product()
    groups = data["project"].get("backup_export_groups", [])
    if not isinstance(groups, list):
        return []
    return [str(group).strip() for group in groups if str(group).strip()]


def backup_export_fields(product: dict[str, Any] | None = None) -> dict[str, list[str]]:
    data = product if product is not None else load_product()
    fields = data["project"].get("backup_export_fields", {})
    if not isinstance(fields, dict):
        return {}
    return {
        str(group).strip(): [str(field).strip() for field in values if str(field).strip()]
        for group, values in fields.items()
        if str(group).strip() and isinstance(values, list)
    }


def backup_field_state_keys(product: dict[str, Any] | None = None) -> dict[str, dict[str, list[str]]]:
    data = product if product is not None else load_product()
    mappings = data["project"].get("backup_field_state_keys", {})
    if not isinstance(mappings, dict):
        return {}

    result: dict[str, dict[str, list[str]]] = {}
    for raw_group, raw_fields in mappings.items():
        group = str(raw_group).strip()
        if not group or not isinstance(raw_fields, dict):
            continue
        result[group] = {}
        for raw_field, raw_state_keys in raw_fields.items():
            field = str(raw_field).strip()
            if not field:
                continue
            if isinstance(raw_state_keys, list):
                state_keys = [str(key).strip() for key in raw_state_keys if str(key).strip()]
            else:
                state_key = str(raw_state_keys).strip()
                state_keys = [state_key] if state_key else []
            result[group][field] = state_keys
    return result


def backup_schema(product: dict[str, Any] | None = None) -> list[dict[str, Any]]:
    data = product if product is not None else load_product()
    fields = backup_export_fields(data)
    state_key_mappings = backup_field_state_keys(data)
    schema: list[dict[str, Any]] = []
    for group in backup_export_groups(data):
        for field in fields.get(group, []):
            schema.append(
                {
                    "group": group,
                    "field": field,
                    "state_keys": state_key_mappings.get(group, {}).get(field, []),
                }
            )
    return schema


def web_settings_metadata(product_settings: list[dict[str, Any]] | None = None) -> dict[str, dict[str, Any]]:
    result: dict[str, dict[str, Any]] = {}
    for setting in product_settings if product_settings is not None else settings():
        entity = setting["entity"]
        key = str(setting["key"])
        result[key] = {
            "entity": f'{entity["domain"]}/{entity["name"]}',
            "domain": entity["domain"],
            "default": setting.get("default", ""),
            "options": setting.get("options", []),
        }
        if setting.get("developer_options"):
            result[key]["developerOptions"] = setting["developer_options"]
        for field in ("min", "max", "step"):
            if field in setting:
                result[key][field] = setting[field]
    return result


def web_static_entities(product: dict[str, Any] | None = None) -> dict[str, dict[str, Any]]:
    data = product if product is not None else load_product()
    entities = data["project"].get("web_static_entities", {})
    if not isinstance(entities, dict):
        return {}
    return {
        str(key): dict(metadata)
        for key, metadata in entities.items()
        if isinstance(metadata, dict)
    }


def web_static_entities_metadata(product: dict[str, Any] | None = None) -> dict[str, dict[str, Any]]:
    result: dict[str, dict[str, Any]] = {}
    for key, metadata in web_static_entities(product).items():
        result[key] = {field: value for field, value in metadata.items() if field not in {"fetch", "firmware_file"}}
    return result


def web_entity_aliases(product: dict[str, Any] | None = None) -> dict[str, list[dict[str, Any]]]:
    data = product if product is not None else load_product()
    aliases = data["project"].get("web_entity_aliases", {})
    if not isinstance(aliases, dict):
        return {}
    return {
        str(key): [dict(alias) for alias in value if isinstance(alias, dict)]
        for key, value in aliases.items()
        if isinstance(value, list)
    }


def web_entity_aliases_metadata(product: dict[str, Any] | None = None) -> dict[str, list[dict[str, Any]]]:
    return {key: [dict(alias) for alias in aliases] for key, aliases in web_entity_aliases(product).items()}


def web_manual_entities(product: dict[str, Any] | None = None) -> dict[str, dict[str, Any]]:
    data = product if product is not None else load_product()
    entities = data["project"].get("web_manual_entities", {})
    if not isinstance(entities, dict):
        return {}
    return {
        str(key): dict(metadata)
        for key, metadata in entities.items()
        if isinstance(metadata, dict)
    }


def web_manual_entities_metadata(product: dict[str, Any] | None = None) -> dict[str, dict[str, Any]]:
    return {key: {"entity": metadata["entity"]} for key, metadata in web_manual_entities(product).items()}


def web_manual_state_keys(product: dict[str, Any] | None = None) -> list[str]:
    data = product if product is not None else load_product()
    keys = data["project"].get("web_manual_state_keys", [])
    if not isinstance(keys, list):
        return []
    return [str(key).strip() for key in keys if str(key).strip()]


def web_local_state_keys(product: dict[str, Any] | None = None) -> set[str]:
    data = product if product is not None else load_product()
    return {str(key).strip() for key in data["project"].get("web_local_state_keys", []) if str(key).strip()}


def web_initial_fetch_first_keys(product: dict[str, Any] | None = None) -> list[str]:
    data = product if product is not None else load_product()
    keys = data["project"].get("web_initial_fetch_first_keys", [])
    if not isinstance(keys, list):
        return []
    return [str(key).strip() for key in keys if str(key).strip()]


def web_live_render_state_keys(product: dict[str, Any] | None = None) -> list[str]:
    data = product if product is not None else load_product()
    keys = data["project"].get("web_live_render_state_keys", [])
    if not isinstance(keys, list):
        return []
    return [str(key).strip() for key in keys if str(key).strip()]


def web_live_render_state_prefixes(product: dict[str, Any] | None = None) -> list[str]:
    data = product if product is not None else load_product()
    prefixes = data["project"].get("web_live_render_state_prefixes", [])
    if not isinstance(prefixes, list):
        return []
    return [str(prefix).strip() for prefix in prefixes if str(prefix).strip()]


def web_initial_fetch_keys(product_settings: list[dict[str, Any]] | None = None) -> list[str]:
    product = load_product()
    if product_settings is None:
        product_settings = product["settings"]
    keys: list[str] = []

    def add(key: str) -> None:
        if key not in keys:
            keys.append(key)

    for key in web_initial_fetch_first_keys(product):
        add(key)
    for setting in product_settings:
        add(str(setting["key"]))
    for key, metadata in web_static_entities(product).items():
        if metadata.get("fetch"):
            add(key)
    return keys
