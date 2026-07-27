#!/usr/bin/env python3
"""Fast repository validation for Space Kart Legends.

This validates manifests, generated blockout assets and critical gameplay source
patterns. It deliberately does not claim to replace a real O3DE CMake/Android
build with the engine and SDK installed.
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
    "scripts/generate_blockout_assets.py",
    "scripts/generate_environment_assets.py",
    "scripts/validate_environment_assets.py",
    "Docs/ASSET_PIPELINE.md",
]

EXPECTED_PILOTS = {"cheikh", "yvane", "nelvyn", "nova"}
EXPECTED_CIRCUITS = {
    "orbit_zero",
    "saturn_rings",
    "turbo_nebula",
    "titan_station",
    "final_black_hole",
}
EXPECTED_ITEMS = {
    "comet_boost",
    "plasma_shield",
    "gravity_mine",
    "photon_pulse",
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


def require_tokens(source: str, tokens: set[str], label: str) -> None:
    missing = sorted(token for token in tokens if token not in source)
    if missing:
        fail(f"{label} is missing required gameplay tokens: {', '.join(missing)}")


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


def validate_generated_gltf(relative_path: str) -> None:
    path = ROOT / "Assets" / relative_path
    if not path.is_file():
        fail(f"generated blockout asset is missing: Assets/{relative_path}")
    document = load_json(str(path.relative_to(ROOT)))
    if document.get("asset", {}).get("version") != "2.0":
        fail(f"invalid glTF version in Assets/{relative_path}")
    if not document.get("meshes") or not document.get("buffers"):
        fail(f"incomplete glTF model in Assets/{relative_path}")
    uri = document["buffers"][0].get("uri", "")
    if not uri.startswith("data:application/octet-stream;base64,"):
        fail(f"blockout glTF must embed its geometry: Assets/{relative_path}")


def validate_game_content() -> None:
    content = load_json("Assets/Config/game_content.json")
    if content.get("schemaVersion") != 4:
        fail("game_content schemaVersion must be 4")

    game = content.get("game", {})
    if game.get("lapsPerRace") != 3 or game.get("countdownSeconds") != 3.5:
        fail("race configuration must use three laps and a 3.5 second countdown")
    if game.get("autoAcceleration") is not True:
        fail("Android arcade controls require autoAcceleration")
    if game.get("startupLevel") != "spacekartlegends":
        fail("game_content startupLevel must be spacekartlegends")

    pilots = content.get("pilots", [])
    pilot_ids = {pilot.get("id") for pilot in pilots}
    if pilot_ids != EXPECTED_PILOTS:
        fail(f"pilot ids mismatch: {sorted(pilot_ids)}")

    circuits = content.get("circuits", [])
    circuit_ids = {circuit.get("id") for circuit in circuits}
    if circuit_ids != EXPECTED_CIRCUITS:
        fail(f"circuit ids mismatch: {sorted(circuit_ids)}")

    items = content.get("items", [])
    item_ids = {item.get("id") for item in items}
    if item_ids != EXPECTED_ITEMS:
        fail(f"item ids mismatch: {sorted(item_ids)}")
    gates = content.get("itemGateProgress", [])
    if len(gates) != 4 or gates != sorted(gates) or not all(0.0 < gate < 1.0 for gate in gates):
        fail("itemGateProgress must contain four sorted progress values between zero and one")

    controls = content.get("mobileControls", {})
    expected_control_keys = {"steering", "item", "drift", "brake"}
    if set(controls) != expected_control_keys:
        fail("mobileControls must define steering, item, drift and brake")

    animations = set(content.get("requiredDriverAnimations", []))
    missing_animations = sorted(EXPECTED_ANIMATIONS - animations)
    if missing_animations:
        fail("missing driver animations: " + ", ".join(missing_animations))

    karts = content.get("karts", [])
    kart_ids = {kart.get("id") for kart in karts}
    for pilot in pilots:
        if pilot.get("kartId") not in kart_ids:
            fail(f"pilot {pilot.get('id')} references an unknown kart")
        blockout = pilot.get("blockoutModelPath")
        production = pilot.get("productionModelPath")
        if not blockout or not production or not production.endswith(".fbx"):
            fail(f"pilot {pilot.get('id')} has incomplete model paths")
        validate_generated_gltf(blockout)

    for kart in karts:
        blockout = kart.get("blockoutModelPath")
        production = kart.get("productionModelPath")
        if not blockout or not production or not production.endswith(".fbx"):
            fail(f"kart {kart.get('id')} has incomplete model paths")
        validate_generated_gltf(blockout)

    for circuit in circuits:
        blockout = circuit.get("blockoutModelPath")
        production = circuit.get("productionPrefabPath")
        if not blockout or not production or not production.endswith(".prefab"):
            fail(f"circuit {circuit.get('id')} has incomplete asset paths")
        validate_generated_gltf(blockout)

    for item in items:
        blockout = item.get("blockoutModelPath")
        production = item.get("productionModelPath")
        if not blockout or not production or not production.endswith(".fbx"):
            fail(f"item {item.get('id')} has incomplete model paths")
        validate_generated_gltf(blockout)


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


def validate_gameplay_features() -> None:
    race_header = (ROOT / "Gem/Source/SpaceKartRace.h").read_text(encoding="utf-8")
    race_source = (ROOT / "Gem/Source/SpaceKartRace.cpp").read_text(encoding="utf-8")
    system_source = (ROOT / "Gem/Source/SpaceKartLegendsSystemComponent.cpp").read_text(encoding="utf-8")

    require_tokens(
        race_header,
        {"RacePhase", "ItemType", "UseItem", "PlasmaShield", "GravityMine", "PhotonPulse"},
        "SpaceKartRace.h",
    )
    require_tokens(
        race_source,
        {"m_countdownTime", "CheckItemPickup", "UseDriverItem", "ApplyHit", "DrawItemGates"},
        "SpaceKartRace.cpp",
    )
    require_tokens(
        system_source,
        {
            "InputDeviceTouch",
            "PositionData2D",
            "m_race.UseItem()",
            "EnsureActiveCamera",
            "CameraComponentTypeId",
            "MakeActiveView",
        },
        "SpaceKartLegendsSystemComponent.cpp",
    )


def main() -> int:
    validate_required_files()
    validate_project_manifest()
    validate_game_content()
    validate_source_registration()
    validate_track_math()
    validate_gameplay_features()
    print("Space Kart Legends validation passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
