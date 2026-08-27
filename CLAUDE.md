# CLAUDE.md

Behavioral guidelines to reduce common LLM coding mistakes. Merge with project-specific instructions as needed.

**Tradeoff:** These guidelines bias toward caution over speed. For trivial tasks, use judgment.

## 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

## 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

## 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

## 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

Strong success criteria let you loop independently. Weak criteria ("make it work") require constant clarification.

---

**These guidelines are working if:** fewer unnecessary changes in diffs, fewer rewrites due to overcomplication, and clarifying questions come before implementation rather than after mistakes.

# GUIDELINES
- Do not run git add, git commit or git push commands
- Maintain visual consistency with the rest of the application (colors, spacing, etc).
- When the user requests a git commit, only generate a one-line description of the changes made, in English. Example: "refactor: move and enhanced script VAPID, add docs and hosting"
- Create a markdown document `./HANDOFF.md` with the actual project state, decisions made, next steps and any important context for a different agent to continue the project. Whenever the project is modified, update the `./HANDOFF.md` document. > [!IMPORTANT]
- Create checklist of the milestones that already made of in the `./HANDOFF.md` file. If the milestone is already made put a "x" in the checkbox `[ ]` using the markdown format

# PROJECT
# Arcade Car Soccer (Rocket League-style)

A **third-person**, **3D**, **arcade** car-soccer game for **Linux**.

Rocket-powered cars play soccer inside an enclosed futuristic arena. The player drives a responsive car, boosts, jumps, double-jumps/flips, and controls the car in the air (pitch, yaw, roll) to hit a large, heavy-but-bouncy ball into the opponent's goal. A match has a timer and a score; when the ball fully crosses a goal line the score updates, a celebration plays, and the field resets with a kickoff countdown. Boost is a 0–100 meter that drains while held and is refilled by boost pads spread around the arena. A single bot opponent chases the ball and aims roughly at the player's goal; if the bot proves too risky, ship a solo practice mode with two working goals and a scoreboard instead.

**The first thing shown after the main menu is the actual match — not a landing page.** Prioritize *fun over realism*: strong acceleration, drift-friendly turning, stable recovery, and a ball that feels weighty but reacts sharply. The quality bar is an arcade sports game someone would actually play for five minutes, so spend the extra effort tuning car handling, camera smoothing, ball impact, boost feel, and goal feedback. A technically complete but boring physics demo is not acceptable.

---

# 1. Technology and Structure

## 1.1. Tools and Platforms
- **Platform:** Linux
- **Main language:** C++.
  - **Compiler:** GCC (g++) and Makefile
