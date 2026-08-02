# HANDOFF — Arcade Car Soccer

Project state for whoever continues the work. Update this file whenever the project changes.

## Current state

**Milestone 01 — Project Structure: DONE.**
**Milestone 02 — Base Scene and Car Movement: DONE.**
**Milestone 03 — Main Menu and Pause Menu: DONE.**
**Milestone 04 — Car Handling and Feel: DONE.**
**Asset pipeline — FBX cooking + car models (CLAUDE.md 4.1): DONE.**
**Milestone 05 — Ball and Car-Ball Interaction: DONE.**

The game opens on a main menu (Play / Settings / Exit). Play starts the match scene: an arena
floor, a player car driven with WASD/arrows on a Jolt rigid body, a ball to hit around, and a
smooth third-person chase camera. The car draws a real low-poly model from the Cars-Park pack,
cooked from FBX and painted in the team colour. Esc or P pauses, with Resume / Settings / Return
to main menu / Exit. The settings panel is one widget reused by both menus. No walls, goals,
scoring, jumping or boost yet.

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

## Layout

```
Game/Source/
  Main.cpp, App.h/cpp          entry point, window, settings, scene switching, smoke test
  Scene.h/cpp                  camera + GameObjects + Jolt PhysicsSystem, fixed step loop
  MainMenuScene.h/cpp          title, Play / Settings / Exit, credits and build string
  MatchScene.h/cpp             the gameplay scene, plus the pause overlay
  GameSettings.h               the settings struct App owns
  MenuAction.h                 what a scene asks App to do next
  UserInterface.h/cpp          namespace uistyle: colors, text helpers, MenuList widget
  SettingsMenu.h/cpp           settings panel reused by both menus
  GameObject.h/cpp             base object, body transform helpers
  StaticModelAsset.h/cpp       .evmodel loader + assets::Path, team repaint
  GameObjects/CarObject.h/cpp  arcade car (single box body + cooked car model)
  GameObjects/BallObject.h/cpp the ball (single dynamic sphere)
  GameObjects/ArenaObject.h/cpp floor only for now
  CarController.h              CarInput + abstract controller
  PlayerController.h/cpp       keyboard input
  ChaseCamera.h/cpp            third person follow camera
  PhysicsLayers.h/cpp          namespace physics: layers and Jolt filters
Game/ThirdParty/   raylib, Jolt, glm, imgui     (cloned, git-ignored)
Game/Assets/       Cars-Park/ (OBJ, FBX, Blends, License.txt, Preview.png)
Tools/             AssetCooker.py, FbxReader.py, requirements.txt
Build/<Config>/    ArcadeCarSoccer + assets/Models/*.evmodel
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

Controls so far: **WASD / arrows** drive and steer, **R** resets the car, **Esc** or **P** pauses.
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
- **Nothing is lit yet.** The car models render with raylib's default unlit shader, so each material
  is one flat colour — consistent with the rest of the scene, which is also unlit, but the cars read
  as silhouettes rather than shapes. The cooked meshes already carry per-face normals, so Milestone
  13's lighting shader has what it needs. Do **not** bake shading into vertex colours in the
  meantime; a real light would then double-darken the model.
- **The OBJ+MTL copies in `Game/Assets/Cars-Park/OBJ/` are now unused by the build.** They are worth
  keeping as the reference the FBX reader is validated against
  (`Tools/FbxReader.py` output matched them exactly), but nothing loads them at runtime.
- **There are no walls yet** (Milestone 06), so driving off the floor edge makes the car fall
  forever, and a hard hit can put the ball off the field. `R` now re-centres **both** the car and
  the ball for exactly that reason — drop the ball half of it once the arena is closed if it stops
  being useful.
- **The Release build prints GCC `-O3` warnings from inside Jolt.** Known third-party false
  positives, not project errors.
- `CarInput` currently only carries `throttle`, `steer` and `reset`. Jump, boost and air control
  fields get added by the milestones that implement them, rather than sitting unused.
- `.venv` does not exist yet; the Makefile falls back to `python3`, which is fine while the cooker
  has no dependencies. Create it before the cooker starts using Pillow/numpy:
  `python3 -m venv .venv && .venv/bin/pip install -r Tools/requirements.txt`.
- The chase camera has no wall-occlusion handling yet (Milestone 09 / 6.2); it only clamps its own
  height above the floor.
- **A box has no wheels, so the car cannot climb steps.** A vertical lip taller than a few
  centimetres stops it dead: the contact normal barely tilts up, so lift never beats the car's
  weight. Rounding the box enough to climb would make the underside nearly cylindrical and wreck
  stability, so the fix is to **keep the arena floor smooth** — build ramps and wall transitions as
  slopes, never as steps. Worth remembering when Milestone 06 builds the arena.
- **The righting assist runs while airborne within `recoveryProbe` (1.3 m) of the ground.** That is
  what makes bump landings clean now, but Milestone 08 must gate it off while the player is giving
  air input, or it will fight deliberate aerial rotation.
- **Some settings are stored but not consumed yet**, because the systems that read them do not exist:
  camera sensitivity (Milestone 09), match duration (Milestone 06), bot enabled (Milestone 11), and
  the master/SFX volumes (Milestone 14 — the audio device is not even initialised). Fullscreen,
  resolution and the post-processing flag are the only ones with an effect today, and post-processing
  is just a stored flag until Milestone 13.
- **Settings live for the session only.** Nothing is written to disk; Milestone 12 introduces the
  config file, and that is the natural place to persist them.

## Next steps — Milestone 06 (Arena, Goals and Scoring)

1. Extend `ArenaObject` with side walls, back walls and a ceiling, all static boxes on the
   `physics::Arena` layer. **Keep every surface smooth** — see the "box has no wheels" note above:
   build transitions as slopes, never steps, or the car will stop dead against them.
2. Size the ceiling against the ball's arc: a top-speed hit peaks around 7 m, so a ceiling below
   ~12 m will change how the game plays. The ball already uses `LinearCast`, so thin walls are safe.
3. Add `GameObjects/GoalObject.h/cpp` with a **sensor** body (`physics::Trigger` layer,
   `BodyCreationSettings::mIsSensor`). Goal detection must only fire when the ball is **fully**
   across the line, so either inset the sensor by the ball radius (1.25 m) or test the ball's
   centre against the goal plane rather than relying on first overlap.
4. Add `Match.h/cpp` for score, the match timer (`GameSettings::matchDurationMinutes` is already
   stored but unread) and the kickoff countdown, then reset with `CarObject::ResetTo` and
   `BallObject::ResetTo`, which both already zero the velocities.
5. Once the arena is closed, reconsider whether `R` should still re-centre the ball.

Note the layer filters in `PhysicsLayers.cpp` already allow Ball vs Car, Ball vs Arena and Ball vs
Ball, and a `Trigger` layer exists, so no collision-filter work should be needed.

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
