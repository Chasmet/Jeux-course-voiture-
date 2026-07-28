#!/usr/bin/env python3
"""Generate lightweight glTF blockouts for Space Kart Legends tracks and items.

The generated files are real embedded glTF 2.0 meshes. They remain technical
blockouts, but they can be inspected, imported and later replaced by final art.
"""

from __future__ import annotations

import base64
import json
import math
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Iterable

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "Assets" / "Blockout"


@dataclass(frozen=True)
class TrackSpec:
    slug: str
    folder: str
    radius_x: float
    radius_y: float
    width: float
    height_wave: float
    color: tuple[float, float, float, float]
    glow: tuple[float, float, float, float]
    shape: Callable[[float, "TrackSpec"], tuple[float, float, float]]


def normalize(vector: tuple[float, float, float]) -> tuple[float, float, float]:
    length = math.sqrt(sum(value * value for value in vector))
    if length <= 1.0e-8:
        return (1.0, 0.0, 0.0)
    return tuple(value / length for value in vector)  # type: ignore[return-value]


def subtract(a: tuple[float, float, float], b: tuple[float, float, float]) -> tuple[float, float, float]:
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def cross(a: tuple[float, float, float], b: tuple[float, float, float]) -> tuple[float, float, float]:
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def add_scaled(
    position: tuple[float, float, float],
    direction: tuple[float, float, float],
    scale: float,
) -> tuple[float, float, float]:
    return (
        position[0] + direction[0] * scale,
        position[1] + direction[1] * scale,
        position[2] + direction[2] * scale,
    )


def orbit_shape(angle: float, spec: TrackSpec) -> tuple[float, float, float]:
    return (
        math.cos(angle) * spec.radius_x,
        math.sin(angle) * spec.radius_y,
        math.sin(angle * 2.0) * spec.height_wave,
    )


def saturn_shape(angle: float, spec: TrackSpec) -> tuple[float, float, float]:
    radial = 1.0 + 0.10 * math.sin(angle * 3.0)
    return (
        math.cos(angle) * spec.radius_x * radial,
        math.sin(angle) * spec.radius_y * radial,
        math.sin(angle * 2.5 + 0.7) * spec.height_wave,
    )


def nebula_shape(angle: float, spec: TrackSpec) -> tuple[float, float, float]:
    return (
        math.cos(angle) * (spec.radius_x + 5.5 * math.sin(angle * 3.0)),
        math.sin(angle) * (spec.radius_y + 4.0 * math.cos(angle * 2.0)),
        math.sin(angle * 3.5 + 1.4) * spec.height_wave,
    )


def titan_shape(angle: float, spec: TrackSpec) -> tuple[float, float, float]:
    return (
        math.cos(angle) * spec.radius_x + math.cos(angle * 3.0) * 6.0,
        math.sin(angle) * spec.radius_y + math.sin(angle * 2.0) * 4.0,
        math.sin(angle * 4.0 + 2.1) * spec.height_wave,
    )


def black_hole_shape(angle: float, spec: TrackSpec) -> tuple[float, float, float]:
    radial = 1.0 + 0.15 * math.sin(angle * 2.0) + 0.07 * math.cos(angle * 5.0)
    return (
        math.cos(angle) * spec.radius_x * radial,
        math.sin(angle) * spec.radius_y * radial,
        math.sin(angle * 4.0 + 2.8) * spec.height_wave + math.sin(angle) * 2.5,
    )


TRACKS = (
    TrackSpec("orbit_zero", "OrbitZero", 42.0, 30.0, 10.0, 2.5, (0.10, 0.24, 0.42, 1.0), (0.15, 0.85, 1.0, 1.0), orbit_shape),
    TrackSpec("saturn_rings", "SaturnRings", 50.0, 22.0, 9.0, 5.5, (0.55, 0.30, 0.10, 1.0), (1.0, 0.65, 0.15, 1.0), saturn_shape),
    TrackSpec("turbo_nebula", "TurboNebula", 36.0, 36.0, 11.0, 8.0, (0.38, 0.10, 0.52, 1.0), (0.95, 0.20, 1.0, 1.0), nebula_shape),
    TrackSpec("titan_station", "TitanStation", 58.0, 28.0, 8.5, 3.0, (0.18, 0.34, 0.30, 1.0), (0.25, 1.0, 0.60, 1.0), titan_shape),
    TrackSpec("final_black_hole", "FinalBlackHole", 44.0, 34.0, 9.0, 12.0, (0.08, 0.08, 0.12, 1.0), (1.0, 0.15, 0.40, 1.0), black_hole_shape),
)


def pad4(buffer: bytearray) -> None:
    while len(buffer) % 4:
        buffer.append(0)


def bounds(positions: Iterable[tuple[float, float, float]]) -> tuple[list[float], list[float]]:
    values = list(positions)
    return (
        [min(position[axis] for position in values) for axis in range(3)],
        [max(position[axis] for position in values) for axis in range(3)],
    )


