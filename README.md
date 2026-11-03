# SPACE JAM 🚀

**A cinematic, polished 2D arcade space shooter & space opera built in C with raylib.**

Repository: [https://github.com/HarperKollins/SPACE-JAM](https://github.com/HarperKollins/SPACE-JAM)  
Author: **Harper Kollins** (<somtochukwucollins8@gmail.com>)

---

## The Saga of Space Jam

**SPACE JAM: Episode I — The Last Vector** is an epic retro-futuristic arcade space survival opera created by **Harper Kollins**. 

You pilot the legendary **HK-01 Vanguard** prototype strike craft, flown by ace aviator **Harper Kollins**. The peaceful Free Colonies of the Orion Reach have been shattered by the autocratic **Iron Synod**, a monolithic galactic empire deploying an endless armada of mass-produced war machines. As the last surviving strike vector, you are the sole escort protecting an unarmed civilian refugee convoy fleeing toward the Outer Rim.

### Official Universe Documents & Story Bible

- 📖 **[STORY_BIBLE.md](STORY_BIBLE.md)** — **The Complete Universe, Lore & Franchise Bible** (Parts 1–22): Galactic history, The Architects, The Hollow, The Echo, The Seven Vectors, Factions, The Odyssey, Crew, and Tech.
- 🎬 **[EPISODE_1_SCRIPT.md](EPISODE_1_SCRIPT.md)** — **The Complete Episode I Script**: All 29 chapters, full character dialogues, Act I to Act III, Climax, and the 3 Canonical Endings.
- 📐 **[docs/SYSTEMS_ARCHITECTURE.md](docs/SYSTEMS_ARCHITECTURE.md)** — High-performance zero-heap C architecture, memory pools, relic state machine, and asset pipeline.
- 🌌 **[STORY.md](STORY.md)** — The quick-start universe overview and sector campaign guide.

---

## Features

- 🌀 **The Seven Vectors Relic System** — Unlock and channel ancient Architect cosmic artifacts:
  - **Gravity Vector (`Q`):** Spawns a collapsing gravitational singularity crushing hostile formations.
  - **Temporal Vector (`E`):** Chrono-Stasis rewinds player hull by +35 HP and slows hostile time to 25%.
  - **Void Vector (`F`):** Phase Dematerialization grants ethereal immunity to collisions for 3.5s.
  - **Genesis Vector:** Passive +35% weapon damage overcharge and hyper-velocity piercing.
  - **Ascendant Vector:** The mystical 7th key recovered from the Citadel Flagship, unlocking the secret True Ending.
- 🌌 **Act III Climax & Three Canonical Endings** —
  - **Ending A — The Human Future:** Shatter the Genesis Engine; Earth is saved and humanity charts its own destiny.
  - **Ending B — The Eternal Order:** Claim the Synod Core; cold, everlasting peace across 40,000 worlds without freedom.
  - **Ending C — The Last Vector (Canonical True Ending):** Transcendent ascension; Harper's consciousness becomes the immortal bridge guarding all living minds from The Hollow.
- 🎭 **Cinematic Universe & Tactical Comms System** — Star Wars style opening crawl followed by an interactive **Tactical Military Radio Comms Transmission** (Freq 104.8 MHz) featuring custom character portraits (Harper Kollins, Commander Vance, Nadia, The Iron Archon, Dr. Amara, Mira), authentic radio static, typewriter dialogue, and battle orders.
- 🪐 **Four Dynamic Celestial Sectors** —
  - **Sector 1 (Waves 1–3):** *Shattered Terra Novus* — Defend launching civilian refugee shuttles above a burning cradle world with glowing lava fissures.
  - **Sector 2 (Waves 4–6):** *The Solar Corona* — Dogfight inside the blazing solar furnace of *Prometheus* with looping solar flares and heat waves.
  - **Sector 3 (Waves 7–9):** *Ice Rings of Xylos* — Navigate through dense concentric frozen ring fields orbiting a massive emerald gas giant.
  - **Sector 4 (Waves 10–12+):** *The Synod Citadel* — Assault the moon-sized orbital fortress ring world in pure obsidian space.
- 🛸 **Physical Ship Evolution (HK-01 Upgrades)** — Your starfighter visibly evolves as you clear sectors: *Mk I (Vanguard Interceptor)* → *Mk II (Armored Striker)* with titanium prow armor → *Mk III (Plasma Ace)* with cyan plasma conduits → *Mk IV (Ascendant Legend)* with royal gold trim and radiant ion particle aura.
- 👾 **Epic Sector Climax Bosses** —
  - **Wave 3:** *Commander Varek // Synod Cruiser 'Reaver'*
  - **Wave 6:** *Inquisitor Malakor // Solar Siege Platform 'Helios'*
  - **Wave 9:** *High Admiral Zephyr // Stealth Carrier 'Void Stalker'*
  - **Wave 12:** *The Iron Archon // Citadel Flagship 'Apocalypse'* (multi-phase health bar and exposed reactor core breach).
- 🛡️ **Civilian Refugee Escorts** — Unarmed colony evacuation shuttles launch in Sector 1. Escort them safely across the sector to earn +500 bonus points and military honors.
- 🏅 **Military Service Record & Medals System** — Persistent progress saved to disk (`spacejam_save.dat`) tracking lifetime kills, shuttles saved, bosses crushed, military ranks (`Cadet` to `Vanguard Legend`), and 8 campaign valor medals.
- ⚡ **Hyper-Nitro Engine Flex** — High-speed propulsion mechanics with live engine glow, 2.5D bank tilting, cockpit throttle rumble, and continuous overdrive boost.
- 🎵 **Original Synth & Orchestral Soundtrack** — Seamlessly loopable space opera prologue fanfare (`music_intro.wav`) and high-energy 130 BPM arcade combat dogfight synthwave (`music.wav`).
- 🔊 **Full 16-Bit SFX Suite** — Custom synthesized audio for player laser pews, enemy turbolaser pulses, crunchy detonations, shield hits, power-up chimes, dreadnought alarms, radio static/chirps/beeps, warp jumps, and medal fanfares.
- 🎯 **Tactical Enemy Combat & Signature Lasers** — Scouts fire swift cyan needles, Interceptors fire twin crimson plasma, Tanks fire heavy amber flak, and Bombers launch tracking violet torpedoes.
- 💥 **Dynamic Weapon Arsenal** — Single Laser, Twin Double Shot, Rapid Plasma Cannon, and 5-Way Starburst Array.
- ☄️ **Procedural Asteroids** — Physics-based destructible asteroids that dynamically fragment from Large into Medium into Small chunks.
- ✨ **Vibrant Particle System** — Thruster exhaust plumes, weapon impact sparks, energetic debris, screen shake, and radiant explosions.
- 💎 **Rewarding Power-Ups** — Health Repairs, Kinetic Shields, Rapid Fire, Double Shot, Spread Array, and Temporary Invulnerability.
- 🖥️ **Full HUD & Mission Help** — Real-time health gauges, life indicators, power-up timers, boss meters, and an in-game controls HUD overlay (`F1`).
- 🏆 **High Score System** — Local persistent leaderboard saved to disk with checksum data validation.
- ⚙️ **Customizable Settings** — Audio volume controls, fullscreen toggle, screen shake intensity, and particle quality options.
- 🎮 **Comprehensive Menus** — Main Menu, Story Prologue, Controls Screen, In-Game Pause Menu, and Game Over summary. Full keyboard and mouse support.

---

## Controls

| Action | Primary Key | Secondary / Mouse |
| :--- | :--- | :--- |
| **Thrust Forward** | `W` | `Up Arrow` |
| **Hyper-Nitro Overdrive** | `LEFT SHIFT` / `RIGHT SHIFT` | `Right Mouse Button` |
| **Brake / Reverse** | `S` | `Down Arrow` |
| **Rotate Left** | `A` | `Left Arrow` |
| **Rotate Right** | `D` | `Right Arrow` |
| **Fire Weapons** | `SPACE` | `Left Mouse Button` |
| **Relic: Gravity Singularity** | `Q` | Crushes enemies in 360px gravity vortex |
| **Relic: Chrono-Stasis** | `E` | Rewinds +35 HP, slows hostiles to 25% |
| **Relic: Void Phasing** | `F` | 3.5s ethereal immunity to physical collisions |
| **Pause / Leave Game** | `ESC` | Click `[ESC] PAUSE / QUIT` in top-right |
| **Toggle Controls Hint** | `F1` | Click `[F1] CONTROLS` in top-right |
| **Navigate Menus** | `W` / `S` or `UP` / `DOWN` | Mouse Hover |
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
   mingw32-make
   ```

4. **Run the game:**
   ```bash
   ./build/spacejam.exe
   ```

---

## Project Structure

```
SPACE_JAM/
├── src/
│   ├── main.c          # Application entry point, window initialization, main loop
│   ├── game.c / .h     # Game state machine, wave progression, coordinator
│   ├── player.c / .h   # Spaceship physics, rotation, damage, weapon systems, ship tiers
│   ├── enemy.c / .h    # Enemy archetypes (Scout, Interceptor, Tank, Bomber, Sector Bosses)
│   ├── asteroid.c / .h # Asteroid field mechanics, random spawning, splitting logic
│   ├── projectile.c/.h # Projectile pools (lasers, plasma, torpedos, faction colors)
│   ├── collision.c / .h# Collision detection & resolution
│   ├── particles.c / .h# Particle engine (explosions, exhaust, nitro sparks)
│   ├── powerup.c / .h  # Power-up drops, collection, timers
│   ├── world.c / .h    # 4 Planetary Sectors, parallax stars, hyperspace warp effect
│   ├── hud.c / .h      # Heads-Up Display (health, score, wave, boss bar, medals)
│   ├── menu.c / .h     # Main, Prologue crawl, Comms cutscene with portraits, Medals, Pause
│   ├── progress.c / .h # Medals & career progression system (spacejam_save.dat)
│   ├── audio.c / .h    # Audio subsystem with multi-path resolution & fallback synthesis
│   ├── highscore.c / .h# High score file I/O with checksum verification
│   ├── settings.c / .h # User preferences I/O (volumes, fullscreen, shake)
│   ├── config.h        # Centralized tunable balance constants & settings
│   └── utils.h         # Shared math, trigonometry, vectors, and color utilities
├── assets/
│   ├── audio/          # Sound effects and music tracks (.wav)
│   └── portraits/      # High-res character portraits (Harper, Vance, Archon, Nadia, Amara, Mira)
├── STORY_BIBLE.md      # Master Universe Bible & Lore (Parts 1–22)
├── EPISODE_1_SCRIPT.md # Complete Episode I Script (All 29 chapters & 3 endings)
├── STORY.md            # Quick-start story overview
├── build/              # Binary output directory
├── Makefile            # Modular cross-platform build automation script
├── LICENSE             # MIT License
└── README.md           # Documentation and user guide
```

---

## Author & License

Created and developed by **Harper Kollins** (<somtochukwucollins8@gmail.com>).  
Licensed under the [MIT License](LICENSE).
