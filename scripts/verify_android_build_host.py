#!/usr/bin/env python3
"""Fail fast when the Android/O3DE self-hosted runner cannot build a real APK."""

from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
from pathlib import Path


def fail(message: str) -> None:
    print(f"ERROR: {message}", file=sys.stderr)
    raise SystemExit(1)


def run(command: list[str]) -> str:
    try:
        completed = subprocess.run(
            command,
            check=True,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
    except (OSError, subprocess.CalledProcessError) as exc:
        fail(f"command failed: {' '.join(command)}: {exc}")
    return completed.stdout.strip()


def first_version(text: str) -> tuple[int, ...]:
    match = re.search(r"(?<!\d)(\d+)(?:\.(\d+))?(?:\.(\d+))?", text)
    if not match:
        fail(f"could not parse a version from: {text[:200]}")
    return tuple(int(part or 0) for part in match.groups())


def require_tool(name: str) -> str:
    path = shutil.which(name)
    if not path:
        fail(f"required tool is not available on PATH: {name}")
    return path


def require_minimum(label: str, actual: tuple[int, ...], minimum: tuple[int, ...]) -> None:
    padded_actual = actual + (0,) * (len(minimum) - len(actual))
    if padded_actual < minimum:
        fail(f"{label} {'.'.join(map(str, actual))} is older than required {'.'.join(map(str, minimum))}")


def main() -> int:
    engine = Path(os.environ.get("O3DE_ENGINE_PATH", ""))
    sdk = Path(os.environ.get("ANDROID_SDK_ROOT", ""))

    if not engine.is_dir():
        fail("O3DE_ENGINE_PATH does not point to an installed engine")
    o3de = engine / "scripts" / "o3de.sh"
    export_script = engine / "scripts" / "o3de" / "ExportScripts" / "export_source_android.py"
    if not os.access(o3de, os.X_OK):
        fail(f"missing executable: {o3de}")
    if not export_script.is_file():
        fail(f"missing Android export script: {export_script}")

    if not sdk.is_dir():
        fail("ANDROID_SDK_ROOT does not point to an Android SDK")

    java = require_tool("java")
    gradle = require_tool("gradle")
    cmake = require_tool("cmake")
    require_tool("ninja")

    java_output = run([java, "-version"])
    java_match = re.search(r'version "(\d+)', java_output)
    if not java_match:
        fail(f"could not parse Java version: {java_output}")
    require_minimum("Java", (int(java_match.group(1)),), (17,))

    gradle_output = run([gradle, "--version"])
    gradle_match = re.search(r"^Gradle\s+(\d+(?:\.\d+){1,2})", gradle_output, re.MULTILINE)
    if not gradle_match:
        fail(f"could not parse Gradle version: {gradle_output[:300]}")
    require_minimum("Gradle", first_version(gradle_match.group(1)), (8, 1))

    cmake_output = run([cmake, "--version"])
    require_minimum("CMake", first_version(cmake_output), (3, 30, 0))

    platform = sdk / "platforms" / "android-34" / "android.jar"
    if not platform.is_file():
        fail("Android SDK platform 34 is not installed")

    ndk_root = sdk / "ndk"
    ndk_candidates = sorted(ndk_root.glob("25.*")) if ndk_root.is_dir() else []
    if not ndk_candidates:
        fail("Android NDK 25.x is not installed")

    build_tools_root = sdk / "build-tools"
    build_tools = sorted((path for path in build_tools_root.iterdir() if path.is_dir()), reverse=True) if build_tools_root.is_dir() else []
    if not build_tools:
        fail("Android SDK build-tools are not installed")
    selected = build_tools[0]
    for tool in ("aapt2", "apksigner", "zipalign"):
        candidate = selected / tool
        if not os.access(candidate, os.X_OK):
            fail(f"missing Android build-tool: {candidate}")

    run([str(o3de), "--help"])
    print(f"Build host ready: Java {java_match.group(1)}, Gradle {gradle_match.group(1)}, {cmake_output.splitlines()[0]}")
    print(f"Android SDK: {sdk}; NDK: {ndk_candidates[-1].name}; build-tools: {selected.name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
