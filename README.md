# Anno 2070 mod loader

A native mod loader for **Anno 2070 3.00.8051 (x64)**, using the
[upstream ModOps XML core](https://github.com/jakobharder/anno-mod-loader).

Place `version.dll` and `libmcfgthread-2.dll` beside the game executable, with
mod packages in `mods/`. Under Wine, set the DLL override `version=n,b`.

Packages load alphabetically; a leading `-` disables a package. Files under
`data/` and `addondata/` replace game resources; XML rooted at `ModOps` patches
them instead. Use 2070-specific paths and GUIDs. `modinfo.json` dependency
resolution and build 7792 are not supported.

Restart after editing mods. Diagnostics are written to `anno2070-modloader.log`.
`ANNO5_MODS_DIR` overrides the mod directory; `ANNO5_MODS_DISABLED=1` disables hooks.

## License

[MIT](LICENSE). Thanks to Alexander Guettler and Jakob Harder for the upstream
loader and XML operations core. Third-party notices are included in packages.
