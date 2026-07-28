#!/usr/bin/env python3
"""Install the private Cheikh/Yvane/Nelvyn rig pack into this O3DE project.

The private archive is deliberately kept outside the public repository. This
installer rejects unsafe archives and raw media, verifies hashes, inspects the
actual GLB skeleton, skinning, animation, face-texture and socket data, and
writes the local override used by character integration.
"""
from __future__ import annotations

import hashlib
import json
import shutil
import struct
import sys
import tempfile
import zipfile
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parents[1]
TARGET_ROOT = ROOT / "Assets" / "PrivateCharacters"
OVERRIDE_PATH = ROOT / "Assets" / "Config" / "private_character_overrides.json"
EXPECTED_PILOTS = {"Cheikh", "Yvane", "Nelvyn"}
EXPECTED_ANIMATIONS = {
    "drive_idle", "steer_left", "steer_right", "drift_left", "drift_right",
    "boost", "brake", "jump", "land", "impact_left", "impact_right",
    "spin", "victory",
}
REQUIRED_SOCKETS = {
    "kart_seat_socket",
    "steering_left_socket",
    "steering_right_socket",
    "face_camera_socket",
}
FORBIDDEN_SUFFIXES = {".jpg", ".jpeg", ".webp", ".mp4", ".mov", ".m4v"}
GLB_MAGIC = b"glTF"
GLB_VERSION = 2
JSON_CHUNK_TYPE = 0x4E4F534A
SUPPORTED_ANIMATION_PATHS = {"translation", "rotation", "scale"}


def fail(message: str) -> None:
    raise SystemExit(f"ERROR: {message}")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def validate_archive_members(archive: zipfile.ZipFile) -> None:
    """Reject path traversal, absolute paths, links and raw personal media."""
    for info in archive.infolist():
        name = info.filename.replace("\\", "/")
        path = PurePosixPath(name)
        if path.is_absolute() or ".." in path.parts:
            fail(f"unsafe path in private archive: {info.filename}")
        unix_mode = (info.external_attr >> 16) & 0o170000
        if unix_mode == 0o120000:
            fail(f"symbolic links are forbidden in private archive: {info.filename}")
        if path.suffix.lower() in FORBIDDEN_SUFFIXES:
            fail(f"raw media is forbidden in the private model pack: {info.filename}")


def inspect_glb(path: Path) -> dict:
    """Read and validate the JSON chunk of a binary glTF 2.0 file."""
    data = path.read_bytes()
    if len(data) < 20:
        fail(f"GLB is truncated: {path.name}")
    magic, version, declared_length = struct.unpack_from("<4sII", data, 0)
    if magic != GLB_MAGIC or version != GLB_VERSION:
        fail(f"{path.name} is not a glTF 2.0 binary file")
    if declared_length != len(data):
        fail(f"GLB length mismatch for {path.name}")

    offset = 12
    json_document = None
    while offset + 8 <= len(data):
        chunk_length, chunk_type = struct.unpack_from("<II", data, offset)
        offset += 8
        end = offset + chunk_length
        if end > len(data):
            fail(f"invalid GLB chunk length in {path.name}")
        if chunk_type == JSON_CHUNK_TYPE and json_document is None:
            try:
                json_document = json.loads(data[offset:end].rstrip(b"\x00 \t\r\n").decode("utf-8"))
            except (UnicodeDecodeError, json.JSONDecodeError) as exc:
                fail(f"invalid GLB JSON in {path.name}: {exc}")
        offset = end

    if json_document is None:
        fail(f"GLB JSON chunk is missing: {path.name}")
    if json_document.get("asset", {}).get("version") != "2.0":
        fail(f"GLB asset version must be 2.0: {path.name}")
    return json_document


