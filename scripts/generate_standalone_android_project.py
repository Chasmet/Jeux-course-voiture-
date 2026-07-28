#!/usr/bin/env python3
"""Generate the standalone Android/OpenGL ES project for Space Kart Legends.

The embedded payload contains only public source code and placeholder tools.
Private face-derived pilot models are never stored in the public repository.
"""
from __future__ import annotations

import base64
import io
import tarfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PARTS = ROOT / "scripts" / "android_project_payload"


def main() -> int:
    encoded = "".join(path.read_text(encoding="ascii").strip() for path in sorted(PARTS.glob("part*.txt")))
    if not encoded:
        raise SystemExit("Android project payload is missing")
    payload = base64.b64decode(encoded)
    with tarfile.open(fileobj=io.BytesIO(payload), mode="r:gz") as archive:
        for member in archive.getmembers():
            destination = (ROOT / member.name).resolve()
            if ROOT not in destination.parents and destination != ROOT:
                raise SystemExit(f"Unsafe embedded path: {member.name}")
        archive.extractall(ROOT, filter="data")
    print("Generated android-standalone project.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
