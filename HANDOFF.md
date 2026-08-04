# HANDOFF — Arcade Car Soccer

Project state for whoever continues the work. Update this file whenever the project changes.

## Current state

- [x] **Milestone 01 — Project Structure**
- [x] **Milestone 02 — Base Scene and Car Movement**
- [x] **Milestone 03 — Main Menu and Pause Menu**
- [x] **Milestone 04 — Car Handling and Feel**
- [x] **Milestone 05 — Ball and Car-Ball Interaction**
- [x] **Milestone 06 — Arena, Goals and Scoring**
- [x] **Milestone 07 — Boost System and Boost Pads**
- [x] **Milestone 08 — Car Selection**
- [x] **Milestone 09 — Jumps, Flips and Air Control**
- [x] **Milestone 10 — Camera Modes**
- [ ] **Milestone 11 — HUD** — the next one; score, clock and boost meter exist as a placeholder in `MatchScene`
- [ ] **Milestone 12 — Bot Opponent (or Solo Practice)**
- [ ] **Milestone 13 — Tuning Panels (Debug/Development)**
- [ ] **Milestone 14 — Visual Polish and Effects** — lighting is done, pulled forward; shadows, bloom and effects are not
- [ ] **Milestone 15 — Audio**
- [ ] **Milestone 16 — UI Polish (raygui)**

Two pieces of work outside the milestone list are also done: the **asset pipeline** (FBX cooking plus
the car models, CLAUDE.md 4.1) and the **arena ramps with wall and ceiling driving** (PROMPTS.md).

Milestone 08 was inserted into CLAUDE.md after 09 was already built, which is why it was finished out
of order. Everything from the old Milestone 08 onwards shifted up by one; this document uses the new
numbering throughout. A second new milestone, Milestone 16 — UI Polish (raygui), was added at the end
of the list.

The game opens on a main menu (Play / How to play / Settings / Exit). How to play is its own screen
of rules and controls. Play opens the car picker — six of the
seven cooked cars on a 2 x 3 grid of pedestals, spinning slowly in the team colour — and starting
from there opens a real match: an enclosed
55 x 80 m arena with side walls, back walls, a ceiling and two coloured goals; a player car driven
with WASD/arrows on a Jolt rigid body; a ball; a score, a match clock and a kickoff countdown; and
a smooth third-person chase camera. A goal resets the field and counts down again, and the match
ends at full time. The car draws a real low-poly model from the Cars-Park pack, cooked from FBX and
painted in the team colour, and the whole scene is flat-shaded by one directional light. Shift
boosts, drawing on a 0-100 meter refilled by 18 boost pads spread across the field. Space jumps,
a second press flips in whatever direction is held, and the car can be pitched, yawed and rolled in
the air, so aerial ball touches work. Esc or P pauses, with Resume / Settings / Return to main menu
/ Exit. No opponent yet — the bot is Milestone 12.

**C** toggles between the chase camera and a ball cam that sits on the far side of the car from the
ball so both are in frame, and the camera now keeps itself inside the arena: it is clamped under the
ceiling and ray cast against the real collision geometry, trading distance for height when a wall,
a ramp or a corner is behind the car.

Every edge of the arena is now ramped rather than square, as in Rocket League: a 5 m quarter-circle
carries the floor up into the walls, and a 3.5 m one carries the walls into the ceiling. Driving into
the side of the pitch now carries the car up the wall and, with boost, across the roof: `CarObject`
measures the surface under the car rather than the world, so the same throttle, steering and grip
work upside down.

Verified on 2026-08-01 with temporary scripted-input harnesses, since there is no `xdotool`/`wtype`
on this machine to press keys. Milestone 02 used a scripted controller (accelerate → sustained right
turn → coast → reverse → reset); Milestone 03 force-included a shim that replaced `IsKeyPressed` with
a timed key script and captured a screenshot at each step. Both harnesses were removed afterwards —
no test code remains in `Game/Source/`.

| Check | Result |
|---|---|
| `make debug` / `development` / `release` | all succeed, no warnings from game code |
| Accelerates | 0 → 20 m/s in ~1.5 s, tops out exactly at `maxSpeed` (32 m/s) |
| Turns | heading sweeps a full 360° under sustained steering, stays grounded, never flips |
| Coasts | 32 → 12 m/s with throttle released |
| Reverses | brakes through zero, capped at `maxReverseSpeed` (14 m/s) |
| `R` reset | returns to spawn, zero velocity |
| Chase camera | trails ~9.5 m behind at constant height, smooth through the whole turn |
| Smoke test screenshot | non-blank, in all three configs (`Build/Release/SmokeTest.png`) |
| Debug build with `JPH_ENABLE_ASSERTS` | no assert fires during a run |
| Play starts a match | yes, from the main menu |
| Esc / P pause and resume | yes, and physics stops while paused |
| Settings shared between both menus | yes — "Bot opponent → Solo practice" set in the main menu shows in the pause menu |
| Return to main menu, from the pause menu | yes, the match scene is torn down and the menu comes back |

Milestone 04 handling, measured by a temporary harness that drove fixed routines and computed the
numbers (also removed afterwards):

| Measurement | Result |
|---|---|
| 0 to 100 km/h | 1.63 s |
| Top speed | 32.1 m/s (116 km/h), exactly `maxSpeed` |
| Turn at top speed | radius 26.1 m = 8.1 car lengths, 6.0 deg of slip |
| Turn at low speed | radius 2.7 m at 6.9 m/s |
| Recovery from fully upside down | upright and drivable in 0.78 s |
| Asymmetric ramp at 30 m/s | launches, lands flat, keeps speed, never tilts |

For reference, Rocket League turns in roughly 8.5 car lengths at top speed, so the cornering is in
the right family rather than the twitchy ~3.8 car lengths it started at.

Asset pipeline checks:

| Check | Result |
|---|---|
| All 7 FBX cars cook | yes, into `Build/<Config>/assets/Models/*.evmodel` |
| Cooked geometry matches the pack's own OBJ export | identical triangle counts, bounds within 1e-6 m, all 7 cars |
| Incremental cook | touching one `.fbx` recooks only it; touching the cooker recooks all 7 |
| Model loads and renders in game | yes, wheels on the floor, facing the direction of travel |
| Team paint | blue and orange both applied; glass, tyres, lights and the Cop light bar keep their baked colours |
| Model swapping | verified on `SportsCar`, `SUV`, `Taxi`, `Cop` |
| Missing `assets/Models` | falls back to the box stand-in, no crash, exit 0 |
| Debug / Development / Release | build with no game-code warnings, smoke test exits 0 |

Milestone 05 ball, measured by a temporary harness that stepped physics directly and computed the
numbers (removed afterwards):

| Measurement | Result |
|---|---|
| Drop from 12 m, bounce apexes | 4.36 → 1.91 → 0.86 → 0.41 m, an even ~0.64 rebound each time |
| Settles | 5.4 s after the drop, resting exactly at `radius` (1.250 m), no jitter or sinking |
| Roll from 20 m/s | 35.6 m in 7.7 s before stopping — about half the field |
| Hit at 20.4 m/s | ball leaves at 24.0 m/s (1.18x), 25 deg launch, travels 38 m |
| Hit at 26.5 m/s | ball leaves at 31.1 m/s (1.17x), 25 deg launch, travels 60 m |
| Hit at 32.0 m/s (top speed) | ball leaves at 38.0 m/s (1.19x), 24 deg launch, 1.8 s airborne, travels 54 m |

Lighting checks:

| Check | Result |
|---|---|
| Shader cooks and ships | `assets/Shaders/Lit.vs` + `Lit.fs` in all three configs |
| Compiles on load | yes, no GLSL warnings |
| Flat shading | cars, ball and floor all render one tone per triangle |
| Missing `assets/Shaders` | logs `LIGHTING: lit shader unavailable, drawing unlit`, renders unlit, exit 0 |
| Model unload with a shared shader | no double free; all three configs exit 0, Debug included |

Milestone 06, measured by a temporary harness (removed afterwards):

