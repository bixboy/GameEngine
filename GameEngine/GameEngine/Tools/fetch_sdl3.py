#!/usr/bin/env python3
"""Download and unpack the official SDL3 development binaries.

This helper avoids checking large binary blobs into the repository.  It
fetches the prebuilt Visual C++ development archive from the SDL project and
extracts the import libraries and DLLs under ``Libs/SDL3-<version>/lib`` so
Visual Studio can link successfully.
"""

from __future__ import annotations

import argparse
import hashlib
import io
import sys
import zipfile
from pathlib import Path
from urllib.request import urlopen

RELEASE_VERSION = "3.2.22"
ARCHIVE_TEMPLATE = "https://github.com/libsdl-org/SDL/releases/download/release-{version}/SDL3-devel-{version}-VC.zip"
KNOWN_HASHES = {
    "3.2.22": "093821fcd2b0eafedc86e93713687136872a6556966db036feba2672f58586ed",
}


def compute_sha256(data: bytes) -> str:
    digest = hashlib.sha256()
    digest.update(data)
    return digest.hexdigest()


def download_archive(version: str) -> bytes:
    url = ARCHIVE_TEMPLATE.format(version=version)
    with urlopen(url) as response:  # nosec: trusted upstream release
        data = response.read()
    return data


def ensure_directory(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--version",
        default=RELEASE_VERSION,
        help="SDL release to download (default: %(default)s)",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Re-download the archive even if libraries are already present.",
    )
    parser.add_argument(
        "--skip-hash",
        action="store_true",
        help="Skip verifying the archive's SHA-256 checksum.",
    )
    args = parser.parse_args(argv)

    repo_root = Path(__file__).resolve().parents[2]
    libs_root = repo_root / "Libs" / f"SDL3-{args.version}"

    existing_lib = next((libs_root / "lib").glob("*/SDL3.lib"), None)
    if existing_lib and not args.force:
        print(f"SDL3 {args.version} libraries already present at {libs_root}. Use --force to re-download.")
        return 0

    print(f"Fetching SDL3 {args.version} development archive...")
    archive_data = download_archive(args.version)

    if not args.skip_hash:
        expected = KNOWN_HASHES.get(args.version)
        if expected is None:
            print("No known checksum for this version; enable --skip-hash to proceed.")
            return 1
        actual = compute_sha256(archive_data)
        if actual != expected:
            print("Checksum mismatch!")
            print(f"  expected: {expected}")
            print(f"  actual:   {actual}")
            return 2

    with zipfile.ZipFile(io.BytesIO(archive_data)) as archive:
        members = [m for m in archive.infolist() if m.filename.startswith(f"SDL3-{args.version}/lib/")]
        if not members:
            print("Archive did not contain any SDL3 libraries.")
            return 3
        for member in members:
            relative = Path(member.filename).relative_to(f"SDL3-{args.version}")
            destination = libs_root / relative
            ensure_directory(destination.parent)
            with archive.open(member) as source, destination.open("wb") as target:
                target.write(source.read())
                print(f"Extracted {destination.relative_to(repo_root)}")

    print("SDL3 libraries ready.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
