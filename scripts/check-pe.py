"""Check release architecture, exports, and absence of companion runtime DLLs."""

import argparse
import pathlib
import re
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("dll", type=pathlib.Path)
parser.add_argument("--readobj", default="llvm-readobj")
args = parser.parse_args()
output = subprocess.check_output(
    [args.readobj, "--file-headers", "--coff-imports", "--coff-exports", str(args.dll)],
    text=True,
)
if "IMAGE_FILE_MACHINE_AMD64" not in output:
    raise SystemExit("Expected an x64 PE image")
imports = re.findall(r"Import \{\s+Name: (\S+)", output)
if not imports:
    raise SystemExit("No PE imports found")
for name in imports:
    if re.search(
        r"mcfg|libgcc|libstdc|libc\+\+|libwinpthread|vcruntime|msvcp|ucrtbase",
        name,
        re.IGNORECASE,
    ):
        raise SystemExit(f"Unexpected dynamic runtime: {name}")
expected = {
    line.strip().split("=", 1)[0]
    for line in (pathlib.Path(__file__).resolve().parent.parent / "src/version.def")
    .read_text()
    .splitlines()
    if "=proxy_" in line
}
actual = set(re.findall(r"Export \{[^}]*?Name: (\S+)", output))
if actual != expected:
    raise SystemExit(
        f"Export mismatch: missing={expected - actual}, extra={actual - expected}"
    )
print("x64 PE; all proxy exports; imports: " + ", ".join(imports))