| Check | Result |
|---|---|
| Ball fired at 50 m/s into each side wall | contained, max abs(x) 26.4 against a wall at 27.5 |
| Ball fired at 50 m/s into each back wall, off centre | contained, max abs(z) 38.9 against a line at 40.0 |
| Ball fired straight up at 50 m/s | peaks at y 14.0 under a 15.0 ceiling, comes back down |
| Ball touching the line from in front | no goal |
| Ball half across | no goal |
| Ball **1 mm** short of fully across | no goal |
| Ball exactly fully across | GOAL |
| Ball past the line but outside the mouth (x = 20) | no goal |
| Kickoff | releases after exactly 3.00 s into Playing |
| Clock | runs down 1.00 s per second of play, stops at 0 into Finished |
| Goal sequence | score 0 → 1, celebration 2.50 s, then reset |
| Reset accuracy | ball back to (0, 1.25, 0), car to (0, 0.36, 24), countdown re-armed at 3.00 s |

Milestone 07 boost, measured by a temporary harness (removed afterwards):

| Measurement | Result |
|---|---|
| Full tank held down | empties in 3.03 s at 33 per second |
| `boosting` flag once empty | clears on the next step |
| 0 to 100 km/h, throttle only | 1.63 s |
| 0 to 100 km/h, throttle + boost | 0.56 s |
| Top speed, throttle only | 32.1 m/s (116 km/h) |
| Top speed with boost | 46.2 m/s (166 km/h) |
| Cruising 31.6 m/s, then boost | 45.7 m/s in 0.62 s, costs 8 boost |
| Pads laid out | 18 (4 full-refill, 14 small) |
| Small pad driven over | boost 20 → 32, pad dark, 4.0 s cooldown |
| Full pad driven over | boost 20 → 100, pad dark, 10.0 s cooldown |
| Pad on cooldown | gives nothing |
| Pad after its cooldown | ready again at exactly 4.0 s / 10.0 s |
| Car 5 m directly above a pad | pad not taken |
| Nearest pad edge to the kickoff spot | 2.00 m, so kickoff never grabs one |

Milestone 09 jumps and air control, measured by a temporary harness (removed afterwards):

| Measurement | Result |
|---|---|
| Single jump | peaks 1.49 m above rest, 1.01 s airtime, lands upright |
| Double jump | peaks 2.67 m, 1.67 s airtime, lands upright |
| Forward / backward flip | 8.3 m/s speed change, 7.9 rad/s spin, upright again 2.02 s later |
| Side flip | 8.3 m/s, 7.7 rad/s, upright again 0.89 s later |
| Pitch, yaw and roll, each alone | 5.56 rad/s, 292 degrees per second |
| Launched tumbling on all three axes | upright and drivable 3.16 s later, unaided |
| Rolling 1.3 m above the floor | 5.43 rad/s — the righting assist stays out of the way |
| Jump + second jump + boost, nose up | peaks at **10.09 m** |
| Ball hung at 4.0 m (underside 2.75 m) | car climbs to 2.69 m and drives it up at 8.1 m/s |

Arena ramps, measured by a temporary probe that raycast the built collision shape and stepped
physics directly (removed afterwards):

| Check | Result |
|---|---|
| Floor ramp profile vs an ideal quarter circle | within 0.03 m through the drivable part, 0.08 m at the steepest facet |
| Lips anywhere in the ramp | none — the profile is monotonic, biggest backward step 0.000 m |
| Rise per 0.25 m across the ramp | 0.025 → 0.29 m, no discontinuity between facets |
| Vertical wall | exactly x = 27.5 from y = 5.0 to y = 11.5 |
| Ceiling fillet | leaves the ceiling at x = 24.0, meets the wall at y = 11.5, matches R = 3.5 |
| Goal mouth | floor stays flat to the goal line, no ramp across it |
| Boost pads overhanging a ramp | 0 of 18 |
| Standing start, full boost into the wall | climbs to 14.1 m, reaches the wall face, then tumbles |
| 32 m/s into the ramp | launches off it at 3.8 m, stays upright (uprightness 0.77) |
| Ball at 50 m/s into a ramp, five angles | contained; max abs(x) 26.4, abs(z) 38.8, peak y 13.8 under the 15 m ceiling |
| Ramp reads as joining the wall | yes, after the seam line and mullions were added — checked on a screenshot |
| Rounded corner profile vs the ideal | within 0.055 m at every height and angle |
| Smoothness around a corner | biggest jump 0.010 m between samples 10 degrees apart |
| All four corners the same shape | identical to 3 decimals, sampled every 15 degrees |
| Driving a wall straight through a corner | grounded **100%** of 2 s at both 7.5 m and 10.5 m, 63% of the run spent on the far wall |
| Ball at 60 m/s straight into each corner, 3 heights | 12 of 12 contained, nothing escaped |
| Wall-to-corner handover, scanned every 0.5 m | continuous, no gap |
| Corner brightness after trimming the drawn overlap | matches the flat walls |
| Boost pads vs the rounded flat floor | 0 of 18 overhang; worst rise under any pad rim 0.000 m |
| Arena size | 770 collision pieces, 94 drawn ramp meshes |
| Ramp drawn as one surface, no alpha banding | yes; forcing the ramps opaque shows full coverage, correct facing, goal mouth left clear |
| Debug teardown with 10 ramp models | 10 VAOs unloaded, lit shader unloaded exactly once, no errors |

Surface-relative driving, measured by a temporary harness that ran the same routines against the
original `CarObject` and the new one (removed afterwards). The left column is the point: everything
on flat ground had to stay exactly as Milestones 04 and 09 left it.

| Measurement | Before | After |
|---|---|---|
| 0 to 100 km/h | 2.13 s | 2.13 s |
| Top speed | 32.1 m/s | 32.1 m/s |
| Jump held 4 s (infinite-jump check) | peak 1.85 m | peak 1.85 m |
| Tap, tap (double jump) | peak 4.08 m | peak 4.08 m |
| Recovery from upside down | 0.80 s | 0.93 s |
| Wall climb, 20 m/s, no boost | 1.97 m, never on the wall, airborne 37% | **13.15 m, on the wall 47% of 3 s, airborne 0%** |
| Wall climb, 32 m/s, no boost | 3.36 m, never on the wall, airborne 55% | **10.88 m, on the wall 48% of 3 s, airborne 0%** |
| Driving on the ceiling | falls off immediately (0% of 3 s) | **holds 92% of 3 s** |
| Boosted run up a wall and across the roof | — | wall at 2.16 s, ceiling at 3.03 s, grounded 92% of the run |

The 0-100 figure differs from the 1.63 s recorded for Milestones 04 and 07 because this harness
starts and measures differently, **not** because anything regressed — the original code measures
2.13 s on this harness too. Compare within a column, not across harnesses.
| Debug / Development / Release | build with no game-code warnings, smoke test exits 0, screenshot non-blank |

Milestone 10 camera modes, measured by a temporary probe that placed the car around the arena, ran a
second of camera frames per placement and asked Jolt — not the camera code — whether the resulting
camera point was inside a body (removed afterwards). "Car seen" is the car's centre projected into
the window:

| Placement | Camera | Distance | In solid | Car | Ball |
|---|---|---|---|---|---|
| Kickoff, chase | (0.00, 3.76, 33.50) | 10.09 | no | seen | seen |
| Kickoff, ball cam | (0.00, 4.76, 35.50) | 12.31 | no | seen | seen |
| Ball behind the car, ball cam | (0.00, 4.76, -11.48) | 12.29 | no | seen | seen |
| Ball 9 m up, ball cam | (0.00, 4.76, 21.50) | 12.31 | no | seen | seen |
| Ball on top of the car, ball cam | (-1.72, 4.76, 11.37) | 12.31 | no | seen | seen |
| Parked facing away from the +X wall | (26.07, 4.97, 0.00) | 6.49 | no | seen | seen |
| Same, ball cam | (26.17, 6.34, 0.00) | 7.59 | no | seen | seen |
| Inside the goal mouth | (0.00, 4.58, 43.44) | 6.88 | no | seen | seen |
| In a corner, facing the middle | (24.04, 4.48, 37.04) | 7.04 | no | seen | seen |
| Same, ball cam | (22.98, 5.88, 37.92) | 7.97 | no | seen | seen |
| Up the +X ramp, facing the wall | (26.06, 8.55, 0.00) | 7.03 | no | seen | — |
| Driving the +X wall at 8 m | (26.00, 11.40, 9.50) | 10.09 | no | seen | — |
| Driving the ceiling at 13.8 m | (0.00, 14.20, 9.49) | 9.50 | no | seen | — |

