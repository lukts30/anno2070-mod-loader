# Warehouse storage

For **Anno 2070 version 3.00.8051 (64-bit)**.

Increases the level-1 warehouse's storage contribution from **40 to 80 tonnes**.
Copy this `01-warehouse-storage` directory into `mods/` beside the game executable,
then restart the game.

## What it changes

The patch selects existing asset GUID `10035` in `data/config/game/assets.xml`
and replaces `/Values/Warehouse/Capacity`. It adds no goods or buildings.
Upgraded warehouses are separate assets and keep their original values.

## Check it in-game

Start a new Easy Continuous game as Global Trust, press `K` to select the
warehouse, and hover over the storage-capacity indicator:

| | Original | With this mod |
| --- | --- | --- |
| Warehouse contribution | 40 t | 80 t |
| Starting island capacity | 50 t | 90 t |

Verified in-game using the [Clang CI artifact at `7d317e3`](https://github.com/lukts30/anno2070-mod-loader/actions/runs/40239864061).
The field changes the warehouse's contribution, not the island's total capacity.
