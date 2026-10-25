# Anno 2070 mod loader

A port of the [Anno 1800 / Anno 117 mod loader](https://github.com/jakobharder/anno-mod-loader)
for **Anno 2070 version 3.00.8051 (64-bit)**. It reuses the upstream XML operations
core with a native loader written for Anno 2070.

Place `version.dll` beside the game executable, with mod packages in `mods/`.
Under Wine, set the DLL override `version=n,b`.

Packages load alphabetically; a leading `-` disables a package. Files under
`data/` and `addondata/` replace game resources; XML rooted at `ModOps` patches
them instead.

Anno 2070 has its own XML structure, so modding guides for Anno 1800 and Anno 117
do not directly apply. Only the core XML operations (`ModOps`) are reused.

Restart after editing mods. Diagnostics are written to `anno2070-modloader.log`.
`ANNO5_MODS_DIR` overrides the mod directory; `ANNO5_MODS_DISABLED=1` disables hooks.

## Building

**Windows:** use an x64 Visual Studio developer shell with CMake and Ninja.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix dist --component Runtime
```

For Clang, configure with `-DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl`.

**NixOS/Linux:** `direnv allow` provides Clang/LLD and xwin. Download the Microsoft
SDK/CRT (accepting its license), then cross-build with the Microsoft ABI:

```sh
xwin --accept-license --manifest cmake/xwin-manifest.json splat --output "$PWD/.xwin"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/clang-msvc.cmake
cmake --build build
```

CI checks MSVC, Windows clang-cl, and the Linux cross-build. Both toolchains use
the static Microsoft runtime. Push a tag matching the CMake version (e.g. `v0.1.0`)
to create a **draft release** with the Nix-built Clang ZIP and SHA-256 checksum
after CI passes. The packaged DLL is also tested on Windows before release.
Tags such as `v0.1.0-beta.1` are marked as prereleases.

## Upstream

`anno2070` is the standalone default branch; `main` mirrors upstream. CMake pins
the XML core from `main` by commit and SHA-256. To update, sync `main` with upstream
and update the pin on `anno2070`; no unrelated-history merge is needed.

## License

[MIT](LICENSE). Thanks to Alexander Guettler and Jakob Harder for the upstream
loader and XML operations core. Third-party notices are included in packages.