The first five rows are the point of the last eight: in open field the new clamps are inert and the
camera sits exactly where Milestone 02 left it.

| Moving check (25 m/s into the +X ramp and up the wall, 3 s) | Chase | Ball cam |
|---|---|---|
| Biggest camera move in one frame | 0.299 m | 0.876 m |
| Biggest *change* in that, frame to frame | 0.035 m | 0.058 m |
| Ever inside solid geometry | no | no |
| Ever lost the car off screen | no | no |

| Camera sensitivity | Frames to recover from an 8 m shove |
|---|---|
| 0.5 | 59 |
| 1.0 | 29 |
| 2.0 | 14 |

Milestone 08 car selection, verified with the input-shim harness described at the end of this
document (removed afterwards) plus screenshots of the scene:

| Check | Result |
|---|---|
| All six previews load and paint | yes, one `MODEL: loaded` line each, all in team blue |
| Grid reads evenly | yes — every preview is fitted to the same 3.8 m length, and all six sit at the same distance from the camera |
| Scripted right, then down, then Enter | selection went SPORTS CAR → POLICE → COMPACT, exactly the grid arithmetic |
| The pick reaches the match | the match then loaded `NormalCar2.evmodel`, and the screenshot shows that car |
| Frames and labels track their car | yes, projected from each pedestal, so both rows and any window size line up |
| Right click on a cell | scripted at the SUV cell (970, 450): the match then loaded `SUV.evmodel` |
| Scene teardown | all six models unloaded, lit shader unloaded exactly once, exit 0 |
| Debug / Development / Release | build with no game-code warnings, smoke test exits 0, screenshot non-blank |

## Layout

```
Game/Source/
  Main.cpp, App.h/cpp          entry point, window, settings, scene switching, smoke test
  Scene.h/cpp                  camera + GameObjects + Jolt PhysicsSystem, fixed step loop
  MainMenuScene.h/cpp          title, Play / How to play / Settings / Exit, credits and build string
  HowToPlayScene.h/cpp         the how-to-play screen: six panels of rules and controls
  CarSelectScene.h/cpp         the car picker: six cooked cars on a 2 x 3 grid of pedestals
  MatchScene.h/cpp             the gameplay scene, pause overlay, score/clock readout
  Match.h/cpp                  score, clock, kickoff/goal/full time state machine
  GameSettings.h               the settings struct App owns
  MenuAction.h                 what a scene asks App to do next
  UserInterface.h/cpp          namespace uistyle: colors, text helpers, MenuList widget
  SettingsMenu.h/cpp           settings panel reused by both menus
  GameObject.h/cpp             base object, body transform helpers
  StaticModelAsset.h/cpp       .evmodel loader + assets::Path, team repaint
  Lighting.h/cpp               namespace lighting: the one lit shader, Apply/Detach
  GameObjects/CarObject.h/cpp  arcade car (single box body + cooked car model)
  GameObjects/BallObject.h/cpp the ball (single dynamic sphere)
  GameObjects/GoalObject.h/cpp net geometry + the fully-across-the-line test
  GameObjects/BoostPadObject.h/cpp refill pad, no physics body, cooldown + glow
  GameObjects/ArenaObject.h/cpp floor, walls, ceiling, back walls with goal openings, edge ramps
  CarController.h              CarInput + abstract controller
  PlayerController.h/cpp       keyboard input
  ChaseCamera.h/cpp            third person follow camera, ball cam and the arena clamps
  PhysicsLayers.h/cpp          namespace physics: layers and Jolt filters
Game/ThirdParty/   raylib, Jolt, glm, imgui     (cloned, git-ignored)
Game/Assets/       Cars-Park/ (OBJ, FBX, Blends, License.txt, Preview.png)
                   Shaders/ (Lit.vs, Lit.fs)
Tools/             AssetCooker.py, FbxReader.py, requirements.txt
Build/<Config>/    ArcadeCarSoccer + assets/Models/*.evmodel + assets/Shaders/*
```

## How to build and run

```sh
make                 # Development (default)
make debug           # or: make development / make release / make all
make run CONFIG=Debug
./Build/Release/ArcadeCarSoccer

# headless-ish validation: run N frames, write a screenshot, exit
./Build/Release/ArcadeCarSoccer --smoke-test 60 --screenshot SmokeTest.png
```

