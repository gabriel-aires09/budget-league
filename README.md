# Arcade Car Soccer

A third-person, 3D, arcade car-soccer game for Linux — rocket-powered cars playing football inside an
enclosed futuristic arena. Native C++ with [raylib](https://github.com/raysan5/raylib) for graphics,
[Jolt](https://github.com/jrouwe/JoltPhysics) for physics, [glm](https://github.com/g-truc/glm) for
maths and [Dear ImGui](https://github.com/ocornut/imgui) for the in-engine tuning tools.

## The game

Pick one of six low-poly cars, then kick off in a 76.81 x 102.41 m glass-walled arena with a
20.73 m ceiling. Drive, boost, jump,
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

| Configuration | Debug symbols | ImGui tuning tools | Optimisation |
|---|:---:|:---:|---|
| Debug | yes | yes | `-O0` |
| Development | yes | yes | `-O2` |
| Release | no | no | `-O3` |

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

### Final milestone — polish

- [x] Minor bumps never flip the car, while intentional flips stay snappy.
- [x] The ball is heavy but not sluggish; big hits carry.
- [x] The kickoff reset settles cleanly, with physics simply not stepped while frozen.
- [x] Chase-camera clipping near walls — the camera is pulled in, and trades distance for height when there is none.
- [x] Ball-cam framing keeps the ball and the car readable, including at speed.
- [x] Screen and particle punch on goals and big hits.
- [x] Boost flame/trail — a flickering cone at the exhaust plus embers while the boost is held.
- [x] Boost-pad glow reads ready vs cooldown.
- [x] Bot behaviour — it keeps its target on the flat floor, backs out when blocked and takes a reset if it is ever wedged for five seconds.
- [x] Post-processing degrades gracefully — off in Settings costs nothing, and missing shaders log a warning and draw the scene unbloomed.
- [x] No major errors in the log during a run.

## Layout

```
Game/Source/       game code (scenes, GameObjects, systems)
Game/ThirdParty/   raylib, Jolt, glm, imgui (cloned, git-ignored)
Game/Assets/       Cars-Park car pack and the lit shader
Tools/             AssetCooker.py, FbxReader.py
Build/<Config>/    ArcadeCarSoccer + cooked assets
```

## Credits

Car models from the Cars-Park low-poly pack (`Game/Assets/Cars-Park/License.txt`). Everything else —
arena, ball, goals, boost pads, UI — is procedural.
