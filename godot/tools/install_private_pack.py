#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import shutil
import sys
import tempfile
import zipfile
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "private_models"
EXPECTED = {
    "Cheikh": "cheikh.glb",
    "Yvane": "yvane.glb",
    "Nelvyn": "nelvyn.glb",
}
FORBIDDEN = {".jpg", ".jpeg", ".webp", ".mp4", ".mov", ".m4v"}


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
        fail("Usage: install_private_pack.py /path/to/private-pack.zip")
    archive_path = Path(sys.argv[1]).expanduser().resolve()
    if not archive_path.is_file():
        fail(f"Archive not found: {archive_path}")

    with zipfile.ZipFile(archive_path) as archive:
        for info in archive.infolist():
            path = PurePosixPath(info.filename.replace("\\", "/"))
            if path.is_absolute() or ".." in path.parts:
                fail(f"Unsafe archive path: {info.filename}")
            if path.suffix.lower() in FORBIDDEN:
                fail(f"Raw personal media is forbidden: {info.filename}")

        with tempfile.TemporaryDirectory(prefix="spacekart-godot-private-") as temp_dir:
            temp = Path(temp_dir)
            archive.extractall(temp)
            manifest_path = temp / "private_character_manifest_v2.json"
            if not manifest_path.is_file():
                fail("private_character_manifest_v2.json is missing")
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            if manifest.get("version") != 2:
                fail("Private pack version must be 2")
            models = manifest.get("models", [])
            by_pilot = {entry.get("pilot"): entry for entry in models}
            if set(by_pilot) != set(EXPECTED):
                fail(f"Unexpected pilots: {sorted(by_pilot)}")

            TARGET.mkdir(parents=True, exist_ok=True)
            for pilot, output_name in EXPECTED.items():
                entry = by_pilot[pilot]
                source = temp / entry["file"]
                if not source.is_file() or source.suffix.lower() != ".glb":
                    fail(f"Invalid model for {pilot}")
                if source.read_bytes()[:4] != b"glTF":
                    fail(f"{pilot} is not a GLB file")
                if sha256(source) != entry.get("sha256"):
                    fail(f"SHA-256 mismatch for {pilot}")
                destination = TARGET / output_name
                shutil.copy2(source, destination)
                print(f"Installed {pilot}: {destination.name}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