def validate_face_texture(document: dict, pilot: str) -> None:
    materials = document.get("materials", [])
    images = document.get("images", [])
    textures = document.get("textures", [])
    meshes = document.get("meshes", [])

    face_material_index = None
    face_texture_index = None
    for index, material in enumerate(materials):
        if material.get("name") != "face_photo_atlas":
            continue
        face_material_index = index
        face_texture_index = (
            material.get("pbrMetallicRoughness", {})
            .get("baseColorTexture", {})
            .get("index")
        )
        break

    if face_material_index is None or not isinstance(face_texture_index, int):
        fail(f"{pilot} GLB has no face_photo_atlas material with a base color texture")
    if not (0 <= face_texture_index < len(textures)):
        fail(f"{pilot} GLB face texture index is invalid")

    image_index = textures[face_texture_index].get("source")
    if not isinstance(image_index, int) or not (0 <= image_index < len(images)):
        fail(f"{pilot} GLB face texture has no valid image source")
    image = images[image_index]
    if image.get("mimeType") not in {"image/png", "image/jpeg"}:
        fail(f"{pilot} GLB face atlas must be an embedded PNG or JPEG")
    if "bufferView" not in image:
        fail(f"{pilot} GLB face atlas must be embedded in the GLB")

    textured_face_primitive = False
    for mesh in meshes:
        for primitive in mesh.get("primitives", []):
            if primitive.get("material") != face_material_index:
                continue
            attributes = primitive.get("attributes", {})
            if "TEXCOORD_0" not in attributes:
                fail(f"{pilot} face primitive has no TEXCOORD_0 UV coordinates")
            textured_face_primitive = True
    if not textured_face_primitive:
        fail(f"{pilot} GLB does not apply the face atlas to any mesh primitive")


def validate_model_structure(path: Path, pilot: str) -> dict:
    document = inspect_glb(path)
    skins = document.get("skins", [])
    if len(skins) != 1 or len(skins[0].get("joints", [])) != 20:
        fail(f"{pilot} GLB must contain exactly one skin with exactly 20 joints")
    if "inverseBindMatrices" not in skins[0]:
        fail(f"{pilot} GLB skin has no inverse bind matrices")

    joint_indices = set(skins[0]["joints"])
    nodes = document.get("nodes", [])
    if any(not isinstance(index, int) or not (0 <= index < len(nodes)) for index in joint_indices):
        fail(f"{pilot} GLB contains an invalid skin joint index")

    meshes = document.get("meshes", [])
    if not meshes:
        fail(f"{pilot} GLB contains no mesh")
    skinned_primitive_count = 0
    for mesh in meshes:
        for primitive in mesh.get("primitives", []):
            attributes = primitive.get("attributes", {})
            if "POSITION" not in attributes or "NORMAL" not in attributes:
                fail(f"{pilot} GLB contains a mesh primitive without positions or normals")
            if "JOINTS_0" not in attributes or "WEIGHTS_0" not in attributes:
                fail(f"{pilot} GLB contains an unskinned mesh primitive")
            skinned_primitive_count += 1
    if skinned_primitive_count == 0:
        fail(f"{pilot} GLB contains no skinned primitive")

    animation_names = {animation.get("name") for animation in document.get("animations", [])}
    if animation_names != EXPECTED_ANIMATIONS:
        missing = sorted(EXPECTED_ANIMATIONS - animation_names)
        extra = sorted(animation_names - EXPECTED_ANIMATIONS)
        fail(f"{pilot} GLB animation mismatch; missing={missing}, extra={extra}")

    animated_joint_indices: set[int] = set()
    for animation in document.get("animations", []):
        samplers = animation.get("samplers", [])
        channels = animation.get("channels", [])
        if not samplers or not channels:
            fail(f"{pilot} animation {animation.get('name')} has no samplers or channels")
        for channel in channels:
            sampler_index = channel.get("sampler")
            if not isinstance(sampler_index, int) or not (0 <= sampler_index < len(samplers)):
                fail(f"{pilot} animation {animation.get('name')} has an invalid sampler")
            target = channel.get("target", {})
            target_node = target.get("node")
            target_path = target.get("path")
            if target_node not in joint_indices:
                fail(f"{pilot} animation {animation.get('name')} targets a non-skeleton node")
            if target_path not in SUPPORTED_ANIMATION_PATHS:
                fail(f"{pilot} animation {animation.get('name')} has unsupported path {target_path}")
            animated_joint_indices.add(target_node)
    if len(animated_joint_indices) < 6:
        fail(f"{pilot} animations affect too few skeleton joints")

    node_names = {node.get("name") for node in nodes}
    missing_sockets = sorted(REQUIRED_SOCKETS - node_names)
    if missing_sockets:
        fail(f"{pilot} GLB is missing sockets: {', '.join(missing_sockets)}")

    validate_face_texture(document, pilot)
    return {
        "jointCount": len(joint_indices),
        "animationCount": len(animation_names),
        "animatedJointCount": len(animated_joint_indices),
        "skinnedPrimitiveCount": skinned_primitive_count,
        "faceTextureValidated": True,
    }


