"""Require a vMAJOR.MINOR.PATCH[-prerelease] tag matching the CMake version."""

import argparse
import pathlib
import re

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("tag")
args = parser.parse_args()
tag = re.fullmatch(r"v(\d+\.\d+\.\d+)(?:-[0-9A-Za-z]+(?:[.-][0-9A-Za-z]+)*)?", args.tag)
if tag is None:
    raise SystemExit("Expected vMAJOR.MINOR.PATCH or vMAJOR.MINOR.PATCH-prerelease")
cmake = (pathlib.Path(__file__).resolve().parent.parent / "CMakeLists.txt").read_text()
version = re.search(r"project\(anno2070_modloader\s+VERSION\s+(\d+\.\d+\.\d+)\b", cmake)
if version is None:
    raise SystemExit("Cannot find the CMake project version")
if tag[1] != version[1]:
    raise SystemExit(f"Tag {args.tag} does not match CMake version {version[1]}")
print(f"Release tag {args.tag} matches CMake version {version[1]}")
