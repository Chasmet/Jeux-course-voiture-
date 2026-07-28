#!/usr/bin/env python3
"""Generate lightweight embedded glTF blockout assets for Space Kart Legends.

The generated models are original low-poly placeholders used until the final
FBX actors, skeletons, motions and production kart meshes are ready.
No third-party Python package is required.
"""
from __future__ import annotations

import base64
import json
import math
import struct
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Assets" / "Blockout"
Color = tuple[float, float, float, float]
Vec3 = tuple[float, float, float]


def rgba(hex_value: str) -> Color:
    value = hex_value.lstrip("#")
    return tuple(int(value[i:i + 2], 16) / 255.0 for i in (0, 2, 4)) + (1.0,)


@dataclass
class Part:
    name: str
    positions: list[Vec3]
    indices: list[int]
    color: Color


@dataclass
class Model:
    name: str
    parts: list[Part] = field(default_factory=list)

    def add_box(self, name: str, center: Vec3, size: Vec3, color: Color) -> None:
        cx, cy, cz = center
        sx, sy, sz = (value * 0.5 for value in size)
        vertices = [
            (cx - sx, cy - sy, cz - sz), (cx + sx, cy - sy, cz - sz),
            (cx + sx, cy + sy, cz - sz), (cx - sx, cy + sy, cz - sz),
            (cx - sx, cy - sy, cz + sz), (cx + sx, cy - sy, cz + sz),
            (cx + sx, cy + sy, cz + sz), (cx - sx, cy + sy, cz + sz),
        ]
        indices = [
            0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7,
            0, 1, 5, 0, 5, 4, 1, 2, 6, 1, 6, 5,
            2, 3, 7, 2, 7, 6, 3, 0, 4, 3, 4, 7,
        ]
        self.parts.append(Part(name, vertices, indices, color))

    def add_cylinder(
        self,
        name: str,
        center: Vec3,
        radius: float,
        length: float,
        axis: str,
        color: Color,
        segments: int = 10,
    ) -> None:
        positions: list[Vec3] = []
        half = length * 0.5
        for side in (-half, half):
            for i in range(segments):
                angle = math.tau * i / segments
                a, b = radius * math.cos(angle), radius * math.sin(angle)
                if axis == "x":
                    positions.append((center[0] + side, center[1] + a, center[2] + b))
                elif axis == "y":
                    positions.append((center[0] + a, center[1] + side, center[2] + b))
                else:
                    positions.append((center[0] + a, center[1] + b, center[2] + side))
        indices: list[int] = []
        for i in range(segments):
            n = (i + 1) % segments
            indices.extend((i, n, segments + n, i, segments + n, segments + i))
        for i in range(1, segments - 1):
            indices.extend((0, i + 1, i))
            indices.extend((segments, segments + i, segments + i + 1))
        self.parts.append(Part(name, positions, indices, color))

    def add_sphere(
        self,
        name: str,
        center: Vec3,
        radius: float,
        color: Color,
        rings: int = 5,
        segments: int = 8,
    ) -> None:
        positions: list[Vec3] = []
        for ring in range(rings + 1):
            phi = math.pi * ring / rings
            z = math.cos(phi) * radius
            ring_radius = math.sin(phi) * radius
            for i in range(segments):
                theta = math.tau * i / segments
                positions.append((
                    center[0] + math.cos(theta) * ring_radius,
                    center[1] + math.sin(theta) * ring_radius,
                    center[2] + z,
                ))
        indices: list[int] = []
        for ring in range(rings):
            for i in range(segments):
                n = (i + 1) % segments
                a = ring * segments + i
                b = ring * segments + n
                c = (ring + 1) * segments + i
                d = (ring + 1) * segments + n
                indices.extend((a, b, d, a, d, c))
        self.parts.append(Part(name, positions, indices, color))

    def add_limb(self, name: str, start: Vec3, end: Vec3, radius: float, color: Color) -> None:
        sx, sy, sz = start
        ex, ey, ez = end
        dx, dy, dz = ex - sx, ey - sy, ez - sz
        self.add_box(
            name,
            ((sx + ex) * 0.5, (sy + ey) * 0.5, (sz + ez) * 0.5),
            (max(abs(dx), radius * 2), max(abs(dy), radius * 2), max(abs(dz), radius * 2)),
            color,
        )


