# Descent

A small terminal roguelike in C: procedurally generated dungeons, real
line-of-sight/fog of war, turn-based combat, an inventory, and monsters
that get tougher (and stranger) the deeper you go. Beat the dragon on
depth 10 to win.

## Build

Needs a C compiler, CMake, and an ncurses dev package.

**Linux (e.g. Arch):**
```bash
sudo pacman -S base-devel cmake ncurses
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
./descent
```

**Windows (MSYS2 / UCRT64):**
```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake \
          mingw-w64-ucrt-x86_64-ncurses mingw-w64-ucrt-x86_64-make
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make
./descent.exe
```
Run this from an MSYS2 UCRT64 terminal (not cmd.exe) so ANSI/curses
rendering works properly.

## Controls

| Key                | Action                              |
|---------------------|--------------------------------------|
| Arrows / WASD / HJKL | Move (walking into a monster attacks it) |
| `i`                 | Open inventory; press a letter to use a potion/scroll |
| `>`                 | Descend stairs (must be standing on them) |
| `Q`                 | Quit immediately |

Gold, potions, scrolls, weapons, and armor are picked up automatically
by walking over them. Weapons and armor auto-equip only if they're an
upgrade over what you're already carrying.

## Project layout

```
include/game.h       -- all shared types + function declarations
src/map.c            -- dungeon generation, line-of-sight, fog of war
src/entity.c         -- monster templates, spawning, and AI
src/combat.c         -- attack resolution, XP, leveling
src/item.c           -- item spawning, pickup, inventory use
src/game_state.c      -- ties subsystems together, level transitions
src/render.c          -- all ncurses drawing (map/HUD/inventory/game over)
src/main.c            -- input handling and the turn loop
src/test_headless.c   -- logic-only test suite (no ncurses / no TTY needed)
```

## Running the tests

The core logic (map gen, combat math, leveling, item effects, monster
AI) is decoupled from ncurses, so it can be exercised headlessly:
```bash
./build/test_headless
```
This was how the game got verified during development in an
environment with no real terminal for ncurses to attach to — 27
checks across map generation, field of view, combat, leveling, items,
monster spawning, and AI wake-up/attack behavior.

## Notes on scope / things left simple on purpose

- Field of view is a radius + Bresenham line-of-sight check, not full
  recursive shadowcasting — cheaper to write correctly and good enough
  at this map size.
- Monsters use a direct-step-toward-player AI with simple axis
  fallback if the preferred direction is blocked; there's no proper
  pathfinding, so they can occasionally get stuck on complex geometry.
- No save/load — each run is one sitting, start to dragon (or death).
