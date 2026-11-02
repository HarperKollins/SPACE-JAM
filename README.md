# SPACE JAM 🚀

**A lightweight, polished 2D arcade space shooter built in C with raylib.**

Repository: [https://github.com/HarperKollins/SPACE-JAM](https://github.com/HarperKollins/SPACE-JAM)  
Author: **Harper Kollins** (<somtochukwucollins8@gmail.com>)

---

## Overview

**SPACE JAM: Episode I — The Last Vector** is a retro-futuristic arcade space survival opera. You pilot the legendary **HK-01 Vanguard** test craft, flown by ace pilot **Harper Kollins**. The peaceful Free Colonies have been overwhelmed by the autocratic **Iron Synod**, a monolithic galactic empire deploying mass-produced war machines. As the last surviving strike vector, you must defend an unarmed civilian refugee convoy fleeing toward the Outer Rim by tearing through imperial blockades, asteroid graveyard belts, and capital dreadnoughts.

---

## Features

- 🎵 **Original Synth & Orchestral Soundtrack** — Seamlessly loopable Star Wars style space opera prologue fanfare (`music_intro.wav`) and high-energy 130 BPM arcade combat dogfight synthwave (`music.wav`).
- 🔊 **Full 16-Bit SFX Suite** — Custom synthesized audio for player laser pews, enemy turbolaser pulses, crunchy detonations, shield hits, power-up chimes, dreadnought war alarms, and menu chimes.
- 🛡️ **Tactical Enemy Combat & Repulsor Shields** — Enemies no longer suicide ram; they dogfight at tactical ranges, evade collisions, and fire their own signature weapons (Dagger darts, Jackal twin plasma, Bulwark heavy shells, Goliath torpedoes).
- 📜 **Cinematic Story Prologue** — Star Wars style opening text crawl setting the stakes before combat, with interactive skip or re-watch anytime from the menu.
- 🚀 **"HK-01 Vanguard" Interceptor** — Ceramic white and crimson red strike fighter with gloss-cyan cockpit canopy, wingtip cannons, and dynamic multi-layered plasma thruster jet.
- 📻 **Tactical Comms Chatter** — In-mission radio transmissions from Fleet Command that provide real-time battlefield intelligence, alerts, and sector status.
- 🌌 **Living Sector Environments** — Parallax deep space with drifting volumetric purple/magenta nebulae, distant cratered moons, twinkling cross-glint stars, and giant glowing planet horizons.
- 👾 **Imperial Synod Fleet** —
  - **Dagger Interceptor:** Fast, predatory chevron wings with crimson leading edges and horizontal red sensor visors.
  - **Jackal Heavy Fighter:** Twin-fuselage split fighter with dual red plasma cannons.
  - **Bulwark Ironclad:** Heavy composite-plated octagonal fortress with dual-barrel turbolaser turret.
  - **Goliath Siege Platform:** Asymmetrical ordnance carrier with pulsing missile launch bays.
- 👑 **Synod Dreadnought "Vindicator"** — Massive Star Destroyer style capital flagship bristling with turbolasers, deflector shields, and an exposed reactor core breach in Phase 2.
- 🔫 **Dynamic Weapon Arsenal** — Single Laser, Twin Double Shot, Rapid Plasma Cannon, and 3-Way Spread Shot.
- ☄️ **Procedural Asteroids** — Physics-based destructible asteroids that dynamically fragment from Large into Medium into Small chunks.
- 💥 **Vibrant Particle System** — Thruster exhaust plumes, weapon impact sparks, energetic debris, screen shake, and radiant explosions.
- ⚡ **Rewarding Power-Ups** — Health Repairs, Kinetic Shields, Rapid Fire, Double Shot, 3-Way Spread, and Temporary Invulnerability.
- 🎮 **Full HUD & Mission Help** — Real-time health gauges, life indicators, power-up timers, boss meters, and an in-game controls HUD overlay (`F1`).
- 🏆 **High Score System** — Local persistent leaderboard saved to disk with checksum data validation.
- ⚙️ **Customizable Settings** — Audio volume controls, fullscreen toggle, screen shake intensity, and particle quality options.
- 🪟 **Comprehensive Menus** — Main Menu, Story Prologue, Controls Screen, In-Game Pause Menu (with resume, restart, settings, and direct desktop quit), and Game Over summary. Full keyboard and mouse support.

---

## Controls

| Action | Primary Key | Secondary / Mouse |
| :--- | :--- | :--- |
| **Thrust Forward** | `W` | `Up Arrow (↑)` |
| **Brake / Reverse** | `S` | `Down Arrow (↓)` |
| **Rotate Left** | `A` | `Left Arrow (←)` |
| **Rotate Right** | `D` | `Right Arrow (→)` |
| **Fire Weapons** | `SPACE` | — |
| **Pause / Leave Game** | `ESC` | Click `[ESC] PAUSE / QUIT` in top-right |
| **Toggle Controls Hint** | `F1` | Click `[F1] CONTROLS` in top-right |
| **Navigate Menus** | `W` / `S` or `↑` / `↓` | Mouse Cursor Hover |
| **Select / Confirm** | `ENTER` or `SPACE` | Left Mouse Click |

---

## Technology Stack

| Component | Technology |
| :--- | :--- |
| **Language** | C (C11 standard) |
| **Graphics & Audio** | raylib 6.2 |
| **Compiler** | GCC (MinGW-w64 UCRT64 / Linux GCC) |
| **Build System** | GNU Make |
| **Target Platforms** | Windows 10/11, Linux |

---

## System Requirements

- **OS:** Windows 10/11 (64-bit) or Linux (x86_64)
- **Processor:** Dual-Core 1.6 GHz or better
- **Memory:** 256 MB RAM
- **Graphics:** OpenGL 3.3 compatible integrated graphics or dedicated GPU
- **Storage:** 50 MB available space

---

## Compilation & Installation

### Windows (MSYS2 MinGW UCRT64)

1. **Install MSYS2** (if not already installed):
   ```powershell
   winget install MSYS2.MSYS2
   ```

2. **Open MSYS2 UCRT64 Terminal and install packages:**
   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-raylib
   ```

3. **Clone and build:**
   ```bash
   git clone https://github.com/HarperKollins/SPACE-JAM.git
   cd SPACE-JAM
   make
   ```

4. **Run the game:**
   ```bash
   make run
   # Or run directly from Windows:
   ./build/spacejam.exe
   ```

### Linux

1. **Install raylib development headers:**
   ```bash
   # Debian / Ubuntu / Mint
   sudo apt install libraylib-dev
   
   # Arch Linux / Manjaro
   sudo pacman -S raylib
   
   # Fedora
   sudo dnf install raylib-devel
   ```

2. **Compile and play:**
   ```bash
   make
   make run
   ```

### Makefile Commands

- `make` — Compiles the release build into `build/spacejam.exe` (or `build/spacejam` on Linux).
- `make run` — Compiles and immediately launches the game.
- `make debug` — Compiles with debugging flags (`-g3 -O0`).
- `make clean` — Cleans all object and dependency files.
- `make help` — Lists all available build options.

---

## Project Structure

```
SPACE_JAM/
├── src/
│   ├── main.c          # Application entry point, window initialization, main loop
│   ├── game.c / .h     # Game state machine, wave progression, coordinator
│   ├── player.c / .h   # Spaceship physics, rotation, damage, weapon systems
│   ├── enemy.c / .h    # Enemy archetypes (Scout, Interceptor, Tank, Bomber, Boss AI)
│   ├── asteroid.c / .h # Asteroid field mechanics, random spawning, splitting logic
│   ├── projectile.c/.h # Fixed-capacity projectile pools (lasers & enemy fire)
│   ├── collision.c / .h# Circle-based collision detection & response
│   ├── particles.c / .h# Particle engine (explosions, engine glow, trails, sparks)
│   ├── powerup.c / .h  # Power-up drops, timers, collection, and render icons
│   ├── world.c / .h    # Multi-layered parallax deep space starfield
│   ├── hud.c / .h      # Heads-Up Display (health, score, wave, boss bar, hints)
│   ├── menu.c / .h     # Main, Controls, Pause, Settings, Game Over menus
│   ├── audio.c / .h    # Audio subsystem with graceful asset degradation
│   ├── highscore.c / .h# High score file I/O with checksum verification
│   ├── settings.c / .h # User preferences I/O (volumes, fullscreen, shake)
│   ├── config.h        # Centralized tunable balance constants & settings
│   └── utils.h         # Shared math, trigonometry, vectors, and color utilities
├── assets/
│   ├── audio/          # Sound effects and music tracks (optional)
│   ├── fonts/          # Custom bitmap/vector fonts (optional)
│   └── textures/       # Graphical assets & icon references
├── build/              # Binary output directory
├── Makefile            # Modular cross-platform build automation script
├── LICENSE             # MIT License
├── README.md           # Documentation and user guide
└── .gitignore          # Git exclusion rules
```

---

## Author & License

Developed by **Harper Kollins** (<somtochukwucollins8@gmail.com>).  
Licensed under the [MIT License](LICENSE).