def write_gltf(
    output: Path,
    name: str,
    positions: list[tuple[float, float, float]],
    indices: list[int],
    color: tuple[float, float, float, float],
    emissive: tuple[float, float, float] = (0.0, 0.0, 0.0),
) -> None:
    binary = bytearray()
    position_offset = len(binary)
    for position in positions:
        binary.extend(struct.pack("<3f", *position))
    position_length = len(binary) - position_offset
    pad4(binary)

    index_offset = len(binary)
    for index in indices:
        binary.extend(struct.pack("<I", index))
    index_length = len(binary) - index_offset
    pad4(binary)

    minimum, maximum = bounds(positions)
    uri = "data:application/octet-stream;base64," + base64.b64encode(binary).decode("ascii")
    document = {
        "asset": {"version": "2.0", "generator": "SpaceKartLegends blockout generator"},
        "scene": 0,
        "scenes": [{"name": name, "nodes": [0]}],
        "nodes": [{"name": name, "mesh": 0}],
        "meshes": [{"name": name, "primitives": [{"attributes": {"POSITION": 0}, "indices": 1, "material": 0}]}],
        "materials": [{
            "name": f"{name}_material",
            "pbrMetallicRoughness": {
                "baseColorFactor": list(color),
                "metallicFactor": 0.25,
                "roughnessFactor": 0.48,
            },
            "emissiveFactor": list(emissive),
        }],
        "buffers": [{"byteLength": len(binary), "uri": uri}],
        "bufferViews": [
            {"buffer": 0, "byteOffset": position_offset, "byteLength": position_length, "target": 34962},
            {"buffer": 0, "byteOffset": index_offset, "byteLength": index_length, "target": 34963},
        ],
        "accessors": [
            {
                "bufferView": 0,
                "componentType": 5126,
                "count": len(positions),
                "type": "VEC3",
                "min": minimum,
                "max": maximum,
            },
            {
                "bufferView": 1,
                "componentType": 5125,
                "count": len(indices),
                "type": "SCALAR",
                "min": [min(indices)],
                "max": [max(indices)],
            },
        ],
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(document, indent=2), encoding="utf-8")


def generate_track(spec: TrackSpec, segments: int = 192) -> None:
    positions: list[tuple[float, float, float]] = []
    indices: list[int] = []
    half_width = spec.width * 0.5
    thickness = 0.55
    centers = [spec.shape((index / segments) * math.tau, spec) for index in range(segments)]

    for index, center in enumerate(centers):
        previous = centers[(index - 1) % segments]
        following = centers[(index + 1) % segments]
        tangent = normalize(subtract(following, previous))
        right = normalize(cross(tangent, (0.0, 0.0, 1.0)))
        left_top = add_scaled(center, right, -half_width)
        right_top = add_scaled(center, right, half_width)
        positions.extend(
            [
                left_top,
                right_top,
                (left_top[0], left_top[1], left_top[2] - thickness),
                (right_top[0], right_top[1], right_top[2] - thickness),
            ]
        )

    for index in range(segments):
        current = index * 4
        following = ((index + 1) % segments) * 4
        lt, rt, lb, rb = current, current + 1, current + 2, current + 3
        nlt, nrt, nlb, nrb = following, following + 1, following + 2, following + 3
        indices.extend(
            [
                lt, nlt, rt, rt, nlt, nrt,
                lb, rb, nlb, rb, nrb, nlb,
                lt, lb, nlt, lb, nlb, nlt,
                rt, nrt, rb, rb, nrt, nrb,
            ]
        )

    write_gltf(
        ASSETS / "Tracks" / spec.folder / f"{spec.slug}_track.gltf",
        f"{spec.folder}Track",
        positions,
        indices,
        spec.color,
        spec.glow[:3],
    )


def octahedron(scale_x: float, scale_y: float, scale_z: float) -> tuple[list[tuple[float, float, float]], list[int]]:
    positions = [
        (scale_x, 0.0, 0.0), (-scale_x, 0.0, 0.0),
        (0.0, scale_y, 0.0), (0.0, -scale_y, 0.0),
        (0.0, 0.0, scale_z), (0.0, 0.0, -scale_z),
    ]
    indices = [
        4, 0, 2, 4, 2, 1, 4, 1, 3, 4, 3, 0,
        5, 2, 0, 5, 1, 2, 5, 3, 1, 5, 0, 3,
    ]
    return positions, indices


def spiked_mine() -> tuple[list[tuple[float, float, float]], list[int]]:
    core_positions, core_indices = octahedron(0.72, 0.72, 0.72)
    positions = list(core_positions)
    indices = list(core_indices)
    axes = ((1.35, 0.0, 0.0), (-1.35, 0.0, 0.0), (0.0, 1.35, 0.0), (0.0, -1.35, 0.0), (0.0, 0.0, 1.35), (0.0, 0.0, -1.35))
    for tip in axes:
        base = len(positions)
        positions.extend([(tip[0] * 0.5, tip[1] * 0.5, tip[2] * 0.5), tip, (tip[1] * 0.15, tip[2] * 0.15, tip[0] * 0.15)])
        indices.extend([base, base + 1, base + 2])
    return positions, indices


def generate_items() -> None:
    item_specs = (
        ("CometBoost", "comet_boost", octahedron(0.55, 1.25, 0.55), (0.15, 0.85, 1.0, 1.0)),
        ("PlasmaShield", "plasma_shield", octahedron(1.0, 1.0, 1.0), (0.25, 0.55, 1.0, 0.82)),
        ("GravityMine", "gravity_mine", spiked_mine(), (1.0, 0.25, 0.45, 1.0)),
        ("PhotonPulse", "photon_pulse", octahedron(1.2, 0.42, 1.2), (0.95, 0.40, 1.0, 1.0)),
    )
    for display_name, slug, (positions, indices), color in item_specs:
        write_gltf(
            ASSETS / "Items" / display_name / f"{slug}.gltf",
            display_name,
            positions,
            indices,
            color,
            color[:3],
        )


def main() -> int:
    for track in TRACKS:
        generate_track(track)
    generate_items()
    print(f"Generated {len(TRACKS)} track meshes and 4 item meshes in {ASSETS}.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