Controls so far: **WASD / arrows** drive and steer on the ground and pitch/yaw in the air,
**Space** jumps (again for a double jump, or a flip if a direction is held), **Shift** boosts
(held), **Q/E** air roll, **C** toggles chase cam and ball cam, **R** resets the car, **Esc** or
**P** pauses.
In menus: **up/down** or **W/S** move, **Enter**/**Space** activate, **left/right** change a value,
and the mouse works too. In the car picker: **arrows** or **WASD** move around the grid, **Enter**
or **Space** starts the match, **Esc** goes back to the main menu, left click picks a car and
**right click on a car picks it and starts the match in one go**.

`make clean` removes the build output. `make clean-thirdparty` additionally forces a raylib rebuild.
The first build takes a few minutes (raylib + 153 Jolt translation units per config); after that it
is incremental.

## Decisions made

### Build
- **No CMake** — it is not installed on this machine. raylib is built through its own `src/Makefile`;
  Jolt and ImGui sources are compiled directly by the root `Makefile`; glm is header-only.
- **Pinned versions:** raylib `6.0`, Jolt `v5.6.0`, glm `1.0.3`, Dear ImGui `v1.92.9b-docking`.
- **raylib is built once, release mode**, shared by the three configs. Jolt *is* compiled per config
  because its assert/profiling defines differ.
- **Jolt defines and SIMD flags must stay identical** across the whole build; they live only in
  `JOLT_DEFINES` and `SIMD_FLAGS` in the `Makefile`. Mismatching them causes silent ABI breakage.
- Third-party include paths use **`-isystem`**, so `-Wall -Wextra` only reports game code.
- Mode defines: `GAME_DEBUG` / `GAME_DEVELOPMENT` / `GAME_RELEASE`, plus `GAME_DEV_TOOLS` in Debug
  and Development only — gate all ImGui tuning UI behind it (Milestone 13).

### Gameplay and physics
- **The car is a single dynamic box** with a downward ground-probe ray and custom arcade forces, not
  a Jolt `VehicleConstraint` (CLAUDE.md 2.5). Throttle is a force, **steering sets the yaw angular
  velocity directly** (crisper than torque and it cannot spin out), and grip is an impulse that
  bleeds off sideways velocity.
- **Steering authority falls off with speed** (`highSpeedSteerScale`). Without it the car kept full
  yaw rate at 116 km/h and pivoted in ~3.8 car lengths, which felt like it was on rails.
- **Handling rates are per second, not per step** (`grip`, `tumbleDamping`, converted with `expf`).
  They were per fixed step at first, which silently ties the feel to the 120 Hz timestep — and
  `tumbleDamping` at 0.14 per step was a ~1e-8 per second decay that made the car literally
  unflippable. Keep new tunables in per-second units.
- **The centre of mass is offset below the box** (`centerOfMassOffsetY`, via Jolt's
  `OffsetCenterOfMassShape`). This is what stops minor bumps from tipping the car, and it does more
  for stability than damping does.
- **Two ground probes, not one.** The short one gates driving; a longer one (`recoveryProbe`) gates
  the righting assist, because a car on its roof or balanced on an edge sits too high for the short
  probe and would otherwise get no help at all — that alone took flip recovery from 3.2 s to 0.8 s.
- **The righting torque uses a normalised axis.** `up x worldUp` has length sin(tilt), which is zero
  when the car is exactly upside down; scaling by it left the car stranded on its roof. Strength now
  ramps with the tilt angle instead (squared, so it barely acts when nearly level).
- **The ball has its own gravity, not the scene's.** `gravityFactor = 1.7` on the body (Jolt's
  `MotionProperties::SetGravityFactor`) makes it fall at ~16.7 m/s² so hits arc like an arcade ball
  instead of floating. Doing this per body is deliberate: raising the *scene* gravity would have
  invalidated all of Milestone 04's car tuning. At factor 1.0 a top-speed hit hangs for 2.3 s and
  crosses 67 m of an 80 m field; at 1.7 it is 1.8 s and 54 m.
- **Rolling resistance lives in the ball's `angularDamping`, not `linearDamping`.** A rolling ball's
  speed is coupled to its spin by friction, so angular damping is what stops it, while linear
  damping would also bleed speed off a ball in flight and make big hits feel weak. At the first
  guess of 0.35 a 20 m/s roll crossed 69 m of an 80 m field and took 16 s to stop; 1.2 gives 36 m
  in 7.7 s.
- **Ball restitution drives car-ball hits too**, because Jolt combines restitution with `max()` and
  the car's is only 0.05. So the one number controls both how lively the floor bounces are and how
  hard the ball comes off the car — they cannot be tuned separately without a contact listener.
  0.70 was the balance point: 0.55 gave a limp 1.07x speed transfer, 0.85 gave 1.28x but the ball
  took 10 s to settle and played like a pinball.
- **Every ground hit lofts the ball ~24 degrees, and that is geometry, not a bug.** The car box tops
  out at y = 0.8 and the ball's centre is at 1.25, so contact is always below the centre and the
  normal tilts up by about 21 degrees. Rocket League behaves the same way. Do not "fix" it.
- **The ball uses `LinearCast` motion quality.** It is the fastest thing in the scene (38 m/s off a
  top-speed hit, capped at `maxSpeed` 55) and the one object that must never tunnel through a wall
  once the arena is closed in Milestone 06.
- **Driving is measured against the surface the car is on, not against the world.** This is what
  lets the same code drive the floor, a wall and the ceiling. Three things changed together and none
  of them works alone: the drive probe casts along the car's **own** down rather than straight down;
  the surface normal is read back from that hit (`Body::GetWorldSpaceSurfaceNormal`) and becomes what
  `uprightness`, `driveUprightMin` and the righting assist all measure against; and a stick force
  holds the car on. On level ground with an upright car the probe is the same ray it always was, so
  none of this touches flat-ground handling — verified identical, see the table above.
- **The recovery probe deliberately stayed world-down.** It exists for a car on its roof, and such a
  car has its own down pointing at the sky, so a local ray would never find the floor to right it
  onto. The rule is: `alignTo` is the surface normal when grounded, world up otherwise.
- **The stick force must be scaled by surface tilt; a constant one destroys the car.** Measured:
  a constant 10 m/s² took 0-100 from 2.13 s to 3.41 s, 25 m/s² dropped top speed from 32 m/s to
  **5 m/s**, and 50 m/s² pinned the car so hard it could not move at all (0.2 m/s). Extra downforce
  on the floor is simply extra friction. Scaling by `(1 - normal·worldUp) * 0.5` makes it exactly
  zero on level ground, half on a wall and full on the ceiling, which is also the order in which it
  is actually needed — gravity presses the car into the floor, does nothing on a wall, and pulls it
  off the ceiling. `surfaceStick` (22 m/s²) therefore has to beat gravity, since the full value
  applies exactly where it is inverted.
- **The real reason a car bounced off the ramp was the probe, not the stick force.** A 3.2 m box
  bridges a concave curve: on the 5 m ramp its middle rides 0.26 m higher than its ends, which ate
  most of a 0.7 m probe and dropped the car exactly where it was trying to climb. `groundStickyProbe`
  (0.75, against `groundProbe` 0.35) extends the reach *only once already grounded*, so it can never
  make an airborne car grounded. That one change took a 20 m/s no-boost approach from 1.97 m and
  bouncing to 13.15 m and glued. 1.10 measured no better than 0.75, so 0.75 is the value.
- **The sticky probe is suppressed during `jumpLockout`, and has to be.** A jump clears only 0.72 m
  in those 0.15 s, so a 0.75 m reach would still find the floor as the lockout expired, hand the
  jumps straight back and let the car jump forever — the exact bug `jumpLockout` exists to stop.
  With the short probe used during the lockout the car is already 0.72 m up when it expires, past
  the 0.35 m the short probe can see, so grounded stays false. Verified: jump held for 4 s peaks at
  1.85 m, identical to before.
- **`CarInput::jump` is held, not an edge, and `CarObject` finds the rising edge itself.**
  `CarObject::Update` runs once per *fixed step* while `IsKeyPressed` stays true for a whole frame,
  so at 120 Hz an edge-triggered field would fire twice in one frame and eat the double jump
  instantly. Any new one-shot input must follow the same pattern.
- **`jumpLockout` (0.15 s) exists because the ground probe still hits right after a jump.** Without
  it the very next step sees `grounded`, hands the jumps back, and the car can jump forever.
- **A flip is `up x heldDirection` for its spin axis.** That one expression gives a nose-down pitch
  for a forward flip and a roll for a side flip, with no per-direction special casing.
- **The righting assist is suppressed only when airborne *and* air input is present.** It cannot be
  gated on input alone: W is throttle on the ground and pitch only in the air, so that would switch
  the assist off every time the player drives forwards.
- **`airControlRate` is a requested rate; the car settles at about 80% of it.** The body's
  `angularDamping` (2.2) pulls the spin back every step, so 7.0 measures as 5.6 rad/s (300 deg/s,
  about the Rocket League figure). Tune it by measuring, not by reading the number.
- Air control drives the angular velocity towards the requested rate rather than applying torque,
  matching how ground steering already works. With no input the target is zero and the response
  drops to `airDamping`, so a flip still completes but the car settles before landing.
- **Boost is applied before the grounded gate**, so it already works in the air. It is the one
  control that must keep working off the ground, and Milestone 09 builds aerials on top of it.
- **Boost has its own speed cap** (`boostMaxSpeed` 46 m/s) above the throttle's `maxSpeed` (32),
  because the throttle stops adding force at its own cap. `boostForce` was tuned on the metric that
  matters in play — how long the push from cruising to supersonic takes. 9000 N did it in 0.36 s,
  which read as a teleport; 4000 N never reached the cap before running out of field; 6000 N gives
  0.62 s. Do not tune this on the 0-100 figure alone.
- **Boost pads have no physics body at all.** A pad has to be flush with the floor, because the car
  is a box with no wheels and would stop dead against any raised lip, so a collider would be either
  useless or harmful. Pickup is a distance check in the XZ plane plus a height limit, the same
  analytic approach `GoalObject` uses. `physics::Trigger` is therefore *still* unused — it may never
  be needed.
- **A pad is taken even when the car is already full**, as in Rocket League. Wasting a pad is part of
  managing boost.
- `MatchScene::boostPads` is a vector that must be **filled completely before any pointer is taken
  into `Scene::objects`** — a later `push_back` would reallocate and leave those pointers dangling.
  This is the one place the scenes-own-objects-by-value rule needs care.
- **Goal detection is an analytic test, not a Jolt sensor.** `GoalObject::IsBallFullyInside` compares
  the ball's centre against the goal line plus its radius. "Fully across the line" is exactly a
  statement about centre and radius, so this is one comparison and it is exact — verified to be
  correct to the millimetre. A sensor body would have had to be inset by a whole ball diameter to
  mean the same thing, would need a `ContactListener`, and would still depend on when Jolt happened
  to generate the contact. This is why `physics::Trigger` is still unused; boost pads in Milestone
  07 can most likely do the same thing with a distance check.
- **Z is the goal-to-goal axis, X is sideline to sideline.** The field was 80 x 55 with the long axis
  across the pitch, which would have put the goals on the short side; the numbers are now swapped
  (55 wide, 80 long, a 0.69 ratio close to a real football pitch). 80 m at 32 m/s is a 2.5 s run
  from goal to goal. Blue defends +Z, orange defends -Z, and the player kicks off in front of blue.
- **The arena and each goal are one Jolt compound body each**, built from a `Piece` list that
  physics and rendering both read. That list is the single source of truth, so the collision cannot
  drift away from what is drawn — worth preserving when Milestone 14 adds trim and stands.
- **The ceiling has collision but is never drawn.** Drawing it would put a slab between the chase
  camera and the field the moment the car climbs a wall.
- **Every arena edge is a quarter-circle ramp, built from tilted boxes** (`ArenaObject::AddFillet`).
  `Piece` gained a `rotation`; it defaults to identity, which is why the floor, walls and ceiling are
  still written as plain four-field initializers. The rotation is derived from the surface basis
  (tangent, normal, run axis) as a matrix and converted once — do not try to express these as a
  single world-axis angle. It works for the floor ramps but breaks for the ceiling ones, where the
  box's local up has to end up pointing *downwards*.
- **Each ramp facet sits on the chord between two angles, not on the tangent.** A chord is a secant,
  so extending a facet to overlap its neighbour buries the extension inside the solid; extending a
  tangent plane would push it out into the driving surface and create exactly the lip that stops a
  wheel-less box car. The 0.02 m overlap in `AddFillet` depends on this.
- **8 facets per quarter turn.** The error against a true circle is the sagitta,
  `R(1 - cos(Δθ/2))`: 2.4 cm at the floor's 5 m radius, 1.7 cm at the ceiling's 3.5 m. At 6 facets it
  is 4.3 cm, which is into the range that a box car notices. Raising the radius or the segment count
  is safe; lowering either is not.
- **The floor ramp radius is 5 m and the ceiling's is 3.5 m**, so the flat playing surface is 45 x 70
  inside an arena that is still 55 x 80, and the wall is truly vertical only between y = 5 and
  y = 11.5. `FlatHalfWidth()` / `FlatHalfLength()` report that flat area; anything that has to lie on
  the floor must be placed against them, not against `width` / `length`.
- **The back-wall ramps stop either side of the goal mouth.** A ramp across the mouth would wall the
  goal off. This leaves the floor flat right up to the goal line, which is also how Rocket League
  reads, and it keeps `GoalObject`'s analytic line test untouched.
- **The four vertical wall intersections are rounded too** (`ArenaObject::AddCorner`, `cornerRadius`
  8 m). Three surfaces meet at each: the quarter cylinder replacing the 90 degree intersection, and
  the floor and ceiling ramps carried around it as a torus. The torus is a ring of `cornerSegments`
  short straight fillets laid on the chords of the corner arc, so `AddFillet` builds all three and
  nothing new was written — it only had to take an explicit segment count, because the corner needs
  a finer division (10) than a ramp profile (8).
- **`AddFillet` never assumed `up` was vertical, which is what made the corner free.** A vertical
  corner is the same quarter-circle join with a horizontal `up` and a vertical run. If that
  assumption is ever added to it, the corners break.
- **All three corner surfaces share one angular division, and that is what makes them seamless.**
  Their chord midpoints then coincide, and the arithmetic lines up exactly: at the top of the floor
  ramp the torus is `(cornerRadius - floorRampRadius) + floorRampRadius = cornerRadius` from the
  corner axis, which is the cylinder. Measured across the whole corner, the surface tracks the ideal
  profile to 0.055 m with a biggest jump of 0.010 m between samples 10 degrees apart.
- **`cornerRadius` must stay larger than `floorRampRadius`.** The floor ramp is carried around a
  circle of radius `cornerRadius - floorRampRadius`; at or below zero the corner has no flat floor
  left in it and the torus turns inside out. It also sets the shape of the pitch: the flat floor is
  now a rounded rectangle, 45 x 70 with corners of radius 3.
- **Adding the corners needed no trimming of the *collision* — but the *drawing* had to be trimmed.**
  Everything the straight walls and ramps put in the corner region sits further from the corner axis
  than the corner surfaces do, so it is buried in solid and the rounded corner is what the car
  reaches first. Glass never writes depth though, so every buried layer still blends: with the
  straight runs, the corner cylinder and the tori all drawn on top of each other, the corners lit up
  about two and a half times brighter than the walls. `AddFillet` therefore takes a `drawCenter` and
  `drawHalfLength` separate from the collision run — collision overlaps into the corner, drawing
  stops at it.
- **Do not "simplify" that by trimming the collision to match the drawing.** It was tried, and it is
  wrong: near the corner entrance the straight ramp surface and the corner cylinder are only about
  4 cm apart, so removing the straight one changes which surface a car rides for a stretch. Measured,
  a car carrying the wall through a corner went from **100% grounded to 78%**, and reached the far
  wall later. The drawn strip is a strict subset of the solid, on purpose.
- **The draw range needs its own centre, not just its own length.** The back-wall floor ramps are the
  one case where the two are not concentric — collision runs from the goal mouth to the far wall,
  drawing stops at the corner — and reusing the collision centre with a shorter length drew the
  strip in the wrong place, leaving a dark gap beside the goal.
- **A corner ramp's drawn strips must taper, because a torus narrows as it comes off the wall**
  (`drawTaper`). A straight fillet is an extrusion of constant width, and a ring of them is not: at
  the top of the ramp each run wants the full chord (1.26 m at these radii) but at the toe the true
  arc is only 0.47 m, so untapered runs overlapped about **2.7x** near the toe. That piled two or
  three layers of glass on each other and smeared the flat shading, which is the whole thing that
  makes the slope readable — the corner looked like a plain dark bowl next to a straight ramp with
  clean bands. The taper is the ratio of the flat end's distance from the corner axis to the wall
  end's, so it is `(cornerRadius - floorRampRadius) / cornerRadius` for the floor and the
  `ceilingRampRadius` equivalent for the roof; everything straight passes 1.0 and is unaffected.
- **The taper is drawing only — the collision boxes stay over-wide, and that is fine.** An over-wide
  box at the toe sticks out *tangentially*, which puts its ends further from the corner axis, i.e.
  deeper into the solid. It never protrudes into free space, so the measured corner profile is
  unchanged.
- **Lines buried in solid still show through glass, because glass never writes depth.** The old
  square top-of-wall edges and the vertical posts at the sharp corners had to go, not just be left
  to be occluded — they were drawn over the rounded corner. The roof outline now traces the real
  edge where the ceiling ramps end, corners included, and the field markings are clipped to the
  rounded flat floor rather than squared off.
- **Every ramp is glass, like the walls**, so the whole edge of the arena is one continuous
  see-through surface. The floor ramps are held at alpha 90 against the walls' 52 because they are
  driven on and the slope still has to read. Colour alpha alone decides which pass a piece lands in,
  so switching them cost one constant. They started opaque; that was changed on request.
- **A ramp's collision is boxes but what is drawn is a single strip mesh** (`ArenaObject::RampMesh`).
  Drawing the boxes made a transparent ramp read as stacked bands: 8 overlapping boxes compound
  their alpha at every overlap, and each box blends its near face *and* its far face. A strip has
  neither problem. The boxes are still built and still own the collision — they are just pushed with
  `visible = false`, which is the mechanism the undrawn ceiling slab already used.
- **The strip and the boxes cannot drift apart, because both come from `ArcPoint` in the same call.**
  The strip's vertices are the arc points where consecutive chords meet, which are exactly the ends
  of the box top faces. This is the one place in the arena where collision and rendering are not
  literally the same data, so keep them derived from the same arc — do not hand-tune one.
- **The strip winding is taken from `up x inward`, not from the `runAxis` argument.** The two side
  walls are passed the same `runAxis` but mirror each other, so half the ramps would have been wound
  backwards and culled away when seen from inside the arena. `runAxis` is still what positions a run
  along its length; it just cannot decide the facing. Verified by temporarily forcing the ramps
  opaque, which shows every one of them at full coverage from the default camera.
- **`lighting::Detach` runs on all 10 ramp models before `UnloadModel`.** Same trap as everywhere
  else: any one of them would otherwise destroy the shared lit shader. Debug teardown was checked —
  10 VAOs unloaded, shader program unloaded exactly once, no errors.
- **`LoadModelFromMesh` does not upload**, so `UploadMesh` has to be called first or the ramp draws
  nothing. `MemAlloc` for the vertex array is correct here: `UnloadModel` frees it with `RL_FREE`.
- **The seam line and mullions are what make the edge of the arena readable at all.** They were added
  when the ramps were still solid and the ramp appeared to end in mid air against an invisible glass
  wall; now that the ramps are glass too they matter more, not less, because nothing else tells the
  player where the ramp starts and where it becomes wall. `DrawGlassWalls` draws a
  bright line along the top of every ramp (`y = floorRampRadius`) plus faint vertical mullions every
  5 m up the flat part of each wall, matching the field grid spacing. Pushing the walls' alpha up
  would read too, but at the cost of the see-through the chase camera depends on — lines cost
  nothing and keep both. The back-wall seam is split around the goal mouth, which works out
  because `goalHeight` and `floorRampRadius` are both 5.0, so no mullion ever crosses the opening.
- **The two trim strips along the base of the side walls were removed.** The floor ramp now occupies
  that space and they were left buried inside it.
- **The field markings are drawn to the flat area, not to the field bounds.** Past the ramp toe they
  would be inside solid geometry. The pitch outline therefore traces the bottom of the ramps, which
  is the edge a player actually reads as the boundary.
- **Three boost pads moved inward** (the four corner pads to x 19 / z 31, the two halfway pads to
  x 20) so no pad hangs over a ramp. A pad is a flat disc with no body, so one overhanging would
  clip into the slope and look broken.
- **The arena walls are glass, and that needed a separate draw pass, not just an alpha.** The chase
  camera regularly ends up outside the arena when the car is against a wall, and a solid wall then
  hid the car completely. Lowering the alpha alone fixes nothing: `ArenaObject` draws first in
  `Scene::objects`, so the wall stamps itself into the depth buffer and the car behind it is
  rejected before it can ever blend through. `ArenaObject::DrawGlassWalls` therefore runs **after
  every other object**, called explicitly at the end of `MatchScene::Draw`, with `rlDisableDepthMask`
  so the panels test depth but never write it. A piece is glass purely because its colour has
  alpha below 255, so `Draw` and `DrawGlassWalls` split the same `pieces` list with no extra flag.
  Anything transparent added later has to join that late pass for the same reason.
- The wall edges are drawn as solid lines so the boundary still reads once the panels are
  see-through. The opaque base trim serves the same purpose at floor level.
- **The kickoff freeze is simply not stepping physics.** Everything has just been re-centred with
  zero velocity, so skipping `StepPhysics` holds it there exactly, with no new "frozen" flags on the
  bodies and no risk of Jolt waking something. `Match::IsFrozen()` covers Kickoff and Finished;
  Celebration deliberately keeps simulating so the ball is seen going into the net.
- Car and ball kickoff positions come from their own `spawnPosition` fields, so `Match::ResetField`
  does not duplicate them. The car's spawn Y is 0.36 (its box half-height plus a hair) so it does
  not visibly drop when the countdown ends.
- **Physics runs at a fixed 120 Hz** with an accumulator (`Scene::StepPhysics`, 0.25 s clamp).
  `GameObject::Update` is called once per *fixed step*, right before the simulation runs — this
  matters because Jolt forces only persist for one step, so applying them per frame would be wrong.
- **Field scale:** 80 m (X) × 55 m (Z), car 1.7 × 0.7 × 3.2 m, top speed 32 m/s. Chosen so the car
  crosses the field in a couple of seconds. The ball should be sized against this later (~2.5 m).
- **Forward is local −Z**, right is +X, up is +Y — the OpenGL/raylib convention, shared with Jolt.
  Positive yaw turns left, so steering negates it.
- The car spawns at `(0, 0.45, 15)` facing the middle of the field.
- **`JobSystemSingleThreaded`**, not the thread pool: the scene will only ever hold a handful of
  bodies. Swap it if body count grows a lot.
- Scenes own their GameObjects **by value** and register pointers in `Scene::objects`, so there is
  no allocation or ownership question. Only `App::activeScene` is `new`/`delete`.
- `MatchScene::Shutdown` removes **and destroys** every body before the `PhysicsSystem` goes away.

### Assets and cooking
- **The cooker reads FBX, not OBJ.** CLAUDE.md 3.1 says to use the pack's OBJ+MTL and ignore the FBX;
  this was changed on request. `Tools/FbxReader.py` is a ~300-line reader for binary FBX 7.x, scoped
  to exactly what this pack contains (static flat-shaded meshes, per-polygon materials, no textures,
  no skinning). It **raises on anything it was not built for** — ASCII FBX, unsupported normal or
  material mappings, model rotation/scaling it would have to bake — so a surprising input fails the
  cook loudly instead of producing a silently wrong model. No `assimp`, no external dependency; the
  cooker still needs nothing but the standard library.
- **The axis conversion is read from the file, not hardcoded.** FBX GlobalSettings names which axis is
  up/front/right and the sign of each; the reader turns that into the game's X-right, Y-up,
  Z-toward-viewer basis, and scales by `UnitScaleFactor/100` to get metres. That is what makes the
  result checkable: cooked geometry matches the pack's own OBJ export to within 1e-6 m on all 7 cars.
  If a future asset looks mirrored or rotated, check GlobalSettings first.
- **`.evmodel` format** (`EVMDMSH1`, layout documented in `AssetCooker.WriteModel`): bounds, then
  materials (RGBA + paint flag + shade), then one **triangle-soup mesh per material**. No index
  buffer on purpose — the models are flat shaded so every triangle owns its normals and indexing
  would deduplicate almost nothing, and it keeps the format clear of raylib's 16-bit index limit.
- **Team paint is per material, not a whole-model tint.** The cooker flags which materials are car
  paint (everything except black/grey/glass/tyre names and anything ending in "light(s)") and stores
  a `shade`, its brightness relative to the brightest paint material. `SetPaintColor` then repaints
  only those, scaled by the shade — so `SportsCar`'s two-tone orange stays two-tone, and the Cop's
  light bar, the glass and the tyres keep their authored colours. Tinting the whole model instead
  would collapse the car into one flat blob.
- **Materials a mesh never references are dropped at cook time.** Several packs carry leftover
  materials that are all default grey (`NormalCar1` has an unused `Yellow`), and keeping them made
  the paint detection pick the wrong one.
- **Diffuse colours are converted linear → sRGB** and multiplied by `DiffuseFactor`, which is what
  reproduces the MTL's `Kd`. raylib renders without gamma correction, so skipping this leaves every
  car far too dark.
- **The model is fitted to the collision box at runtime**, not baked: `CarObject::Initialize` scales
  by length (`halfExtents.z * 2 / modelLength`) and lifts the model so its wheels rest on the bottom
  face. Any of the 7 cars can be dropped in via `CarObject::modelName` and it will fit. The pack
  models face **+Z** and the car drives towards **−Z**, hence `modelYawDegrees = 180`.
- **`assets::Path` resolves against `GetApplicationDirectory()`**, so the executable runs from any
  working directory. `TextureAsset` should reuse it rather than adding a second scheme.
- **The cooked model is optional.** If `assets/Models` is missing, `CarObject` falls back to the old
  box; the game still runs from an uncooked build instead of crashing.

### Lighting
Pulled forward from Milestone 14 because without it every material rendered as one flat colour and
the ball and floor were bare silhouettes. Milestone 14 still owns shadows, bloom and effects.

- **The low-poly look needs light, not textures.** The Cars-Park pack contains **no textures at all**
  — every `.mtl` is flat `Kd` values and the only image in `Game/Assets/` is a preview thumbnail.
  Nothing was missing from the cooker; what was missing was a shader. raylib's default shader is
  unlit (`texelColor*colDiffuse*fragColor`), so it never touches a normal and a sphere renders as a
  flat disc.
- **The normal is derived, not interpolated.** `Lit.fs` computes
  `cross(dFdx(fragPosition), dFdy(fragPosition))`, which gives the true face normal, constant across
  each triangle. This is why one shader flat-shades everything: the cooked cars carry per-face
  normals and would have shaded correctly from `vertexNormal`, but raylib's `GenMeshSphere` and
  `GenMeshCylinder` carry **smooth** normals, and reading those would have left the ball looking
  like a beach ball next to hard-edged cars. It also means anything added later (boost pads, arena
  trim, light rigs) is flat shaded for free with no per-mesh normal work.
- **`lighting::Detach` must be called before `UnloadModel` on any model that went through `Apply`.**
  `UnloadModel` → `UnloadMaterial` → `UnloadShader`, which would delete the one shared shader
  program and leave every other model drawing with a dead one. `Detach` hands the materials back
  raylib's default first. This is the single sharpest edge in the lighting code — a new lit model
  that forgets it will work fine until the scene is torn down.
- **The arena floor is a `Model`, not `DrawCube`.** `DrawCube` goes through the rlgl immediate batch,
  which pre-transforms vertices by `RLGL.State.transform` *and* uploads that same matrix as
  `matModel` — so a shader that computes `matModel*vertexPosition` would double-transform it. Going
  through `DrawModel` keeps one predictable path. The grid, centre circle and field outline stay
  unlit lines; do not wrap lines in the lit shader, their derivatives are degenerate (the shader
  falls back to an up normal, but it is meaningless).
- **Detecting a missing shader needs two checks, not one.** `IsShaderValid` only tests
  `id > 0 && locs != NULL`, and raylib answers a *missing file* with its **default** shader, whose
  id is perfectly valid — so the check silently passed and no warning was logged. `Load` now also
  compares against `rlGetShaderIdDefault()`. Worth remembering for `TextureAsset`.
- **The sun is mostly overhead** (`0.30, 0.90, 0.32`). The first version lit from behind the scene
  and every camera-facing surface fell into fill light; because the chase camera swings all the way
  around the car, only a high sun keeps whichever side faces the player readable.
- Light colours are constants in `Lighting.cpp` and set into the program once at load, since the
  light never moves. Milestone 13 can bind them to the ImGui panel.

### Camera
- **The ball cam is a direction, not a second camera.** It swaps the flat direction the camera sits
  behind — the car's heading for chase, the line from the ball through the car for ball cam — and
  slides the look point 30% of the way to the ball. Everything after that (smoothing, clamps, the
  occlusion pull-in) is shared, so the two modes cannot drift apart.
- **Ball cam gives way to the car's own heading over the last few metres** (`ballCamNearRange`, 7 m).
  The car-to-ball direction spins wildly as the car arrives at the ball, which is exactly the moment
  the player needs to see where the car is pointing. Without the blend the view whips round on
  contact.
- **`C` is read inside `ChaseCamera::Update`**, as CLAUDE.md 2.4 specifies. That is also why it does
  nothing while paused: `MatchScene::Update` returns before the camera is updated.
- **The occlusion ray starts at the car, not at the look point.** A physics body can never be inside
  static geometry, so the car is always a valid ray origin; the look point sits `lookHeight` above the
  car and is therefore *beyond the ceiling* whenever the car is driving on it, which would make every
  cast report a hit at zero distance and collapse the camera onto the car.
- **The margin belongs inside `FreeReach`, not at the call sites.** Subtracting it unconditionally —
  including when nothing was hit — pulls the camera 0.6 m closer every frame, and because the clamped
  position is what gets stored, it compounds: the camera settled 4 m from the car in open field.
  Measured and fixed; the open-field rows in the table above are the regression test.
- **When the view back is blocked the camera trades distance for height**, because up is the one
  direction that is always open — every concave join in this arena curves away from the floor. The
  distance lost is whatever the geometry takes, but the height is bought back on a *squared* curve, so
  a wall a few metres behind the car barely lifts the view while a ramp right against it goes
  overhead. Linear lift measured 6.09 m of height for a car parked 6 m from a wall, which read as a
  top-down view in an ordinary situation.
- **The lift has to drag the look point back to the car**, or ball cam keeps aiming a third of the way
  to the ball while the camera climbs and the car drops out of the bottom of the frame. That was
  measured: the corner-plus-ball-cam case lost the car until `desiredTarget` was blended back.
- **The desired position is clamped under the ceiling *before* the occlusion test.** The unclamped
  mark is `height` above the car, which is outside the arena whenever the car is on the ceiling, so
  the test would read as fully blocked for the whole time the car spends up there and jam the camera
  1.5 m away. Clamped first, the ceiling run keeps the normal 9.5 m trail.
- **The pulled-in position is what gets stored**, so the camera eases back out as the obstruction
  clears instead of snapping the frame the ray stops hitting.
- **Only `physics::Arena` blocks the view.** The ball and the other car are on their own layers, so
  something passing behind the car can never yank the camera in.
- **`minDistance` (1.5 m) is a last resort and is the one clamp that can still put the camera in a
  wall.** The lift is what normally keeps the pull-in from ever getting that short. Raising it back
  towards the old 2.5 m re-introduces the clipping it was measured to cause on the ramp.
- **`cameraSensitivity` scales the smoothing rates.** There is no mouse look for it to mean anything
  else, and this is the one number that changes how the camera feels: 0.5 takes 59 frames to recover
  from a shove, 2.0 takes 14. `MatchScene` copies it in every frame rather than at `Initialize`, so
  changing it in the pause menu is felt as soon as play resumes.

### Car selection
- **The pick lives in `GameSettings::playerCarModel`,** a model name string, even though it is not a
  row in the settings panel. That struct is the one thing every scene can already see, so it is what
  carries the choice from the picker to `MatchScene` and back again — the picker re-selects whatever
  is stored when it opens, so returning to the menu and pressing Play twice keeps your car. The
  default is `SportsCar`, which is also what the smoke test gets, since it skips both menus.
- **`MenuAction::SelectCar` sits between the menu and the match.** Play no longer starts a match; it
  asks `App` for `CarSelectScene`, and only that scene raises `StartMatch`. Nothing else changed in
  the scene-switching flow.
- **The six previews are the real cooked models, drawn directly rather than through `CarObject`.**
  A `CarObject` would need a `PhysicsSystem`, a body and a controller for a rotating showroom prop.
  `StaticModelAsset` already loads, paints and lights a model on its own, so the scene has no
  physics at all and holds nothing but six of those.
- **The cars stand in the plane z = 0, not on a floor.** Laid out on a floor the back row would be
  further from the camera and would render smaller; standing them in a plane facing the camera keeps
  all six the same size, which is what makes it read as a grid rather than a diorama. Each is fitted
  to the same 3.8 m length (`PREVIEW_LENGTH`) so the pack's own scale differences — an SUV is 4.2 m
  and a NormalCar2 3.3 m — do not make one cell look bigger than another.
- **The camera is above the top row and tilted down.** A level camera looking at the middle of the
  grid puts the top row overhead and shows the underside of those three cars, which looked broken.
  Anything that moves the rows has to keep `camera.position.y` above `ROW_Y[0]`.
- **Each cell rectangle is projected from its own pedestal, not laid out in screen space.** The two
  rows are seen from different angles, so a fixed pixel box around the projected centre fits one row
  and misses the other; projecting the top of the car, the near edge of the pedestal and both sides
  gives a frame and a label position that follow the car at any window size. This is also the mouse
  hit box, so hover, click and the drawn frame can never disagree.
- **The labels are display names, not the asset names.** `NormalCar1` and `NormalCar2` show as COUPE
  and COMPACT, `Cop` as POLICE. The asset name is the first field of the same `CARS` table, so the
  two cannot drift apart.
- **Mouse selection lands a frame after keyboard selection.** Grid keys are read before the 3D pass
  and hover after it (the cells only exist once projected), so clicking a car draws its 2D frame
  immediately but grows its pedestal on the next frame. Not worth splitting the pass in two.
- **`Shutdown` unloads all six models**, and `StaticModelAsset::Unload` detaches the lit shader
  first — the same trap as everywhere else in this project.

### How to play screen
- **It is a scene, not an overlay on the main menu**, so it gets the whole window for six panels and
  cannot fight the menu for the Esc key. It reaches `App` the same way every other screen does, via
  `MenuAction::HowToPlay`.
- **The text is data, not draw calls.** Two arrays of `Section` (a heading plus null-terminated
  lines) are walked by one `DrawSection`, which returns the height it used so the column stacks
  itself. Adding or removing a line cannot break the layout, which is the failure the settings panel
  had before `PreferredHeight` was derived.
- **Keep lines under about 45 characters.** That is what fits a 540-unit column at 720p with the
  raylib default font; longer lines run out of the panel rather than wrapping. Continuation lines are
  indented by hand, as in the boost section.
- **Only ASCII.** The default font has no dashes or arrows beyond `-`, so em dashes render as boxes.

### Car selection: right click
- **Right click on a car is the whole choice in one action** — that car, and start the match — while
  left click still only highlights, so the grid can be browsed with either button. Right clicking the
  car that is already highlighted therefore behaves exactly like pressing START MATCH.
- The click is collected in the same cell loop that does hover and left click, into `rightClicked`,
  and applied after the buttons are drawn. It cannot be handled inside the loop, because `selected`
  is what the start path reads and the buttons have not been drawn yet at that point.

### Menus
- **`uistyle::Button` is the shared standalone button** used by the car picker and the how-to-play
  screen. It was a file-static in `CarSelectScene.cpp` until the second screen needed one. It is
  mouse-only on purpose: on both screens the keyboard is already driving something else (the car grid,
  or nothing at all), and Enter/Esc are handled by the scene.
- **`App` owns the single `GameSettings`** and hands each scene a pointer, which is what makes the
  settings shared between the main menu and the pause menu. The `SettingsMenu` *widget* is per scene
  (so each keeps its own cursor), but it edits the one shared struct.
- **Scenes never switch scenes themselves.** They set `Scene::pendingAction`, and `App::Run` acts on
  it *after* `EndDrawing`, so no frame ever draws a scene that has just been deleted.
- **Menus are immediate mode**: `MenuList` and `SettingsMenu` draw and handle input in the same call,
  which is the raylib idiom and avoids computing the row rectangles twice. This is why menu logic
  lives in `Draw` rather than `Update`.
- **Esc is handled by the scene that owns the settings panel**, never inside `SettingsMenu`, so a
  single key press cannot both close the panel and toggle pause in the same frame.
- `SettingsMenu::PreferredHeight()` is derived from the layout constants, so adding a row cannot
  silently overflow the panel — the first version had exactly that bug, with "Back" hanging outside.
- `SetExitKey(KEY_NULL)` in `App::Initialize`: Esc must pause, not kill the window. Quitting goes
  through `MenuAction::ExitGame` and the `App::running` flag.
- The smoke test starts **directly in the match**, skipping both the menu and the car picker, because
  that is the scene that needs rendering validation. It therefore always drives the default car.

## Known deviations / things to be aware of

- **Assets folder name.** CLAUDE.md says `Game/Assets/Cars-pack/`; on disk it is
  `Game/Assets/Cars-Park/`.
- **There are still no shadows.** Everything is lit by one directional light plus a hemisphere fill,
  with no occlusion, so nothing casts onto anything else and the cars do not sit into the floor as
  firmly as they could. Milestone 14 owns that, along with bloom. Do **not** bake shading into
  vertex colours as a substitute — the lit shader would then double-darken the model.
- **The OBJ+MTL copies in `Game/Assets/Cars-Park/OBJ/` are now unused by the build.** They are worth
  keeping as the reference the FBX reader is validated against
  (`Tools/FbxReader.py` output matched them exactly), but nothing loads them at runtime.
- **`R` resets the car only.** It briefly also re-centred the ball while the field had no walls;
  now that the arena is closed the ball cannot be lost, so that was dropped again.
- **`GameSettings::matchDurationMinutes` is now read** (at `MatchScene::Initialize`), so changing it
  mid-match has no effect until the next match. That is the intended behaviour, not a bug.
- **Full time just stops.** The match freezes and shows FULL TIME with the result; there is no
  post-match screen or rematch flow. Esc still opens the pause menu to leave. Worth revisiting when
  the HUD lands in Milestone 11.
- **The Release build prints GCC `-O3` warnings from inside Jolt.** Known third-party false
  positives, not project errors.
- **Air roll is on Q/E, which CLAUDE.md section 8 does not list.** That section says WASD maps to
  pitch, yaw and roll, but two axes of input cannot drive three axes of rotation independently, and
  `CarController.h` in section 2.4 does specify separate pitch/yaw/roll fields. W/S is pitch and A/D
  is yaw; roll needed somewhere, and two extra keys beat a modal air-roll modifier.
- **The righting assist no longer fights aerials but still runs on a normal landing.** A car that
  lands upside down with no input rights itself as before; a car being deliberately rotated is left
  alone. Both are verified.
- `CarInput` carries `throttle`, `steer`, `boost`, `jump`, `airPitch`, `airYaw`, `airRoll` and
  `reset` — everything the bot in Milestone 12 needs.
- **There is no boost flame or trail yet.** `CarObject::boosting` is set every step and the meter
  brightens while held, but the visual effect belongs to Milestone 14's `Effects.h/cpp`.
- `.venv` does not exist yet; the Makefile falls back to `python3`, which is fine while the cooker
  has no dependencies. Create it before the cooker starts using Pillow/numpy:
  `python3 -m venv .venv && .venv/bin/pip install -r Tools/requirements.txt`.
- **The camera can still sit inside the goal recess**, behind the goal line, when the car is in its
  own net. That is deliberate — it is where the view has to be — and the probe confirms it is not
  inside solid geometry there.
- **Rocket League also has 45-degree diagonal corner walls, which this arena does not.** The corners
  are rounded now, but the arena is still fundamentally a rectangle with rounded edges rather than an
  octagon. That is arena-shape work, not ramp work.
- **A box has no wheels, so the car cannot climb steps.** A vertical lip taller than a few
  centimetres stops it dead: the contact normal barely tilts up, so lift never beats the car's
  weight. Rounding the box enough to climb would make the underside nearly cylindrical and wreck
  stability, so the fix is to **keep the arena floor smooth** — build ramps and wall transitions as
  slopes, never as steps. The wall transitions are now exactly that (see the ramp notes above, and
  the measured "biggest backward step 0.000 m"); keep it true for boost pads and any Milestone 14
  trim.
- **Some settings are stored but not consumed yet**, because the systems that read them do not exist:
  bot enabled (Milestone 12) and the master/SFX volumes (Milestone 15 — the audio device is not even
  initialised). Fullscreen, resolution, camera sensitivity and the post-processing flag are the ones
  with an effect today, and post-processing is just a stored flag until Milestone 14.
- **Settings live for the session only.** Nothing is written to disk; Milestone 13 introduces the
  config file, and that is the natural place to persist them. The picked car is in the same struct,
  so it is forgotten on exit along with everything else.
- **`SportsCar2` is deliberately not in the picker** (CLAUDE.md Milestone 08 says to use six of the
  seven). It is still cooked and still loadable by name.
- **All six previews are painted blue,** because the player is always on the blue team. If teams ever
  become selectable, `SetPaintColor` in `CarSelectScene::Initialize` is the one place to change.

## Next steps — Milestone 11 (HUD)

1. Move the score, the clock, the boost meter and the state banners out of `MatchScene` into
   `HUD.h/cpp`. They are currently drawn by `DrawMatchStatus` and `DrawBoostMeter`, which were always
   meant as placeholders — everything they draw already scales through `uistyle::Scale()`, so the move
   is mostly relocation plus the pieces that are missing.
2. What is missing: boost-pad state hints, a real goal celebration banner (there is a plain "GOAL!"
   today) and a kickoff countdown treatment beyond the current number.
3. The debug readouts in `MatchScene::Draw` — speed, GROUNDED/AIRBORNE, the camera mode and the
   controls line — should either become part of the HUD or move behind `GAME_DEV_TOOLS`. They are not
   shipping UI.
4. Full time still just freezes the field with a FULL TIME banner and no rematch flow; the HUD is the
   natural place to give it a proper end-of-match screen.

The opponent car (Milestone 12) needs no new asset work: give the second `CarObject` a different
`modelName` and the orange `teamColor` before `Initialize()` and it cooks, fits and paints itself.
It should not be given `settings->playerCarModel` — that field is the player's car; pick the bot's
from the same `CARS` table in `CarSelectScene.cpp`, or simply leave it on the `SportsCar` default.

### Verifying without a keyboard
There is no `xdotool`/`wtype` here, so every milestone so far was verified with a temporary harness
that was deleted afterwards. Three patterns, in increasing order of effort:
- **Scripted controller** — replace `PlayerController::Poll` with a time-based script (Milestones 02,
  04).
- **Measurement harness** — a temp `.cpp` that drives fixed routines, repositions the car per phase
  with `ResetTo`, and prints computed metrics rather than raw rows (Milestone 04). Much easier to
  judge than reading telemetry by eye. Milestone 05 refined this: call it from `Initialize` and step
  `physicsSystem.Update` directly in a tight loop, so 30 simulated seconds run in a fraction of a
  second and nothing has to render. Read the tunables from env vars too — a whole sweep then costs
  one build instead of one per value. One trap worth remembering: sample the *cause* before the
  event, not after. Reading the car's speed once the ball started moving reported a 1.6x speed
  transfer, because the collision had already bled a quarter of the car's speed by then; sampling
  while the ball was still at rest showed the real figure of 1.07x.
- **Input shim** — force-include a header that `#define`s `IsKeyPressed` to a scripted driver, for
  testing menus and other real key handling (Milestone 03). Note ImGui also declares `IsKeyPressed`,
  so `#undef` it in `App.cpp`, and declare the shim `extern "C"` to match raylib's linkage.
