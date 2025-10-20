#!/usr/bin/env python3
"""Download prebuilt SDL3 developer libraries for Visual Studio toolchains."""
from __future__ import annotations

import argparse
import io
import shutil
import sys
import zipfile
from pathlib import Path
from urllib.request import urlopen

RELEASE_VERSION = "3.2.2"
RELEASE_TAG = f"release-{RELEASE_VERSION}"
ARCHIVE_NAME = f"SDL3-devel-{RELEASE_VERSION}-VC.zip"
DOWNLOAD_URL = f"https://github.com/libsdl-org/SDL/releases/download/{RELEASE_TAG}/{ARCHIVE_NAME}"

ARCH_ALIASES = {
    "x64": "x64",
    "amd64": "x64",
    "win32": "x86",
    "x86": "x86",
}

COMPATIBILITY_ALIASES = {
    "x86": {"Win32"},
    "x64": {"Win64", "x86_64"},
}

ROOT = Path(__file__).resolve().parents[1]
SDL_VENDOR = ROOT / "ThirdParty" / "SDL3-3.2.22"
LIB_DIR = SDL_VENDOR / "lib"
BIN_DIR = SDL_VENDOR / "bin"

FILES_TO_COPY = {
    "lib": {"SDL3.lib", "SDL3_test.lib"},
    "bin": {"SDL3.dll", "SDL3.pdb"},
}


def _normalize_arch(value: str) -> str:
    key = value.lower()
    if key == "all":
        return "all"
    if key not in ARCH_ALIASES:
        valid = ", ".join(sorted(set(ARCH_ALIASES) | {"all"}))
        raise argparse.ArgumentTypeError(f"Unsupported architecture '{value}'. Valid options: {valid}.")
    return ARCH_ALIASES[key]


def download_archive() -> bytes:
    print(f"Downloading SDL3 {RELEASE_VERSION} developer package...", flush=True)
    with urlopen(DOWNLOAD_URL) as response:
        if response.status != 200:
            raise RuntimeError(f"Failed to download SDL3 archive: HTTP {response.status}")
        return response.read()


def extract_archives(buffer: bytes, arch: str) -> None:
    with zipfile.ZipFile(io.BytesIO(buffer)) as archive:
        for target_arch in (["x86", "x64"] if arch == "all" else [arch]):
            lib_target = LIB_DIR / target_arch
            bin_target = BIN_DIR / target_arch
            lib_target.mkdir(parents=True, exist_ok=True)
            bin_target.mkdir(parents=True, exist_ok=True)
            prefix = f"SDL3-{RELEASE_VERSION}/"
            lib_prefix = f"{prefix}lib/{target_arch}/"
            bin_prefix = f"{prefix}bin/{target_arch}/"

            for member in archive.namelist():
                if member.startswith(lib_prefix):
                    filename = Path(member).name
                    if filename in FILES_TO_COPY["lib"]:
                        dest = lib_target / filename
                        if dest.exists():
                            continue
                        print(f"Extracting {filename} -> {dest.relative_to(ROOT)}")
                        with archive.open(member) as source, dest.open("wb") as target:
                            target.write(source.read())
                if member.startswith(bin_prefix):
                    filename = Path(member).name
                    if filename in FILES_TO_COPY["bin"]:
                        dest = bin_target / filename
                        if dest.exists():
                            continue
                        print(f"Extracting {filename} -> {dest.relative_to(ROOT)}")
                        with archive.open(member) as source, dest.open("wb") as target:
                            target.write(source.read())

            _mirror_aliases(target_arch)


def _mirror_aliases(arch: str) -> None:
    lib_source = LIB_DIR / arch
    bin_source = BIN_DIR / arch

    for alias in COMPATIBILITY_ALIASES.get(arch, set()):
        _copy_contents(lib_source, LIB_DIR / alias)
        _copy_contents(bin_source, BIN_DIR / alias)


def _copy_contents(source: Path, destination: Path) -> None:
    if not source.exists():
        return
    destination.mkdir(parents=True, exist_ok=True)
    for entry in source.iterdir():
        if entry.is_file():
            shutil.copy2(entry, destination / entry.name)


def ensure_archives(arch: str) -> None:
    targets = ["x86", "x64"] if arch == "all" else [arch]

    missing = False
    for target_arch in targets:
        for name in FILES_TO_COPY["lib"]:
            if not (LIB_DIR / target_arch / name).exists():
                missing = True
        for name in FILES_TO_COPY["bin"]:
            if not (BIN_DIR / target_arch / name).exists():
                missing = True
    if not missing:
        for target_arch in targets:
            _mirror_aliases(target_arch)
        print("SDL3 libraries already present. Nothing to do.")
        return

    data = download_archive()
    extract_archives(data, arch)
    print("SDL3 libraries successfully installed.")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--arch",
        type=_normalize_arch,
        default="all",
        help="Target architecture to install (x86, x64, or all). Defaults to all.",
    )
    args = parser.parse_args(argv)

    if not SDL_VENDOR.exists():
        parser.error(f"SDL3 vendor directory not found: {SDL_VENDOR}")

    ensure_archives(args.arch)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv[1:]))
    except Exception as exc:  # pragma: no cover - setup helper
        print(f"error: {exc}", file=sys.stderr)
        sys.exit(1)
