#!/usr/bin/env python3
"""Validate the files required before an O3DE Android export."""

from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    print(f"ERROR: {message}", file=sys.stderr)
    raise SystemExit(1)


def load_json(relative_path: str) -> dict:
    path = ROOT / relative_path
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        fail(f"missing file: {relative_path}")
    except json.JSONDecodeError as exc:
        fail(f"invalid JSON in {relative_path}: {exc}")


def validate_android_project() -> None:
    path = "Platform/Android/android_project.json"
    project = load_json(path)
    settings = project.get("android_settings", {})
    if settings.get("package_name") != "com.chasmet.spacekartlegends":
        fail("unexpected Android package name")
    if settings.get("orientation") != "landscape":
        fail("Android orientation must be landscape")
    if not isinstance(settings.get("version_number"), int) or settings["version_number"] < 1:
        fail("Android version_number must be a positive integer")
    if not (ROOT / "Platform/Android/android_project.cmake").is_file():
        fail("missing Platform/Android/android_project.cmake")


def validate_startup_level() -> None:
    registry = load_json("Registry/load_level.setreg")
    try:
        level_name = registry["O3DE"]["Autoexec"]["ConsoleCommands"]["LoadLevel"]
    except (KeyError, TypeError):
        fail("Registry/load_level.setreg has no LoadLevel command")
    if level_name.lower() != "spacekartlegends":
        fail("startup level must be spacekartlegends")

    level = load_json("Levels/SpaceKartLegends/SpaceKartLegends.prefab")
    container = level.get("ContainerEntity")
    if not isinstance(container, dict):
        fail("startup prefab has no ContainerEntity")
    if container.get("Name") != "SpaceKartLegends":
        fail("startup prefab container name mismatch")
    components = container.get("Components", {})
    component_types = {component.get("$type", "") for component in components.values()}
    if not any("TransformComponent" in component_type for component_type in component_types):
        fail("startup prefab container has no TransformComponent")
    if "EditorPrefabComponent" not in component_types:
        fail("startup prefab container has no EditorPrefabComponent")
    if not isinstance(level.get("Entities"), dict):
        fail("startup prefab Entities must be an object")


def validate_android_quality() -> None:
    quality = load_json("Registry/Platform/Android/quality.setreg")
    try:
        render_scale = quality["O3DE"]["Quality"]["Groups"]["q_graphics"]["Settings"]["r_renderScale"]
    except (KeyError, TypeError):
        fail("Android quality file has no r_renderScale levels")
    if not isinstance(render_scale, list) or len(render_scale) < 3:
        fail("r_renderScale must define multiple Android quality levels")
    if any(not isinstance(value, (int, float)) or value <= 0.0 or value > 1.0 for value in render_scale):
        fail("Android render scales must be in the range ]0, 1]")


def main() -> int:
    validate_android_project()
    validate_startup_level()
    validate_android_quality()
    print("O3DE Android scaffold validation passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