def main() -> int:
    if len(sys.argv) != 2:
        fail("Usage: python scripts/install_private_character_pack.py /path/to/SpaceKartLegends-vrais-pilotes-rigges-v2.zip")

    archive_path = Path(sys.argv[1]).expanduser().resolve()
    if not archive_path.is_file():
        fail(f"private pack not found: {archive_path}")

    with zipfile.ZipFile(archive_path) as archive:
        validate_archive_members(archive)
        with tempfile.TemporaryDirectory(prefix="spacekart-private-") as temp_dir:
            temp = Path(temp_dir)
            archive.extractall(temp)
            manifest_path = temp / "private_character_manifest_v2.json"
            if not manifest_path.is_file():
                fail("private_character_manifest_v2.json is missing")
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            if manifest.get("version") != 2:
                fail("the private model pack must use version 2")

            models = manifest.get("models", [])
            pilots = {model.get("pilot") for model in models}
            if pilots != EXPECTED_PILOTS or len(models) != len(EXPECTED_PILOTS):
                fail(f"pilot mismatch: {sorted(pilot for pilot in pilots if pilot)}")

            override = {
                "schemaVersion": 2,
                "enabled": True,
                "privacy": "Local private face-derived character pack; excluded from Git.",
                "sourceFormat": "glb",
                "assetProcessor": "O3DE Scene Builder",
                "pilots": {},
                "requiredAnimations": sorted(EXPECTED_ANIMATIONS),
                "requiredSockets": sorted(REQUIRED_SOCKETS),
            }

            for model in models:
                pilot = model["pilot"]
                filename = model["file"]
                relative = PurePosixPath(filename)
                if relative.is_absolute() or ".." in relative.parts:
                    fail(f"unsafe model path in manifest: {filename}")
                source = temp.joinpath(*relative.parts)
                if not source.is_file():
                    fail(f"model file is missing: {filename}")
                if source.suffix.lower() != ".glb":
                    fail(f"unsupported private model format: {filename}")
                if model.get("joints") != 20:
                    fail(f"{pilot} manifest must declare 20 joints")
                if set(model.get("animations", [])) != EXPECTED_ANIMATIONS:
                    fail(f"{pilot} manifest animation set is incomplete")
                if sha256(source) != model.get("sha256"):
                    fail(f"SHA-256 mismatch for {filename}")
                inspection = validate_model_structure(source, pilot)

                pilot_folder = TARGET_ROOT / pilot
                pilot_folder.mkdir(parents=True, exist_ok=True)
                destination = pilot_folder / Path(filename).name
                temporary_destination = destination.with_suffix(destination.suffix + ".tmp")
                shutil.copy2(source, temporary_destination)
                temporary_destination.replace(destination)
                override["pilots"][pilot.lower()] = {
                    "sourceModel": destination.relative_to(ROOT / "Assets").as_posix(),
                    "expectedActorProduct": (pilot_folder / destination.name.replace(".glb", ".actor")).relative_to(ROOT / "Assets").as_posix(),
                    "rigVersion": 2,
                    "sha256": model["sha256"],
                    **inspection,
                }

            OVERRIDE_PATH.parent.mkdir(parents=True, exist_ok=True)
            temporary_override = OVERRIDE_PATH.with_suffix(".json.tmp")
            temporary_override.write_text(json.dumps(override, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
            temporary_override.replace(OVERRIDE_PATH)

    print(f"Installed private character pack in: {TARGET_ROOT}")
    print(f"Wrote local override config: {OVERRIDE_PATH}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