- **External libraries (ThirdParty):**
  - **Graphics:** [raylib](https://github.com/raysan5/raylib)
  - **Physics:** [Jolt](https://github.com/jrouwe/joltphysics)
  - **Math:** [glm](https://github.com/g-truc/glm)
  - **Debug/tuning interface:** [Dear ImGui](https://github.com/ocornut/imgui)
- **Tools language:** Python.

> **Do not** use web technologies (Three.js, Rapier.js, Vite, TypeScript, etc.). This is a native C++ build using raylib + Jolt. The original design reference was a browser game; the stack here is intentionally different.

## 1.2. Build Modes
| Mode | Use | Debug symbols | Dev tools (tuning panels, debug UI) | Performance |
|------|-----|:---:|:---:|-------------|
| **Debug** | Development | Yes | Yes | Slower (symbols) |
| **Development** | Development | Yes | Yes | Better than Debug |
| **Release** | Final build (shipping) | No | No | Speed-focused |

- **Debug** and **Development** are aimed at development: they contain debug symbols and the in-engine tuning tools (physics/handling panels, colliders visualization).
- **Release** is the final build that will be distributed: no symbols, speed-focused, no tuning tools. The essential difference from Development is precisely the absence of those tools.

## 1.3. Project Structure
```
Game/
  Source/            # C++ source code of the game
  ThirdParty/        # External libraries (raylib, Jolt, glm, ImGui)
  Assets/            # Source assets (textures, UI, sounds, optional car model)
Tools/
  AssetCooker.py     # Asset cooking tool
Build/
  Debug/
  Development/
  Release/
  Intermediate/
    Debug/
    Development/
    Release/
Makefile
README.md # Description about the game, controls, how to run and checklist of the milestone that already or not already made for the game using the milestone from CLAUDE.md as refers. 
.gitignore 
```

## 1.4. Code Conventions

**Naming:**
- **PascalCase** for class, struct, function, and method names.
- **camelCase** for variable names.
- Short, single-word (or abbreviated) namespaces, **all lower case**.
- Macros in **ALL_UPPER_CASE**.
- File names always in **PascalCase**.
- Use include guards (`#ifndef` / `#define`) in headers.

**Programming style:**
- Keep the code simple and to the point.
- Don't create functions unnecessarily. If something can be done inline and will only be used in a single place, leave it inline.
- Don't use private variables with getters/setters without a clear reason. Prefer simple structures with public variables.
- Before implementing a feature, check whether it already exists. Focus on reusing what's already there.
- Comment only what isn't immediately clear from reading the code.
- You may use the STL (`std::string`, `std::vector`, `std::unordered_map`, etc.).
- If something can be done with the language's standard libraries, prefer that approach.
- **Don't use smart pointers.**

---

# 2. Code Structure

Code lives in `Game/Source/`.

## 2.1. Application Core
- **Main.cpp** — Program entry point.
- **App.h/cpp** — Main game class. Responsible for the window, input system, active scene management, and running the game loop. Also coordinates the smoke test modes (headless validation with screenshots).
- **Scene.h/cpp** — Base class for scenes. Each scene contains the camera in use, the list of GameObjects, the game interface, and the physics system (Jolt). Manages this data and runs the logic of each object.
- **MainMenuScene.h/cpp** — The game's main menu scene.
- **MatchScene.h/cpp** — The gameplay scene: arena, cars, ball, goals, boost pads, HUD, and the match state machine (kickoff countdown → play → goal → reset).
- **GameSettings.h** — Struct with the game settings (fullscreen, resolution, mouse/camera sensitivity, audio volumes, match duration, bot enabled).

## 2.2. GameObjects
- **GameObject.h/cpp** — Base class. Each instance represents an object in the 3D world with its own shape, logic, and physics. Specialized by the child classes in `Game/Source/GameObjects/`. Exposes `Initialize`, `Update`, `Draw`, and `GetWorldBounds` (AABB).

Specializations in `Game/Source/GameObjects/`:
- **CarObject** — The rocket car. Drives, jumps, double-jumps/flips, boosts, and has air control. Reused for both the player-controlled car and the bot (the controller decides the inputs — see 2.4). The **visual mesh comes from the low-poly `Cars-Pack`** (see section 4); the physics body stays a simple box driven by arcade forces (2.5). The visual model is only attached to that box for rendering — it does not drive the simulation.
- **BallObject** — The soccer ball (a single dynamic sphere body, heavy but responsive).
- **ArenaObject** — Static arena geometry: floor, curved/side walls, back walls, and a ceiling or high invisible bound. All collision is static.
- **GoalObject** — A colored goal with a **trigger volume**. Fires a goal event when the ball fully crosses the goal line into the net.
- **BoostPadObject** — A pad with a trigger volume that refills a car's boost on overlap, then goes on cooldown and glows again when ready.

## 2.3. Interface and Menus
- **UserInterface.h/cpp** — Single source of truth for the HUD/menu style (namespace `uistyle`: panel, border, accent, team-blue, team-orange, and text colors, plus a color reserved for warnings). All UI drawing must go through these values.
- **HUD.h/cpp** — In-match HUD: score, match timer, boost meter, kickoff countdown, boost pad hints, and the goal celebration banner.
- **SettingsMenu.h/cpp** — Modular settings menu, reused in both the main menu and the pause menu.
- **MenuAction.h** — Enum of menu actions (`StartMatch`, `MainMenu`, `ExitGame`, `Resume`, etc.).
- **ImGuiRaylib.h/cpp** — Dear ImGui integration with raylib (namespace `imgui`).

## 2.4. Game Systems
- **Match.h/cpp** — Match state machine and rules: score per team, match timer, kickoff countdown, goal detection handling, and the reset that re-centers ball and cars after each goal.
- **CarController.h** — Abstract input source for a `CarObject` (throttle, steer, jump, boost, air pitch/yaw/roll). Two implementations:
  - **PlayerController.h/cpp** — Reads keyboard input.
  - **BotController.h/cpp** — Simple AI: drive toward the ball, aim the hit roughly toward the player's goal, boost occasionally. Must be **beatable**, not perfect. If the bot proves too risky, disable it and run **solo practice** (two goals, working scoreboard, no opponent).
- **ChaseCamera.h/cpp** — Smooth third-person chase camera that follows the car's rotation and velocity while staying readable, plus a **ball-cam** toggle that keeps the ball framed. Handles the `C` toggle and interpolation/smoothing.
- **AudioSystem.h/cpp** — Audio system with procedural cues (`AudioCue`).

## 2.5. Rendering and Physics
- **Effects.h/cpp** — Visual feedback: boost flame/trail, ball highlight, boost-pad glow, goal-explosion particles. Kept intentionally simple (raylib primitives + billboards).
- **PostProcess.h/cpp** — Optional post-processing (bloom / subtle color grading) applied only if performance allows; toggleable.
- **PhysicsLayers.h/cpp** — Jolt collision layers (namespace `physics`): `Car`, `Ball`, `Arena` (static), and non-solid `Trigger` layers for goals and boost pads.

> **Physics design note (non-obvious, worth stating):** model the car as a **single dynamic rigid body** (a box) with a raycast/ground-contact check and custom arcade forces — throttle, steering torque, jump/flip impulses, and air-control torques — rather than Jolt's full wheeled `VehicleConstraint`. This keeps handling responsive, keeps air control simple, and avoids the twitchy feel of a full suspension sim. Prioritize fun over realism.

## 2.6. Runtime Assets
- **StaticModelAsset.h/cpp** — Static models (e.g. an optional car model) with local bounds. The car and arena may instead be built **procedurally** from raylib primitives — prefer procedural where it looks good enough (see section 4).
- **TextureAsset.h/cpp** — Loading of textures in the cooked format.

## 2.7. Utilities
- **SystemInfo.h/cpp** — Basic hardware/GPU info (used to pick sensible default graphics settings).

---

# 3. Tools

The development support tools are written in **Python**. The main one is the **Asset Cooker** (`Tools/AssetCooker.py`). Keep it **lightweight**: this game leans on procedural geometry, so the cooker mostly prepares textures and UI, not a heavy model/animation pipeline.

## 3.1. Asset Cooker
The Asset Cooker walks through the source assets in `Game/Assets/` and converts them into custom binary formats that the engine reads quickly. **The Asset Cooker runs as a build event.**

Responsibilities:
- Copy/validate the **car models** from `Game/Assets/Cars-pack/` — use the **`.obj` + `.mtl`** versions (raylib loads OBJ with its MTL directly). **Ignore the `.blend` and `.fbx`** versions; they aren't needed.
- Convert **textures** to a custom optimized format (see section 3.2).
- Prepare the **UI** assets and the required fonts.
- **Incremental cooking:** processes only what has changed or is new, comparing the timestamps of the inputs (and of the cooker itself) against the output.

Each build (Debug, Development, and Release) receives, alongside the executable, an `assets` folder containing the cooked binary files that the engine reads.

> **Car model format:** the pack ships `.blend`, `.fbx`, and `.obj`. Use the **`.obj`/`.mtl`** pair — raylib reads OBJ+MTL natively, so **no FBX conversion and no `assimp` are needed**. The cooker just verifies each `.obj` has its matching `.mtl` (and any referenced texture) and copies them into the build's `assets` folder next to the cooked textures. Available models: `Cop`, `NormalCar1`, `NormalCar2`, `SportsCar`, `SportsCar2`, `SUV`, `Taxi`.

**Dependencies (Python):** `Pillow` (images) and `numpy`. No model-import library is required (OBJ is read at runtime by raylib). Create a `.venv` and install via `requirements.txt`.

## 3.2. Texture Format
Textures use a custom binary format (magic `EVTXQOI1`), compressed with [QOI](https://github.com/phoboslab/qoi/blob/master/qoi.h). In the game, they are essentially read from disk and sent straight to the GPU. Maximum resolution: 512 px.

## 3.3. In-Engine Tools (Debug/Development)
Besides the Python tools, the Debug and Development modes contain tools built **inside the engine itself in C++** using Dear ImGui — mainly the **tuning panels** used to dial in gameplay feel (see Milestone 12). These tools do not ship in Release.

---

# 4. Game Assets

**Art direction: low-poly / flat-shaded**, matching the `Cars-pack` look — few polygons, hard edges, flat solid colors, emissive-looking lights. Everything else (arena, goals, boost pads, ball) is built to sit next to those cars, so keep the whole scene in the same low-poly language.

## 4.1. Cars — `Game/Assets/Cars-pack/`
The player and bot cars use the **low-poly `.obj` models from the `Cars-pack` folder** (validated/copied by the Asset Cooker, section 3). Available: `Cop`, `NormalCar1`, `NormalCar2`, `SportsCar`, `SportsCar2`, `SUV`, `Taxi`. Guidance:
- Default to a **sporty** model for gameplay (e.g. `SportsCar` / `SportsCar2`); the others are good for variety / car selection later.
- **Team colors:** the `.mtl` colors are baked, so apply the **blue / orange** team color at runtime by tinting the model's diffuse material (raylib: set `model.materials[i].maps[MATERIAL_MAP_DIFFUSE].color`, or tint in `DrawModelEx`). Both teams can share one model, just tinted differently.
- The visual model is purely cosmetic — it's attached to the box physics body (2.5), not simulated. Scale/offset the model so it visually matches the collision box.

## 4.2. Arena, goals, boost pads, ball — procedural low-poly
Build these from raylib primitives (boxes, cylinders, planes) with flat materials, so they match the cars without extra imported assets:
- **Arena:** an enclosed low-poly stadium — floor with a center circle, side and back walls, a ceiling or high invisible bound, simple angular trim, and two colored goals. Optional low-poly filler (blocky stands, light rigs) as flat-shaded boxes.
- **Boost pads:** flat glowing pads that dim on cooldown and glow when ready.
- **Ball:** a large low-poly (faceted) sphere with a highlight.

## 4.3. UI and Fonts
Energetic, readable HUD assets and one or two fonts, prepared by the Asset Cooker. Keep the style consistent with `uistyle` (section 2.3).

---

# 5. Milestones

## Milestone 01 — Project Structure
Create a "Hello World" for the project that compiles: all external libraries cloned into `Game/ThirdParty/` and linked to the game via the Makefile. The project compiles and opens a window but doesn't do anything yet. `Tools/AssetCooker.py` runs in the build event but does nothing yet.
→ verify: `make` succeeds in all three modes; the executable opens a blank raylib window and closes cleanly.

## Milestone 02 — Base Scene and Car Movement
A playable base: the arena floor, a third-person **chase camera**, and a car (procedural box) that drives with **WASD/arrows** and steers, using Jolt for the ground contact and a single rigid body. No ball or walls yet.
→ verify: the car accelerates, reverses, and turns; the chase camera follows smoothly behind it and stays readable; the smoke-test screenshot is non-blank.

## Milestone 03 — Main Menu and Pause Menu

**Main menu (its own scene):**
- Game title (placeholder for now).
- Options: **Play**, **Settings**, and **Exit**.
- Credits in the bottom-left corner and the game build.

**Pause menu (match scene):** pressing **Esc** or **P** pauses the match and displays a dark overlay with, in order:
1. Resume
2. Settings
3. Return to main menu
4. Exit game

**Settings menu (modular):** reused in both the main menu and the pause menu. Options:
- **Graphics:** fullscreen (yes/no), resolution (default "custom" = window size), post-processing on/off.
- **Gameplay:** camera sensitivity, match duration, bot enabled/disabled (solo practice).
- **Audio:** master/SFX volume.
→ verify: Play starts a match; Esc/P pauses and resumes; settings persist within the session and are shared between both menus.

## Milestone 04 — Car Handling and Feel
Tune the arcade driving on the ground: strong acceleration, drift-friendly turning, stable recovery, and no uncontrollable flipping from minor bumps. Expose the tunables (acceleration, top speed, steering rate, grip/friction, angular damping) so they can be adjusted (see Milestone 12).
→ verify: the car feels responsive and arcade-like, not like a slow simulator; it recovers to an upright, drivable state after small collisions.

## Milestone 05 — Ball and Car–Ball Interaction
Add the **BallObject**: a large sphere, heavy but responsive, with believable bouncy physics. Tune gravity, restitution, friction, and angular damping so hits feel satisfying.
→ verify: the car pushes and pops the ball convincingly; the ball rolls, bounces, and settles believably; big hits read as impactful.

## Milestone 06 — Arena, Goals, and Scoring
Build the enclosed **ArenaObject** (side walls, back walls, ceiling/high bound) with static collision, and two colored **GoalObjects** with trigger volumes. Implement **goal detection** when the ball fully crosses a goal line, the **Match** score, the **match timer**, and the **reset with kickoff countdown** after every goal (re-center ball and cars).
→ verify: the ball bounces off all walls and the ceiling; a goal only counts when the ball is fully in; score and timer update; after a goal the field resets and a countdown plays before kickoff.

## Milestone 07 — Boost System and Boost Pads
Implement boost: a **0–100 meter**, **hold Shift** to accelerate with a flame/trail effect, boost **drains** while active, and **BoostPadObjects** around the arena that **refill** boost on overlap (then cooldown/glow).
→ verify: holding Shift consumes boost and visibly accelerates the car; boost empties and refills; driving over a ready pad refills boost and the pad goes on cooldown.

## Milestone 08 - Car selection
Create a new menu for the player can choose a different car before the match. The menu can be displayed after the player select to the start the game. Use the car assets from `Assets/Cars-Park`. Use `Cop.fbx`, `NormalCar1.fbx`, `SUV.fbx`, etc. Because we have seven assets, only use 6. The menu will have two lines and each line will have tree cars for the player to choose. About the assets, you don't need to use `SportsCars2.fbx`.

## Milestone 09 — Jumps, Flips, and Air Control
Implement the **jump** (Space) with an optional **second jump / flip**, and **air control** (pitch, yaw, roll) while airborne.
→ verify: single and double jump/flip work; the car can be rotated in the air along all three axes and lands recoverably; aerial ball touches are possible.

## Milestone 10 — Camera Modes
Polish the **ChaseCamera**: smooth follow of the car's rotation and velocity, and a **ball-cam** toggle (**C**) that keeps the ball in view. The camera must not clip badly or lose the car.
→ verify: toggling `C` switches between car-facing and ball-cam; neither mode clips through walls or loses the car; motion stays smooth.

## Milestone 11 — HUD
Implement the in-match **HUD**: team scores, match timer, boost meter, kickoff countdown, boost-pad state hints, and a **goal celebration** banner/text. Readable on common laptop screens; resizes correctly.
→ verify: every element updates live and stays legible at different window sizes; the goal banner appears on each score.

## Milestone 12 — Bot Opponent (or Solo Practice)
Add a simple **BotController**: drives toward the ball, aims its hits roughly toward the player's goal, and boosts occasionally. It must be **beatable**. If the bot is too unstable to be fun, fall back to **solo practice mode** (two goals, working scoreboard, no opponent).
→ verify: a full match can be played against the bot and won by a competent player; the bot never gets stuck against a wall for long. (Fallback: solo practice keeps score across both goals.)

## Milestone 13 — Tuning Panels (Debug/Development)
Add Dear ImGui **tuning panels** (Debug/Development only) to adjust, live: gravity, ball restitution/friction/angular damping, car acceleration/top speed/steering/grip, jump and flip impulses, air-control strength, boost drain/refill rates, and camera smoothing. Include a button to save the current values back to a config the game loads on startup.
→ verify: changing a slider immediately changes gameplay feel; saved values persist across restarts; Release builds contain no tuning UI.

At any moment during gameplay, the developer can press a button to open/hide the Level Editor (**F1**). When the editor is opened, the game is paused. 

## Milestone 14 — Visual Polish and Effects
Bring the arena to life: futuristic indoor stadium, arena trim, colored goals, boost-pad glow, clear team colors, a boost flame/trail, a ball highlight, goal-explosion particles, lighting, and shadows. Add **bloom / subtle post-processing** only if performance allows (toggleable).
→ verify: the game reads as an energetic arcade sports game; effects fire on the right events (boost, jump, ball hit, goal); frame rate stays smooth on a common laptop.

## Milestone 15 — Audio
Implement a simple, efficient **procedural sound** system for at least: boost (loop while active), jump/flip, car–ball impact, wall/car impact, boost-pad pickup, kickoff countdown ticks, goal celebration, match end, and UI click/hover.
→ verify: each cue fires on its event, respects the audio volume settings, and doesn't stutter under normal play.

## Milestone 16- Arena field dimensions
Improve the current arena field dimensions. I want a arena simulation from rocket league. That measures for reference creation. There is two type of measures: real life measure using meters and engine units (Unreal Engine units)

**Field Dimensions**
- Goal line to goal line: 10,248 Unreal Units, equivalent to approximately **102.41 meters**
- Sideline to sideline: 7,680 Unreal Units, equivalent to approximately **76.81 meters**
- Floor to ceiling: 2,048 Unreal Units, equivalent to approximately **20.73 meters**

## Milestone 17 — Title Screen ("Press Any Button")
The **first scene shown at launch**, before the main menu. A lightweight title card
over a **live 3D view of the arena** — this is a title screen, not a marketing landing
page. Reuses the existing arena, ball, lighting, and `uistyle` (section 2.3); imports
no new assets except the game logo. For game logo, use `Game/Assets/Icon/budget-league-logo.png`. 
And for references to creation/title screen rocket-league like use `img/press-key-menu.png`

**Boot flow:** `TitleScene` → `MainMenuScene` → `MatchScene`. Pressing any key, mouse
button, or gamepad button leaves the title and goes to the main menu.

**Background — live 3D (not a static image):**
- Render the real arena (reuse the match arena + lighting) from a low, cinematic fixed
  camera, with the **ball** (reuse `BallObject`) resting on the field in the foreground.
- Subtle idle only: a very slow camera drift or a gentle ball spin — enough to read as
  "alive", no gameplay.
- Draw order: 3D scene inside BeginMode3D/EndMode3D first, then 2D overlay on top.

**Overlay (2D, using `uistyle`):**
- The **game logo**, centered (upper-middle). Cooked like the other UI textures
  (section 3.2). For game logo use 
- **"PRESS ANY BUTTON TO START"** below the logo, in the UI accent color, **pulsing**
  (sine-based alpha/scale) so it draws the eye.
- Studio credit + build string centered at the bottom (same info as the base menu).

**Input:** any key (`GetKeyPressed()`), any mouse button, or any gamepad button
(`GetGamepadButtonPressed()`) transitions to `MainMenuScene`. Optional short fade.

→ verify: launch opens the title over the live low-poly arena with the ball visible;
  the prompt text pulses; pressing any key/mouse/gamepad button goes to the main menu;
  no gameplay happens on the title; layout stays centered and readable when the window
  resizes.

## Milestone 18 — Main Menu Showcase (Rocket League-style)
Expands the main-menu portion of the Main Menu milestone. The first screen after
launch is a **car showcase inside the arena**, with the menu overlaid — **reusing the
existing MainMenuScene, the `uistyle` UI (section 2.3), the arena, and the Cars-pack
(section 4)**. Do not build new UI or import new assets for this: only wire the
pieces already in the game (raylib) into the showcase layout.Use `img/menu-rocket-league.png` 
as reference. **Do not create the weekly challenges menu from the original image**

**Background — live 3D showcase (not a static image):**
- Render the real arena behind the menu (reuse the arena + lighting already built for
  the match), with a single car parked on the field, shown from a 3/4 front angle.
- The car is a **random model** picked from the Cars-pack (`Cop`, `NormalCar1`,
  `NormalCar2`, `SportsCar`, `SportsCar2`, `SUV`, `Taxi`), tinted to a random team
  color (blue/orange) via the diffuse material. Re-roll the pick every time the menu
  is (re)entered.
- Slow **turntable** rotation (a few degrees/sec) so it reads as a showcase. Fixed
  showcase camera; subtle idle only.
- Draw order: 3D scene first (arena + car inside BeginMode3D/EndMode3D), then the 2D
  menu on top.

**Menu (left side) — drawn with the existing `uistyle`:**
- Vertical list using the same panel / accent / text colors as the rest of the UI:
  **Play**, **Settings**, **Exit**. Local game — no online/shop/pass/garage/profile
  entries.
- The selected item is highlighted (accent bar/glow), like the reference. Works with
  keyboard (up/down/enter) and mouse (hover/click).
- Credits + game build in the bottom-left corner (as in the base main menu).

**Explicitly removed:** the reference's **Weekly Challenges** panel is **not**
included — no right-side challenges/quests panel at all.

→ verify: launch goes straight to the showcase (no landing page); a random Cars-pack
  car appears tinted and slowly turntabling in the arena; the left menu uses the
  existing `uistyle` with a clear selection highlight and works via keyboard and mouse;
  Play starts a match, Settings opens the shared settings menu, Exit quits; there is no
  Weekly Challenges panel; re-entering the menu re-rolls the car; layout resizes
  correctly on common laptop screens.

## Milestone 19 - Settings Screen — layout tweak (main-menu context)
When Settings is opened from the main menu, **remove the game-title header**
("BUDGET LEAGUE") — the Settings panel is the only focal element. **Center the panel
on screen** (horizontally and vertically). Keep the **live 3D showcase** (arena +
turntabling car from the main menu) rendering **behind** the panel: the settings panel
draws as an overlay on top of that scene, not over a flat dark background. The panel
keeps the existing `uistyle` (border, section labels, Back button highlight).

Scope: this is the **main-menu** Settings only. When the same modular SettingsMenu is
opened from the **pause menu** during a match, it keeps drawing over the paused match
as before (no showcase) — the module just takes a "background mode" flag.

→ verify: opening Settings from the main menu shows no game title, a centered panel,
  and the arena+car still visible behind it; pause-menu Settings is unchanged; the
  panel stays centered on window resize.

---

# 6. Final Milestone — Polish (individual prompts)

A set of one-off polish and fix tasks. Mark items **OK** as they are completed.

## 6.1. Handling and Feel
- Fine-tune the car so minor bumps never flip it uncontrollably, while intentional flips still feel snappy.
- Tune the ball so it is heavy but never sluggish; big hits should feel impactful.
- Smooth the kickoff reset so cars and ball settle cleanly with no jitter.

## 6.2. Camera
- Prevent chase-camera clipping near walls (pull in / occlusion handling).
- Tune ball-cam framing so the ball and the goal stay readable at speed.

## 6.3. Feedback and Juice
- Screen/particle punch on goals and big ball hits (keep it tasteful).
- Boost flame/trail intensity scales with boost use.
- Boost-pad glow clearly communicates ready vs cooldown.

## 6.4. Bot
- Keep the bot from getting stuck against walls or over-committing; make sure it stays beatable but not trivial.

## 6.5. Performance and Stability
- Keep a smooth frame rate on a common laptop; make post-processing degrade gracefully.
- No major console/log errors during a full match.

---

# 7. Verification

Because this is a native build (not a web/dev-server game), verify by **running the executable** and, where possible, the headless **smoke-test mode** (App renders a frame and writes a screenshot).

Confirm:
- The scene renders (non-blank screenshot from the smoke test).
- The car moves, turns, jumps, and boosts.
- The ball collides with the car and the arena.
- Goals are detected correctly (only when the ball is fully in).
- Score and timer update.
- Reset + kickoff countdown after a goal works.
- Boost pads refill boost.
- The camera does not clip badly or lose the car.
- No major errors in the log.

**Deliverable:** a locally runnable native game (path to the built executable and how to run it), a short summary of the controls, and a note on what was implemented vs deferred.

---

# 8. Controls (reference)
- **WASD / Arrow keys:** drive / steer.
- **Space:** jump (double-tap / hold-and-tilt for flip).
- **Shift:** boost.
- **R:** reset car.
- **C:** toggle camera mode (chase / ball-cam).
- **Esc or P:** pause.
- Air control (while airborne): **WASD** maps to pitch/yaw/roll.