def pack_model(model: Model, destination: Path) -> None:
    binary = bytearray()
    buffer_views: list[dict] = []
    accessors: list[dict] = []
    materials: list[dict] = []
    material_lookup: dict[Color, int] = {}
    primitives: list[dict] = []

    def align() -> None:
        while len(binary) % 4:
            binary.append(0)

    def material_index(color: Color) -> int:
        if color not in material_lookup:
            material_lookup[color] = len(materials)
            materials.append({
                "name": f"material_{len(materials)}",
                "pbrMetallicRoughness": {
                    "baseColorFactor": list(color),
                    "metallicFactor": 0.25,
                    "roughnessFactor": 0.48,
                },
            })
        return material_lookup[color]

    for part in model.parts:
        align()
        position_offset = len(binary)
        flat_positions = [value for position in part.positions for value in position]
        binary.extend(struct.pack(f"<{len(flat_positions)}f", *flat_positions))
        position_view = len(buffer_views)
        buffer_views.append({
            "buffer": 0,
            "byteOffset": position_offset,
            "byteLength": len(flat_positions) * 4,
            "target": 34962,
        })
        minimum = [min(position[i] for position in part.positions) for i in range(3)]
        maximum = [max(position[i] for position in part.positions) for i in range(3)]
        position_accessor = len(accessors)
        accessors.append({
            "bufferView": position_view,
            "componentType": 5126,
            "count": len(part.positions),
            "type": "VEC3",
            "min": minimum,
            "max": maximum,
        })

        align()
        index_offset = len(binary)
        binary.extend(struct.pack(f"<{len(part.indices)}H", *part.indices))
        index_view = len(buffer_views)
        buffer_views.append({
            "buffer": 0,
            "byteOffset": index_offset,
            "byteLength": len(part.indices) * 2,
            "target": 34963,
        })
        index_accessor = len(accessors)
        accessors.append({
            "bufferView": index_view,
            "componentType": 5123,
            "count": len(part.indices),
            "type": "SCALAR",
            "min": [min(part.indices)],
            "max": [max(part.indices)],
        })
        primitives.append({
            "attributes": {"POSITION": position_accessor},
            "indices": index_accessor,
            "material": material_index(part.color),
            "mode": 4,
        })

    encoded = base64.b64encode(binary).decode("ascii")
    document = {
        "asset": {"version": "2.0", "generator": "SpaceKartLegends blockout generator"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"name": model.name, "mesh": 0}],
        "meshes": [{"name": model.name, "primitives": primitives}],
        "materials": materials,
        "buffers": [{
            "byteLength": len(binary),
            "uri": "data:application/octet-stream;base64," + encoded,
        }],
        "bufferViews": buffer_views,
        "accessors": accessors,
    }
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(document, separators=(",", ":")), encoding="utf-8")


def pilot_model(name: str, height: float, primary: Color, accent: Color, skin: Color, hair: str) -> Model:
    scale = height / 1.80
    model = Model(name)
    pelvis = 0.92 * scale
    model.add_box("torso", (0, 0, 1.20 * scale), (0.42 * scale, 0.24 * scale, 0.55 * scale), primary)
    model.add_box("pelvis", (0, 0, pelvis), (0.34 * scale, 0.22 * scale, 0.23 * scale), primary)
    model.add_box("chest_accent", (0, -0.13 * scale, 1.27 * scale), (0.43 * scale, 0.05 * scale, 0.07 * scale), accent)
    model.add_box("belt", (0, 0, 1.00 * scale), (0.38 * scale, 0.255 * scale, 0.07 * scale), accent)
    model.add_sphere("head", (0, 0, 1.64 * scale), 0.115 * scale, skin)
    if hair == "afro":
        model.add_sphere("hair", (0, 0, 1.76 * scale), 0.135 * scale, rgba("#15110E"))
    elif hair == "helmet":
        model.add_sphere("helmet", (0, 0, 1.65 * scale), 0.132 * scale, accent)
    else:
        model.add_cylinder("hair", (0, 0, 1.735 * scale), 0.112 * scale, 0.035 * scale, "z", rgba("#151310"))

    for side, sign in (("left", -1), ("right", 1)):
        hip = (0.12 * scale * sign, 0, pelvis - 0.05 * scale)
        knee = (0.13 * scale * sign, 0, 0.50 * scale)
        ankle = (0.14 * scale * sign, 0, 0.12 * scale)
        shoulder = (0.25 * scale * sign, 0, 1.43 * scale)
        elbow = (0.48 * scale * sign, 0, 1.17 * scale)
        hand = (0.62 * scale * sign, 0, 0.98 * scale)
        model.add_limb(f"upper_leg_{side}", hip, knee, 0.075 * scale, primary)
        model.add_limb(f"lower_leg_{side}", knee, ankle, 0.065 * scale, primary)
        model.add_sphere(f"knee_{side}", knee, 0.085 * scale, accent)
        model.add_box(f"boot_{side}", (ankle[0], 0.07 * scale, 0.055 * scale), (0.15 * scale, 0.30 * scale, 0.09 * scale), primary)
        model.add_limb(f"upper_arm_{side}", shoulder, elbow, 0.065 * scale, primary)
        model.add_limb(f"forearm_{side}", elbow, hand, 0.055 * scale, primary)
        model.add_sphere(f"shoulder_{side}", shoulder, 0.075 * scale, accent)
        model.add_sphere(f"glove_{side}", hand, 0.065 * scale, accent)
    return model


