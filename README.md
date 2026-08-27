# Arcade Car Soccer

A third-person, 3D, arcade car-soccer game for Linux — rocket-powered cars playing football inside an
enclosed futuristic arena. Native C++ with [raylib](https://github.com/raysan5/raylib) for graphics,
[Jolt](https://github.com/jrouwe/JoltPhysics) for physics, [glm](https://github.com/g-truc/glm) for
maths and [Dear ImGui](https://github.com/ocornut/imgui) for the in-engine tuning tools.

## The game

The game opens fullscreen. Pick one of six low-poly cars, then kick off in a 76.81 x 102.41 m
glass-walled arena with a 20.73 m ceiling and Rocket League's own goal, 17.86 x 6.43 x 8.80 m. Drive, boost, jump,
double-jump, flip and fly: every edge of the arena is ramped like Rocket League, so you can carry a
wall and, with enough boost, cross the ceiling. Put the heavy-but-bouncy ball fully across the
opponent's goal line to score. The match runs on a clock, a goal resets the field with a kickoff
countdown, and boost is a 0-100 meter drained by holding Shift and refilled by 18 pads spread around
the pitch. **C** swaps between the chase camera and a ball cam that keeps the ball and the car lined
up in frame.

Priority is fun over realism: the car is a single box body driven by arcade forces rather than a
simulated suspension, so acceleration is strong, cornering is drift-friendly and the car rights
itself after a tumble.

The game opens on a title screen — the badge and wordmark with a "press any button" prompt, over a live view of the
arena — and any key, mouse button or gamepad button takes you to the main menu, which is itself a
showcase: a random car turntabling on the pitch with the menu down the left. It carries a
**How to play** screen with the rules and the full control list.

An orange bot opponent chases the ball and shoots at your goal. It can be switched off in Settings,
which turns the match into solo practice with both goals still scoring.

## Controls

### Driving

| Input | Action |
|---|---|
| **W / S** or **Up / Down** | Accelerate / brake and reverse |
| **A / D** or **Left / Right** | Steer |
| **Space** | Jump — press again in the air for a double jump, or a flip if a direction is held |
| **Shift** (hold) | Boost |
| **W / A / S / D** (airborne) | Pitch and yaw |
| **Q / E** (airborne) | Air roll |
| **C** | Toggle chase cam / ball cam |
| **R** | Reset the car to its spawn |
| **Esc** or **P** | Pause |
| **F1** | Tuning panel (Debug and Development builds only) — freezes the match while open |

### Menus

| Input | Action |
|---|---|
| **Up / Down** or **W / S** | Move between rows |
| **Left / Right** or **A / D** | Change a setting's value |
| **Enter** / **Space** | Activate |
| **Esc** | Close the settings panel / go back |
| Mouse | Hover to select, click to activate |
| Any key / mouse / gamepad button | Leave the title screen |

### Full time

| Input | Action |
|---|---|
| **Enter** | Rematch |
| Mouse | REMATCH or MAIN MENU |
| **Esc** or **P** | Pause menu, to leave any other way |

### Car picker

| Input | Action |
|---|---|
| **Arrows** or **WASD** | Move around the 2 x 3 grid |
| **Enter** / **Space** | Start the match with the highlighted car |
| **Esc** | Back to the main menu |
| Right click | Pick a car |
| **Left click a car** | Pick it and start the match in one go |

## How to run

### Requirements

- Linux, GCC with C++17, `make`, Python 3 (the asset cooker runs as a build event).
- OpenGL development libraries and X11 (raylib's own dependencies).

### Get the third-party libraries

They are not vendored in this repository. Clone them into `Game/ThirdParty/` at the pinned versions —
mismatching them, Jolt especially, causes silent ABI breakage:

```sh
git clone --branch 6.0            --depth 1 https://github.com/raysan5/raylib.git      Game/ThirdParty/raylib
git clone --branch v5.6.0         --depth 1 https://github.com/jrouwe/JoltPhysics.git  Game/ThirdParty/Jolt
git clone --branch 1.0.3          --depth 1 https://github.com/g-truc/glm.git          Game/ThirdParty/glm
git clone --branch v1.92.9b-docking --depth 1 https://github.com/ocornut/imgui.git     Game/ThirdParty/imgui
```

### Build

```sh
make                 # Development (default)
make debug           # or: make development / make release / make all
```

The first build takes a few minutes (raylib plus 153 Jolt translation units per configuration); after
that it is incremental. `make clean` removes the build output, `make clean-thirdparty` also forces a
raylib rebuild.

Each configuration lands in `Build/<Config>/` next to an `assets/` folder of cooked models, textures
and shaders, written by `Tools/AssetCooker.py` as part of the build.

The game opens fullscreen at the monitor's own resolution; **Fullscreen** in Settings turns it back
into a 1280x720 window. `--smoke-test` always runs windowed, so its screenshots are the same size on
every machine.

| Configuration | Debug symbols | ImGui tuning tools | Optimisation |
|---|:---:|:---:|---|
| Debug | yes | yes | `-O0` |
| Development | yes | yes | `-O2` |
| Release | no | no | `-O3` |

### Windows build

Linux is the platform the game is developed and verified on, but the `Makefile` can also produce
a Windows executable. Cross-compiling it from Linux needs `mingw-w64-gcc`:

```sh
make -C Game/ThirdParty/raylib/src clean      # only when switching toolchains
make release TARGET_OS=Windows \
     CXX=x86_64-w64-mingw32-g++ CC=x86_64-w64-mingw32-gcc AR=x86_64-w64-mingw32-ar
```

The result is `Build/Windows/Release/ArcadeCarSoccer.exe` with its `assets/` folder beside it —
copy both together, and nothing else: the MinGW runtime is linked statically. Building natively on
Windows works the same way without the toolchain variables, but needs MSYS2 or Git Bash, since the
`Makefile` uses `find` and `rm -rf`. Use a `release`/`development`/`debug` goal rather than a bare
`CONFIG=`, which the default goal overrides. Full notes, including macOS, are in
[docs/CrossPlatformBuild.md](docs/CrossPlatformBuild.md).

### Soundtrack

Six tracks in `Game/Assets/Sounds/` are copied into each build's `assets/Music/` and played as a
shuffled playlist: one streams at a time and a new one starts whenever it ends, never the same track
twice in a row. It runs through the whole game, menus included, on its own **Music volume** slider in
Settings. Adding or removing a track is dropping a `.mp3` in or out of that folder - nothing lists
them in code.

### Run

```sh
make run CONFIG=Development
# or directly
./Build/Release/ArcadeCarSoccer
```

Headless-ish validation — render N frames, write a screenshot and exit:

```sh
./Build/Release/ArcadeCarSoccer --smoke-test 60 --screenshot SmokeTest.png
```

The asset cooker needs **Pillow** to cook textures. Either have it on the system `python3`, or create
the environment with `python3 -m venv .venv && .venv/bin/pip install -r Tools/requirements.txt` — the
Makefile picks the venv up automatically when it exists. Without it the texture step fails the build
and says so; the models and shaders are unaffected.

## Milestones

Tracked against the milestone list in [CLAUDE.md](CLAUDE.md); implementation notes, measurements and
design decisions live in [HANDOFF.md](HANDOFF.md).

- [x] **01 — Project Structure** — all three configurations build, the asset cooker runs as a build event.
- [x] **02 — Base Scene and Car Movement** — arena floor, chase camera, a car on a single Jolt rigid body.
- [x] **03 — Main Menu and Pause Menu** — Play / Settings / Exit, Esc-or-P pause overlay, one settings panel shared by both.
- [x] **04 — Car Handling and Feel** — 0-100 km/h in 1.63 s, 32 m/s top speed, rights itself from upside down in 0.78 s.
- [x] **05 — Ball and Car-Ball Interaction** — heavy sphere with its own gravity factor; a top-speed hit sends it out at 1.19x the car's speed.
- [x] **06 — Arena, Goals and Scoring** — enclosed arena, two goals, exact "fully across the line" detection, clock, kickoff countdown and reset.
- [x] **07 — Boost System and Boost Pads** — 0-100 meter, 3 s of boost from full, 18 pads with cooldown and glow.
- [x] **08 — Car Selection** — six of the seven cooked cars on a 2 x 3 grid, shown after Play; the pick carries into the match.
- [x] **09 — Jumps, Flips and Air Control** — single and double jump, directional flips, pitch/yaw/roll in the air, aerial ball touches.
- [x] **10 — Camera Modes** — smooth chase cam, ball cam on **C**, and the camera keeps itself out of the arena geometry by ray cast rather than clipping through it.
- [x] **11 — HUD** — scoreboard, clock, boost meter, speed, kickoff countdown, goal banner and a full-time screen with a rematch. Boost-pad state is read from the pads' own glow in the world, not from a HUD hint.
- [x] **12 — Bot Opponent (or Solo Practice)** — an orange bot that drives at the ball from the side its shot comes from, boosts down the long approaches and frees itself when it gets blocked. 6-1 against a player who never moves. Switchable off for solo practice.
- [x] **13 — Tuning Panels (Debug/Development)** — **F1** opens a Dear ImGui panel over the match with live sliders for gravity, the ball, car handling, boost and camera smoothing, and a Save button writing `Tuning.cfg` next to the executable, which the next match loads. Release contains none of it.
- [x] **14 — Visual Polish and Effects** — contact shadows, a boost flame and ember trail, a ball highlight, particle bursts on goals, big hits and jumps, a goal screen flash, blocky stands and light rigs outside the glass, and toggleable bloom that costs about 0.1 ms a frame.
- [x] **15 — Audio** — ten procedural cues generated at startup from one synthesis table (no sound files): UI hover and click, jump, car-ball hit, wall/car impact, boost pad, countdown ticks, kickoff go, goal and full time, plus a boost roar held on a running audio stream. Master and SFX volumes are live from the settings panel.
- [x] **16 — Arena Field Dimensions** — the arena now follows the supplied Rocket League reference at 76.81 m wide, 102.41 m goal-to-goal and 20.73 m floor-to-ceiling.
- [x] **17 — Title Screen ("Press Any Button")** — the game now boots on a title card over a live view of the real arena: the cooked badge beside the BUDGET LEAGUE wordmark, a pulsing prompt, and the ball resting on the pitch beside the far goal. Any key, mouse button or gamepad button goes to the main menu.
- [x] **18 — Main Menu Showcase** — the main menu is now a live showcase: the real arena behind it, a random one of the seven cooked cars parked on the pitch in a random team colour on a slow turntable, and the menu list down the left. Re-rolled every time the menu is entered.
- [x] **19 — Settings Screen (main-menu layout)** — opened from the main menu the panel is centred on both axes with no game title above it, over the still-running showcase; opened from the pause menu it is unchanged. The one shared widget takes a background flag.
- [x] **20 — Soundtrack (OST playlist)** — the six tracks in `Game/Assets/Sounds/` are copied into each build and streamed as a shuffled playlist: a new song starts whenever one ends, never the same one twice in a row, with its own Music volume slider.
- [x] **21 — Cross-Platform Builds** — a `Makefile`-only change, with no game code touched: `TARGET_OS` picks the link line, the `.exe` suffix and raylib's platform. The Windows executable is built and verified by cross-compiling from Linux with `x86_64-w64-mingw32` and running its smoke test under Wine; it links the MinGW runtime statically, so only system DLLs are needed. macOS support is written but **not built** — there is no Mac in this project's environment. See [docs/CrossPlatformBuild.md](docs/CrossPlatformBuild.md).

### Final milestone — polish

- [x] Minor bumps never flip the car, while intentional flips stay snappy — a hard sideways kick at speed costs the car under 2.5 degrees of lean and 0.12 s, and a flip is now a committed 0.70 s rotation that sweeps a full turn and comes out level instead of stalling half way round.
- [x] The ball is heavy but not sluggish; big hits carry — a hit leaves at 1.13-1.19x the car's speed and a top-speed one crosses 50 m, while a 20 m/s pass now rolls 43.7 m instead of 35.6 m of the 102.41 m field.
- [x] The kickoff reset settles cleanly, with physics simply not stepped while frozen — the car and the ball now sit at exactly their resting height, so two seconds of idle play after a reset move them 0.0000 m, and the reset hands the car's jumps back.
- [x] Chase-camera clipping near walls — the camera is pulled in, trades distance for height when there is none, and lifts along the surface the car is standing on rather than the world. Measured over fourteen driving routines: the eye is never inside the arena's collision and the car is never hidden behind it, where before it was for up to a quarter of the frames of a wall climb.
- [x] Ball-cam framing keeps the ball and the car readable, including at speed — the side the camera sits on now swings round the car instead of sliding through it, so the car stays on screen for every frame of every routine and the worst single-frame camera move drops from 3.07 m to 0.53 m.
- [x] Screen and particle punch on goals and big hits — the view takes a short kick, 2.50 degrees on a goal and 0.97 on a big hit, scaled by how hard the ball was struck. It rotates the aim rather than moving the camera, so it can never push the view into a wall and never loses the car.
- [x] Boost flame/trail intensity scales with boost use — a tapped boost gets a 0.97 m cone and one ember a frame, a held one the full 2.30 m and four, ramping over 0.45 s.
- [x] Boost-pad glow reads ready vs cooldown — a ready pad breathes, and a recharging one fills its light back in as it goes, so a pad just taken and a pad about to return no longer look the same.
- [x] Bot behaviour — it keeps its target on the flat floor, backs out when blocked and takes a reset if it is ever wedged for five seconds. Eight deliberate wedges (walls, corners, both nets, upside down) all free themselves in 0.12-1.29 s with no resets. It also stops chasing a ball that is already past it in its own half and recovers goal-side instead, which took it from scoring *only* from kickoffs to scoring in open play, and from losing to the previous bot to beating it.
- [x] Smooth frame rate — 0.62 ms a frame in Release at 2560 x 1080 with bloom on, and not one frame over 16.7 ms in any configuration measured. The cost is about 0.50 ms fixed plus 0.096 ms per megapixel, so it is nowhere near fill-bound. Measured on an RTX 3060, not a laptop.
- [x] Post-processing degrades gracefully — off in Settings costs nothing, missing shaders log a warning and draw the scene unbloomed, and render targets that fail to allocate now do the same instead of rendering into an invalid framebuffer for the rest of the run.
- [x] No major errors in the log during a run — a full match to the final whistle logs zero errors, zero warnings and zero asserts in Debug, Development and Release, with resident memory unchanged across the run.

## Layout

```
Game/Source/       game code (scenes, GameObjects, systems)
Game/ThirdParty/   raylib, Jolt, glm, imgui (cloned, git-ignored)
Game/Assets/       Cars-Park car pack and the lit shader
Tools/             AssetCooker.py, FbxReader.py
Build/<Config>/    ArcadeCarSoccer + cooked assets (Linux)
Build/Windows/…    the same, for a Windows build
```

## Credits

Car models from the Cars-Park low-poly pack (`Game/Assets/Cars-Park/License.txt`). Everything else —
arena, ball, goals, boost pads, UI — is procedural.
| Key | Action |
|-----|--------|
| `W` / `↑` | Accelerate |
| `S` / `↓` | Reverse / brake |
| `A` `D` / `←` `→` | Steer |
| `Shift` | Boost |
| `Space` | Jump (press again for a double jump / flip) |
| `R` | Reset car |

### In the air

| Key | Action |
|-----|--------|
| `W` / `S` | Pitch (nose down / nose up) |
| `A` / `D` | Yaw |
| `Q` / `E` | Roll |

### Menus

| Key | Action |
|-----|--------|
| `Esc` / `P` | Pause and resume the match |
| Mouse | Navigate the main menu, pause menu and settings |

## Assets Used
- [Cars Pack](https://quaternius.com/packs/cars.html)
