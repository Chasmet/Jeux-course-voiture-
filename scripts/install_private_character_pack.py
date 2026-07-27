#!/usr/bin/env python3
"""Install the private Cheikh/Yvane/Nelvyn rig pack into this O3DE project.

The private archive is deliberately kept outside the public repository. This
installer validates the archive, rejects raw photo/video files, verifies hashes
and writes the local override configuration used by the character integration.
"""
from __future__ import annotations

import hashlib
import json
import shutil
import sys
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET_ROOT = ROOT / "Assets" / "PrivateCharacters"
OVERRIDE_PATH = ROOT / "Assets" / "Config" / "private_character_overrides.json"
EXPECTED_PILOTS = {"Cheikh", "Yvane", "Nelvyn"}
EXPECTED_ANIMATIONS = {
    "drive_idle", "steer_left", "steer_right", "drift_left", "drift_right",
    "boost", "brake", "jump", "land", "impact_left", "impact_right",
    "spin", "victory",
}
FORBIDDEN_SUFFIXES = {".jpg", ".jpeg", ".webp", ".mp4", ".mov", ".m4v"}


def fail(message: str) -> None:
    raise SystemExit(f"ERROR: {message}")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> int:
    if len(sys.argv) != 2:
        fail("Usage: python scripts/install_private_character_pack.py /path/to/SpaceKartLegends-vrais-pilotes-rigges-v2.zip")

    archive_path = Path(sys.argv[1]).expanduser().resolve()
    if not archive_path.is_file():
        fail(f"private pack not found: {archive_path}")

    with zipfile.ZipFile(archive_path) as archive:
        names = archive.namelist()
        forbidden = [name for name in names if Path(name).suffix.lower() in FORBIDDEN_SUFFIXES]
        if forbidden:
            fail("raw media is forbidden in the private model pack: " + ", ".join(forbidden))

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
            if pilots != EXPECTED_PILOTS:
                fail(f"pilot mismatch: {sorted(pilots)}")

            override = {
                "schemaVersion": 1,
                "enabled": True,
                "privacy": "Local private face-derived character pack; excluded from Git.",
                "pilots": {},
                "requiredAnimations": sorted(EXPECTED_ANIMATIONS),
                "requiredSockets": [
                    "kart_seat_socket",
                    "steering_left_socket",
                    "steering_right_socket",
                    "face_camera_socket",
                ],
            }

            for model in models:
                pilot = model["pilot"]
                filename = model["file"]
                source = temp / filename
                if not source.is_file():
                    fail(f"model file is missing: {filename}")
                if source.suffix.lower() != ".glb":
                    fail(f"unsupported private model format: {filename}")
                if model.get("joints") != 20:
                    fail(f"{pilot} must contain 20 joints")
                if set(model.get("animations", [])) != EXPECTED_ANIMATIONS:
                    fail(f"{pilot} animation set is incomplete")
                if sha256(source) != model.get("sha256"):
                    fail(f"SHA-256 mismatch for {filename}")

                pilot_folder = TARGET_ROOT / pilot
                pilot_folder.mkdir(parents=True, exist_ok=True)
                destination = pilot_folder / filename
                shutil.copy2(source, destination)
                override["pilots"][pilot.lower()] = {
                    "sourceModel": destination.relative_to(ROOT / "Assets").as_posix(),
                    "targetActor": (pilot_folder / filename.replace(".glb", ".actor")).relative_to(ROOT / "Assets").as_posix(),
                    "rigVersion": 2,
                    "jointCount": 20,
                    "sha256": model["sha256"],
                }

            OVERRIDE_PATH.write_text(json.dumps(override, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    print(f"Installed private character pack in: {TARGET_ROOT}")
    print(f"Wrote local override config: {OVERRIDE_PATH}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
