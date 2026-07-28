#!/usr/bin/env python3
"""Validate generated track and item glTF blockouts."""

from __future__ import annotations

import base64
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

TRACKS = (
    "Assets/Blockout/Tracks/OrbitZero/orbit_zero_track.gltf",
    "Assets/Blockout/Tracks/SaturnRings/saturn_rings_track.gltf",
    "Assets/Blockout/Tracks/TurboNebula/turbo_nebula_track.gltf",
    "Assets/Blockout/Tracks/TitanStation/titan_station_track.gltf",
    "Assets/Blockout/Tracks/FinalBlackHole/final_black_hole_track.gltf",
)

ITEMS = (
    "Assets/Blockout/Items/CometBoost/comet_boost.gltf",
    "Assets/Blockout/Items/PlasmaShield/plasma_shield.gltf",
    "Assets/Blockout/Items/GravityMine/gravity_mine.gltf",
    "Assets/Blockout/Items/PhotonPulse/photon_pulse.gltf",
)


def fail(message: str) -> None:
    print(f"ERROR: {message}", file=sys.stderr)
    raise SystemExit(1)


def validate_gltf(relative_path: str, minimum_vertices: int, minimum_indices: int) -> None:
    path = ROOT / relative_path
    if not path.is_file():
        fail(f"missing generated asset: {relative_path}")
    try:
        document = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        fail(f"invalid glTF JSON in {relative_path}: {exc}")

    if document.get("asset", {}).get("version") != "2.0":
        fail(f"{relative_path} is not glTF 2.0")
    meshes = document.get("meshes", [])
    accessors = document.get("accessors", [])
    buffers = document.get("buffers", [])
    if len(meshes) != 1 or len(accessors) < 2 or len(buffers) != 1:
        fail(f"{relative_path} has an incomplete mesh structure")
    if accessors[0].get("count", 0) < minimum_vertices:
        fail(f"{relative_path} has too few vertices")
    if accessors[1].get("count", 0) < minimum_indices:
        fail(f"{relative_path} has too few indices")

    uri = buffers[0].get("uri", "")
    prefix = "data:application/octet-stream;base64,"
    if not uri.startswith(prefix):
        fail(f"{relative_path} does not embed its geometry")
    try:
        decoded = base64.b64decode(uri[len(prefix):], validate=True)
    except ValueError as exc:
        fail(f"{relative_path} has invalid embedded geometry: {exc}")
    if len(decoded) != buffers[0].get("byteLength"):
        fail(f"{relative_path} buffer length mismatch")


def main() -> int:
    for track in TRACKS:
        validate_gltf(track, minimum_vertices=500, minimum_indices=2000)
    for item in ITEMS:
        validate_gltf(item, minimum_vertices=6, minimum_indices=24)
    print("Environment blockout validation passed: 5 tracks and 4 items.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