def kart_model(name: str, body: Color, accent: Color, trim: Color) -> Model:
    model = Model(name)
    model.add_box("main_chassis", (0, 0, 0.40), (1.55, 2.05, 0.32), body)
    model.add_box("nose", (0, 1.15, 0.44), (0.85, 0.90, 0.24), body)
    model.add_box("nose_accent", (0, 1.05, 0.60), (0.40, 1.15, 0.08), accent)
    model.add_box("sidepod_left", (-0.82, 0.05, 0.43), (0.32, 1.30, 0.36), body)
    model.add_box("sidepod_right", (0.82, 0.05, 0.43), (0.32, 1.30, 0.36), body)
    model.add_box("seat", (0, -0.25, 0.72), (0.72, 0.72, 0.52), trim)
    model.add_box("seat_back", (0, -0.63, 0.93), (0.56, 0.18, 0.75), trim)
    model.add_box("rear_wing", (0, -1.15, 0.88), (1.42, 0.18, 0.13), accent)
    model.add_box("wing_support_left", (-0.52, -0.96, 0.68), (0.11, 0.38, 0.55), trim)
    model.add_box("wing_support_right", (0.52, -0.96, 0.68), (0.11, 0.38, 0.55), trim)
    wheel = rgba("#19191E")
    for x_name, x in (("left", -0.93), ("right", 0.93)):
        for y_name, y in (("rear", -0.72), ("front", 0.72)):
            model.add_cylinder(f"wheel_{x_name}_{y_name}", (x, y, 0.29), 0.32, 0.28, "x", wheel)
            model.add_cylinder(f"hub_{x_name}_{y_name}", (x, y, 0.29), 0.17, 0.292, "x", accent)
    model.add_cylinder("steering_wheel", (0, 0.35, 0.92), 0.19, 0.05, "y", rgba("#20232A"), 12)
    model.add_cylinder("thruster_left", (-0.34, -1.17, 0.40), 0.12, 0.30, "y", accent)
    model.add_cylinder("thruster_right", (0.34, -1.17, 0.40), 0.12, 0.30, "y", accent)
    model.add_box("light_left", (-0.34, 1.61, 0.48), (0.30, 0.05, 0.08), accent)
    model.add_box("light_right", (0.34, 1.61, 0.48), (0.30, 0.05, 0.08), accent)
    return model


def generate() -> list[Path]:
    pilot_specs = {
        "Cheikh": (1.80, rgba("#B8AD91"), rgba("#168CFF"), rgba("#6C4128"), "short"),
        "Yvane": (1.55, rgba("#0F1118"), rgba("#FFC400"), rgba("#512B1A"), "afro"),
        "Nelvyn": (1.35, rgba("#0F131A"), rgba("#32F071"), rgba("#532D1B"), "short"),
        "Nova": (1.65, rgba("#21102F"), rgba("#F13DFF"), rgba("#754935"), "helmet"),
    }
    kart_specs = {
        "AzureComet": (rgba("#D8CEB6"), rgba("#168CFF"), rgba("#F4F4EE")),
        "SolarStrike": (rgba("#080A0F"), rgba("#FFC400"), rgba("#2A2F3A")),
        "EmeraldPulse": (rgba("#0B0F16"), rgba("#32F071"), rgba("#353B44")),
        "VioletPhoton": (rgba("#1D0D2A"), rgba("#F13DFF"), rgba("#40E8FF")),
    }
    outputs: list[Path] = []
    for name, spec in pilot_specs.items():
        path = OUT / "Characters" / name / f"{name.lower()}_blockout.gltf"
        pack_model(pilot_model(name, *spec), path)
        outputs.append(path)
    for name, spec in kart_specs.items():
        path = OUT / "Vehicles" / name / f"{name.lower()}_blockout.gltf"
        pack_model(kart_model(name, *spec), path)
        outputs.append(path)
    return outputs


if __name__ == "__main__":
    generated = generate()
    for path in generated:
        print(path.relative_to(ROOT))
