# HANDOFF — Arcade Car Soccer

Project state for whoever continues the work. Update this file whenever the project changes.

## Current state

**Milestone 01 — Project Structure: DONE.**
**Milestone 02 — Base Scene and Car Movement: DONE.**
**Milestone 03 — Main Menu and Pause Menu: DONE.**
**Milestone 04 — Car Handling and Feel: DONE.**
**Asset pipeline — FBX cooking + car models (CLAUDE.md 4.1): DONE.**
**Milestone 05 — Ball and Car-Ball Interaction: DONE.**
**Lighting — pulled forward from Milestone 13 on request: DONE.**
**Milestone 06 — Arena, Goals and Scoring: DONE.**
**Milestone 07 — Boost System and Boost Pads: DONE.**

The game opens on a main menu (Play / Settings / Exit). Play starts a real match: an enclosed
55 x 80 m arena with side walls, back walls, a ceiling and two coloured goals; a player car driven
with WASD/arrows on a Jolt rigid body; a ball; a score, a match clock and a kickoff countdown; and
a smooth third-person chase camera. A goal resets the field and counts down again, and the match
ends at full time. The car draws a real low-poly model from the Cars-Park pack, cooked from FBX and
painted in the team colour, and the whole scene is flat-shaded by one directional light. Shift
boosts, drawing on a 0-100 meter refilled by 18 boost pads spread across the field. Esc or P
pauses, with Resume / Settings / Return to main menu / Exit. No jumping, air control or opponent yet.

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

## Layout

```
Game/Source/
  Main.cpp, App.h/cpp          entry point, window, settings, scene switching, smoke test
  Scene.h/cpp                  camera + GameObjects + Jolt PhysicsSystem, fixed step loop
  MainMenuScene.h/cpp          title, Play / Settings / Exit, credits and build string
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
  GameObjects/ArenaObject.h/cpp floor, walls, ceiling, back walls with goal openings
  CarController.h              CarInput + abstract controller
  PlayerController.h/cpp       keyboard input
  ChaseCamera.h/cpp            third person follow camera
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

Controls so far: **WASD / arrows** drive and steer, **Shift** boosts (held), **R** resets the car,
**Esc** or **P** pauses.
In menus: **up/down** or **W/S** move, **Enter**/**Space** activate, **left/right** change a value,
and the mouse works too.

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
  and Development only — gate all ImGui tuning UI behind it (Milestone 12).

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
- **Boost is applied before the grounded gate**, so it already works in the air. It is the one
  control that must keep working off the ground, and Milestone 08 builds aerials on top of it.
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
  drift away from what is drawn — worth preserving when Milestone 13 adds trim and stands.
- **The ceiling has collision but is never drawn.** Drawing it would put a slab between the chase
  camera and the field the moment the car climbs a wall.
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
Pulled forward from Milestone 13 because without it every material rendered as one flat colour and
the ball and floor were bare silhouettes. Milestone 13 still owns shadows, bloom and effects.

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
  light never moves. Milestone 12 can bind them to the ImGui panel.

### Menus
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
- The smoke test starts **directly in the match**, skipping the menu, because that is the scene that
  needs rendering validation.

## Known deviations / things to be aware of

- **Assets folder name.** CLAUDE.md says `Game/Assets/Cars-pack/`; on disk it is
  `Game/Assets/Cars-Park/`.
- **There are still no shadows.** Everything is lit by one directional light plus a hemisphere fill,
  with no occlusion, so nothing casts onto anything else and the cars do not sit into the floor as
  firmly as they could. Milestone 13 owns that, along with bloom. Do **not** bake shading into
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
  the HUD lands in Milestone 10.
- **The Release build prints GCC `-O3` warnings from inside Jolt.** Known third-party false
  positives, not project errors.
- `CarInput` carries `throttle`, `steer`, `boost` and `reset`. Jump and air control fields get added
  by the milestones that implement them, rather than sitting unused.
- **There is no boost flame or trail yet.** `CarObject::boosting` is set every step and the meter
  brightens while held, but the visual effect belongs to Milestone 13's `Effects.h/cpp`.
- `.venv` does not exist yet; the Makefile falls back to `python3`, which is fine while the cooker
  has no dependencies. Create it before the cooker starts using Pillow/numpy:
  `python3 -m venv .venv && .venv/bin/pip install -r Tools/requirements.txt`.
- The chase camera has no wall-occlusion handling yet (Milestone 09 / 6.2); it only clamps its own
  height above the floor.
- **A box has no wheels, so the car cannot climb steps.** A vertical lip taller than a few
  centimetres stops it dead: the contact normal barely tilts up, so lift never beats the car's
  weight. Rounding the box enough to climb would make the underside nearly cylindrical and wreck
  stability, so the fix is to **keep the arena floor smooth** — build ramps and wall transitions as
  slopes, never as steps. The Milestone 06 arena honours this (the floor is one flat slab and the
  walls meet it at a right angle); keep it true for boost pads and any Milestone 13 trim.
- **The righting assist runs while airborne within `recoveryProbe` (1.3 m) of the ground.** That is
  what makes bump landings clean now, but Milestone 08 must gate it off while the player is giving
  air input, or it will fight deliberate aerial rotation.
- **Some settings are stored but not consumed yet**, because the systems that read them do not exist:
  camera sensitivity (Milestone 09), bot enabled (Milestone 11), and
  the master/SFX volumes (Milestone 14 — the audio device is not even initialised). Fullscreen,
  resolution and the post-processing flag are the only ones with an effect today, and post-processing
  is just a stored flag until Milestone 13.
- **Settings live for the session only.** Nothing is written to disk; Milestone 12 introduces the
  config file, and that is the natural place to persist them.

## Next steps — Milestone 08 (Jumps, Flips and Air Control)

1. Add `jump` to `CarInput` (edge triggered, unlike `boost` which is held) and jump/double-jump
   state to `CarObject`: an impulse on the first press, a second jump or a directional flip on the
   second, reset once the car lands.
2. **Gate the righting assist off while the player is giving air input.** It currently runs whenever
   the car is within `recoveryProbe` (1.3 m) of the ground, airborne or not, and it will fight
   deliberate aerial rotation. See the note above.
3. Air control replaces the `if (!grounded) return;` early-out in `CarObject::Update`. Boost is
   already applied before that gate, so it keeps working; pitch/yaw/roll torques go after it.
4. Keep every new rate in per-second units converted with the step, as `grip` and `tumbleDamping`
   are — this has already caused one silent bug.
5. Aerial ball touches are the goal: the ball sits at 1.25 m and a top-speed hit peaks around 7 m,
   so the car needs to reach roughly that height under boost.

The opponent car (Milestone 11) needs no new asset work: give the second `CarObject` a different
`modelName` and the orange `teamColor` before `Initialize()` and it cooks, fits and paints itself.

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
