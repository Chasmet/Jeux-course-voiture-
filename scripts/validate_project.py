#!/usr/bin/env python3
"""Fast repository validation for Space Kart Legends.

This check intentionally does not pretend to compile O3DE. It validates the
project manifests, expected content, source registration and the critical track
math that previously caused infinite recursion.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_FILES = [
    "project.json",
    "CMakeLists.txt",
    "Gem/gem.json",
    "Gem/CMakeLists.txt",
    "Gem/spacekartlegends_files.cmake",
    "Gem/spacekartlegends_shared_files.cmake",
    "Gem/Source/SpaceKartRace.h",
    "Gem/Source/SpaceKartRace.cpp",
    "Gem/Source/SpaceKartLegendsSystemComponent.h",
    "Gem/Source/SpaceKartLegendsSystemComponent.cpp",
    "Gem/Source/SpaceKartLegendsModule.cpp",
    "Assets/Config/game_content.json",
]

EXPECTED_PILOTS = {"cheikh", "yvane", "nelvyn", "nova"}
EXPECTED_CIRCUITS = {
    "orbit_zero",
    "saturn_rings",
    "turbo_nebula",
    "titan_station",
    "final_black_hole",
}
EXPECTED_ANIMATIONS = {
    "idle",
    "steer_left",
    "steer_right",
    "drift_left",
    "drift_right",
    "boost",
    "brake",
    "jump",
    "land",
    "victory",
}


def fail(message: str) -> None:
    print(f"ERROR: {message}", file=sys.stderr)
    raise SystemExit(1)


def load_json(relative_path: str) -> dict:
    path = ROOT / relative_path
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        fail(f"missing JSON file: {relative_path}")
    except json.JSONDecodeError as exc:
        fail(f"invalid JSON in {relative_path}: {exc}")


def validate_required_files() -> None:
    missing = [path for path in REQUIRED_FILES if not (ROOT / path).is_file()]
    if missing:
        fail("missing required files: " + ", ".join(missing))


def validate_project_manifest() -> None:
    project = load_json("project.json")
    if project.get("project_name") != "SpaceKartLegends":
        fail("project_name must be SpaceKartLegends")
    if project.get("engine_version") != "26.05.0":
        fail("engine_version must stay pinned to 26.05.0")
    gems = set(project.get("gem_names", []))
    required_gems = {"SpaceKartLegends", "Atom", "Camera", "EMotionFX", "PhysX5", "StartingPointInput"}
    missing_gems = sorted(required_gems - gems)
    if missing_gems:
        fail("project.json is missing gems: " + ", ".join(missing_gems))


def validate_game_content() -> None:
    content = load_json("Assets/Config/game_content.json")
    pilots = content.get("pilots", [])
    pilot_ids = {pilot.get("id") for pilot in pilots}
    if pilot_ids != EXPECTED_PILOTS:
        fail(f"pilot ids mismatch: {sorted(pilot_ids)}")

    circuits = content.get("circuits", [])
    circuit_ids = {circuit.get("id") for circuit in circuits}
    if circuit_ids != EXPECTED_CIRCUITS:
        fail(f"circuit ids mismatch: {sorted(circuit_ids)}")

    animations = set(content.get("requiredDriverAnimations", []))
    missing_animations = sorted(EXPECTED_ANIMATIONS - animations)
    if missing_animations:
        fail("missing driver animations: " + ", ".join(missing_animations))

    kart_ids = {kart.get("id") for kart in content.get("karts", [])}
    for pilot in pilots:
        if pilot.get("kartId") not in kart_ids:
            fail(f"pilot {pilot.get('id')} references an unknown kart")


def validate_source_registration() -> None:
    file_list = (ROOT / "Gem/spacekartlegends_files.cmake").read_text(encoding="utf-8")
    required_sources = {
        "Source/SpaceKartRace.cpp",
        "Source/SpaceKartRace.h",
        "Source/SpaceKartLegendsSystemComponent.cpp",
        "Source/SpaceKartLegendsSystemComponent.h",
    }
    for source in required_sources:
        if source not in file_list:
            fail(f"{source} is not registered in spacekartlegends_files.cmake")


def validate_track_math() -> None:
    source = (ROOT / "Gem/Source/SpaceKartRace.cpp").read_text(encoding="utf-8")
    tangent_start = source.find("AZ::Vector3 SpaceKartRace::GetTrackTangent")
    next_method = source.find("AZ::Vector3 SpaceKartRace::GetPlayerPosition", tangent_start)
    if tangent_start < 0 or next_method < 0:
        fail("could not locate GetTrackTangent implementation")
    tangent_body = source[tangent_start:next_method]
    if "GetTrackPosition(" in tangent_body:
        fail("GetTrackTangent must not call GetTrackPosition; this creates infinite recursion")
    if "GetCenterlinePosition(" not in tangent_body:
        fail("GetTrackTangent must sample GetCenterlinePosition")


def main() -> int:
    validate_required_files()
    validate_project_manifest()
    validate_game_content()
    validate_source_registration()
    validate_track_math()
    print("Space Kart Legends validation passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
