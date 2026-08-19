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
- [x] **Milestone 11 — HUD**
- [x] **Milestone 12 — Bot Opponent (or Solo Practice)**
- [x] **Milestone 13 — Tuning Panels (Debug/Development)**
- [x] **Milestone 14 — Visual Polish and Effects**
- [x] **Milestone 15 — Audio**
- [x] **Milestone 16 — Arena Field Dimensions**
- [x] **Milestone 17 — Title Screen ("Press Any Button")**
- [x] **Milestone 18 — Main Menu Showcase**
- [x] **Milestone 19 — Settings Screen (main-menu layout)**
- [x] **Milestone 20 — Soundtrack (OST playlist)**
- [x] **Milestone 21 — Cross-Platform Builds** (Windows done and verified by cross-compile; macOS written but unbuilt — no Mac here)
- [x] **Milestone 22 — Gamepad Support** (built against the 22.1 layout table; **no controller exists in this environment**, so the pad's own buttons are unverified on hardware — see the section below for exactly what was and was not proven)

Final milestone (CLAUDE.md section 6), a subsection at a time:

- [x] **6.1 — Handling and Feel**
- [x] **6.2 — Camera**
- [x] **6.3 — Feedback and Juice**
- [x] **6.4 — Bot**
- [x] **6.5 — Performance and Stability**

Every milestone in CLAUDE.md is now done, and the section 6 polish list is under way: **the whole of section 6 is finished** -
6.1 Handling and Feel, 6.2 Camera, 6.3 Feedback and Juice, 6.4 Bot and 6.5 Performance and
Stability, each with its own section below.

Two pieces of work outside the milestone list are also done: the **asset pipeline** (FBX cooking plus
the car models, CLAUDE.md 4.1) and the **arena ramps with wall and ceiling driving** (PROMPTS.md).

Milestone 08 was inserted into CLAUDE.md after 09 was already built, which is why it was finished out
of order. Everything from the old Milestone 08 onwards shifted up by one; this document uses the new
numbering throughout. **CLAUDE.md was also rewritten on 2026-08-04:** the old Milestone 17 (car and
ball dimensions) was dropped and replaced by Milestone 17 — Title Screen and Milestone 18 — Main Menu
Showcase, which is where the list now ends.

The game opens on a title screen: the cooked logo over a live view of the real arena, with the ball
resting on the pitch beside the far goal and a pulsing "press any button to start". Any key, mouse
button or gamepad button goes to the main menu, which is a showcase of the same kind: the arena
behind it, one of the seven cooked cars parked on the pitch in a random team colour turning slowly,
and the list (Play / How to play / Settings / Exit) down the left. How to play is its own screen
of rules and controls. Play opens the car picker — six of the
seven cooked cars on a 2 x 3 grid of pedestals, spinning slowly in the team colour — and starting
from there opens a real match: an enclosed
76.81 x 102.41 m arena with a 20.73 m ceiling, side walls, back walls and two coloured goals; a player car driven
with WASD/arrows on a Jolt rigid body; a ball; a score, a match clock and a kickoff countdown; and
a smooth third-person chase camera; and an orange bot opponent that chases the ball and shoots at
the player's goal, or solo practice when it is switched off in the settings. A goal resets the field
and counts down again, and the match ends on a full-time screen offering a rematch. Every readout now lives in `HUD.h/cpp`: a scoreboard
with both team scores and the clock, a boost meter and a speed readout, the kickoff countdown and a
goal band in the scoring team's colour. The car draws a real low-poly model from the Cars-Park pack, cooked from FBX and
painted in the team colour, and the whole scene is flat-shaded by one directional light. Shift
boosts, drawing on a 0-100 meter refilled by 18 boost pads spread across the field. Space jumps,
a second press flips in whatever direction is held, and the car can be pitched, yawed and rolled in
the air, so aerial ball touches work. Esc or P pauses, with Resume / Settings / Return to main menu
/ Exit. In Debug and Development, **F1** opens a Dear ImGui tuning panel over the match.
The game is also audible now: ten cues are synthesised at startup from one table in
`AudioSystem.cpp` — no sound files exist — and the boost is a running audio stream rather than a
repeated sound. Master and SFX volumes are read from the settings every frame.

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
| Nearest pad edge to the kickoff spot | 2.00 m, so kickoff never grabs one (2.40 m since the pads were trimmed — see the size row in the Milestone 16 table) |

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

Milestone 11 HUD, verified from screenshots taken by a temporary probe that forced the match into
each state and resized the window from the environment (removed afterwards):

| Check | Result |
|---|---|
| Scoreboard | both scores in their team colours either side of the clock, 3 - 1 read correctly |
| Clock | counts down, and turns `uistyle::Warning` red for the last 30 s — checked at 0:12 |
| Boost meter | fills and empties, quarter ticks visible, number matches the bar at 18 / 25 / 33 / 60 / 100 |
| Speed | reads in km/h bottom left, replacing the old debug line |
| Kickoff countdown | KICKOFF plus the digit, shadowed so it stays readable over the far goal |
| Goal banner | full-width band in the scoring team's colour, GOAL! and BLUE SCORES, wipes open and shut |
| Full time | FULL TIME, 3 - 1 in team colours, BLUE WINS, REMATCH and MAIN MENU buttons |
| 1280 x 720 / 1920 x 1080 / 960 x 540 | every element scales and stays inside the window |
| Release build | no GROUNDED / CHASE CAM readouts — they are behind `GAME_DEV_TOOLS` |
| Debug / Development / Release | build with no game-code warnings, smoke test exits 0, no errors in the log |

Milestone 12 bot, measured by a temporary probe that stepped the whole match loop directly - the
match state machine and physics, no rendering - with the player car left with no controller at all,
so every number below is the bot playing against a car that never moves (removed afterwards):

| Measurement | Result |
|---|---|
| Full 5 minute match | **bot 6, player 1** - the one against it was its own goal, so it is fallible |
| Same match, 2 minutes | bot 3, player 0 |
| Distance driven in 5 minutes | 3417 m, so it is genuinely working the whole field |
| Time under 2 m/s | 31% - approaches, turns and the kickoff freeze |
| Longest single stretch under 2 m/s | 5.12 s, which is the jam reset firing |
| Jam resets in 5 minutes | 2 |
| Ever inverted (uprightness < 0.3) | 1% of the match |
| Started nose-first into a wall | free and driving in 0.10 s, scores inside 30 s, no resets |
| Ever left the flat floor / entered a goal recess | no, the target is clamped to the flat floor |
| Bot switched off in settings | no bot car is built at all; the scene is exactly the solo practice it was |
| Debug / Development / Release | build with no game-code warnings, smoke test exits 0, no errors in the log |

Milestone 13 tuning panel, verified by a temporary probe that wrote a config, ran a match and read
the values back out of Jolt rather than out of the C++ fields (removed afterwards), plus screenshots
of the panel:

| Check | Result |
|---|---|
| Panel renders | title bar, scroll bar, six collapsing sections, 24 sliders, Save button |
| Labels | full at the default window size, including `highSpeedSteerScale` |
| Save | writes `Tuning.cfg` next to the executable, one `Section.name = value` line per slider |
| Load | a match started after a save comes up with the saved values on the objects |
| Load reaches Jolt, not just the fields | gravity -25.00, ball restitution 0.20, ball friction 0.90 read back from the body interface |
| Car entries move the bot too | `maxSpeed` 21.5 on the player's car and 21.5 on the bot's |
| Pad refills split by kind | small pads 40, full pads 60, from two sliders |
| No config file | defaults, no warning, exit 0 |
| Release build | `Tuning (F1)` appears 0 times in the binary; Development, 1 |
| Debug / Development / Release | build with no game-code warnings, smoke test exits 0, no errors in the log |

Milestone 14 effects, verified from screenshots of each effect forced into view by a temporary probe
(removed afterwards) plus a frame-cost measurement with the frame rate cap lifted:

| Check | Result |
|---|---|
| Contact shadows | under both cars and the ball, softening and widening with height |
| Boost flame | flickering cone at the exhaust, following the car through its roll and pitch |
| Boost trail | embers behind a boosting car, which is what actually reads from a chase camera |
| Goal burst | 90 team-coloured plus 40 white cubes thrown out of the net |
| Goal screen flash | team colour over the whole screen, gone in a third of a second |
| Ball appearance | wireframe polygon overlay removed; only the lit faceted shading and contact shadow remain |
| Goal space | this row described the enclosure being left unrendered; that was undone on 2026-08-04 and the goal draws its own pieces again — see the goal frame note below |
| Big-hit burst | fires on a jump in the ball's speed over 7 m/s, sized by how big the jump was |
| Jump puff | at the wheels, on both the first and the second jump |
| Stadium | three tiers of stands and ten light rigs, visible through the glass, none of it solid |
| Bloom on | boost pads, goal frame, ball and countdown all glow; the HUD is drawn after and is untouched |
| Bloom frame cost | **0.51 ms with, 0.41 ms without** - about 0.1 ms, measured over 600 uncapped frames |
| Bloom off in Settings | nothing is allocated and the scene draws straight to the screen |
| `assets/Shaders/Bright.fs` missing | logs `POSTFX: bloom shaders unavailable`, renders unbloomed, exit 0 |
| Debug / Development / Release | build with no game-code warnings, smoke test exits 0, no errors in the log |

Milestone 15 audio, implemented on `feat/audio` on 2026-08-04. Two temporary probes, both removed
afterwards: one exported every cue as a `.wav` at startup and measured it in Python, the other logged
each cue as it fired while a scripted input harness played the game.

The cues themselves, measured from the exported waves. The zero-crossing rate is what confirms the
synthesis arithmetic: on the cues with no chord and no noise it lands on the table's own frequency.

| Cue | Length | Peak | First / last sample | Clipped samples | Measured tone |
|---|---|---|---|---|---|
| UiHover | 0.060 s | 0.14 | 0.0000 / 0.0000 | 0 | 917 Hz against 900 in the table |
| UiClick | 0.100 s | 0.23 | 0.0000 / 0.0000 | 0 | falls, 1500 → 700 |
| Jump | 0.200 s | 0.34 | 0.0000 / 0.0001 | 0 | rises, 280 → 760 |
| BallHit | 0.300 s | 0.69 | 0.0000 / 0.0000 | 0 | falls hard, 460 → 85 |
| Impact | 0.220 s | 0.54 | 0.0000 / 0.0000 | 0 | 6718 crossings/s — mostly noise |
| BoostPad | 0.300 s | 0.29 | 0.0000 / 0.0001 | 0 | rises, 620 → 1240 plus a fifth |
| CountdownTick | 0.120 s | 0.37 | 0.0000 / 0.0001 | 0 | 717 Hz against 720 in the table |
| CountdownGo | 0.360 s | 0.43 | 0.0000 / 0.0000 | 0 | 980 plus a fifth |
| Goal | 0.900 s | 0.50 | 0.0000 / 0.0001 | 0 | rises, 440 → 880 plus a fifth |
| MatchEnd | 1.200 s | 0.46 | 0.0000 / 0.0000 | 0 | 517 → 183 against 520 → 190 |

Every cue starts and ends at zero and none clips, which is what says none of them can pop. BallHit is
the loudest at 0.69 and UiHover the quietest at 0.14, so the mix has a deliberate order to it.

Then the wiring, from one scripted run in the match (times are seconds from launch):

| Event | Fired |
|---|---|
| Kickoff countdown | ticks at 0.10, 1.08, 2.08 — one per whole second, never twice |
| Field going live | 3.08, exactly when the countdown ends |
| Car hitting the ball | 4.27, pitched to 0.85 by the size of the hit |
| Boost held | on at 5.02, off at 7.02 — the scripted Shift window to the frame |
| Wall / car impact | 5.03 at volume 0.83, and 13.15 at 1.00 |
| Goal | 6.10 and 14.62, on entering the celebration |
| Jump | 8.22 |
| Boost pad | 12.70, from the tank going up rather than from a new event on the pad |
| Second kickoff | ticks at 8.60, 9.60, 10.60, live again at 11.60 |
| Full time | 21.08 |
| Boost during a kickoff freeze | never — the car is not stepped, so `boosting` stays false |
| False pad pickup at match start | none, once `previousBoostAmount` was seeded (it fired before) |

And the menus, from a second scripted run that walked the main menu and the settings panel:

| Event | Fired |
|---|---|
| Keyboard moving the selection | one hover per press, 0 → 1 and 1 → 2 |
| Mouse moving across rows | one hover per row crossed, never per frame |
| Activating a row | click at 1.40 |
| Changing a value with left/right | click at 1.80 |

| Check | Result |
|---|---|
| Debug / Development / Release | build with no game-code warnings, smoke test exits 0, screenshot non-blank |
| Log during a run | `AUDIO: 10 procedural cues ready`, no warnings and no errors in any config |

Milestone 16 arena dimensions, implemented on `feat/arena` on 2026-08-04:

| Check | Result |
|---|---|
| Sideline to sideline | 76.81 m along X |
| Goal line to goal line | 102.41 m along Z |
| Floor to ceiling | 20.73 m |
| Flat floor inside the 5 m ramps | 66.81 x 92.41 m |
| Goals, kickoff spawns, bot limits and camera ceiling | continue to derive from `ArenaObject` |
| Boost pads | all 18 preserve their old proportional layout across the enlarged flat floor |
| Boost pad size | small pads use a 1.4 m radius; full pads use 2.2 m — trimmed on 2026-08-04, down from 1.8 / 2.7. `radius` is the drawn size *and* the XZ pickup radius; there is no separate visual field, so shrinking a pad also makes it slightly harder to clip |
| Ground appearance | deterministic 6 m green triangle mosaic creates a procedural low-poly grass effect |
| Soccer markings | centre circle, halfway line and pitch boundary render at 4 px; decorative grid remains thin |
| Wall appearance | metallic gray transparent panels use staggered connected diamond cells matching the low-poly metal-grid reference |
| Field line colour | centre circle, halfway line and boundary are white; decorative grid is faint white |
| Upper cage | ceiling ramps share the silver-gray wall shade; the invisible ceiling collision is represented by an open diamond lattice |
| Wall-grid transitions | rounded corners space visual nodes by arc length (~7 m), independent of their 10 collision facets, so cell density stays consistent |
| Ramp seam lines | continuous white bands mark floor-ramp/wall, wall/ceiling-ramp and ceiling-ramp/ceiling joins, including rounded corners |
| Goal-end metal grid | orange segments are generated from integer-indexed nodes and mirrored exactly onto blue, with the same continuous lower boundary and no vertical patch rails |
| Floor-to-wall ramps | neutral gray with light transparency and a vertex-color gradient that brightens toward the wall |
| Debug / Development / Release | all build successfully |

Goal visibility, fixed on `feat/arena` on 2026-08-04. The report was that a car or the ball
disappeared into a black slab on entering either net. **The slab was not the goal at all — it was the
stadium stands, standing inside the recess.** Two attempts before this one went to `GoalObject` and
made its enclosure invisible, which could never have worked: the enclosure was already invisible, and
hiding it only exposed the stands behind it more. The fix is entirely in `ArenaObject::AddStadium`.

| Check | Before | After |
|---|---|---|
| End stand tier 0 | z 52.80 – 57.60, y 0 – 4.0, **inside** the 4 m recess (51.21 – 55.21) | z 56.20 – 61.00, clear |
| End stand tier 1 | z 57.80 – 62.60, y 0 – 8.5, **inside** the recess | z 61.20 – 66.00, clear |
| End stand tier 2 | z 62.80 – 67.61, clear | z 66.20 – 71.00, clear |
| Colour reaching the screen | `{30,38,58} x (0.34, 0.365, 0.43)` = `(10,14,25)`, near black | nothing opaque left in the recess |
| Car and ball 2.5 m into the net | car occluded past ~1.6 m | both fully visible, screenshotted |
| Stand ring at the four corners | continuous | continuous — side banks were run out to meet the ends |
| Physics / camera | — | unchanged: the stands are `solid = false`, so they are in neither the compound shape nor the camera's occlusion ray |
| Debug / Development / Release | — | build with no game-code warnings, smoke test exits 0, screenshot non-blank, no errors in the log |

Verified with a temporary probe that parked the car and the ball inside a net and aimed the camera at
the mouth (removed afterwards). **The environment can run the game now**: `DISPLAY=:1` works, so the
smoke test is no longer blocked the way the Milestone 16 note said. There is still no Xvfb, and still
no `xdotool`/`wtype` for key presses.

**Reverted on request, the same day:** a team-coloured opaque net backdrop, solid goalposts and a
crossbar and a `netTeamMix` colour control were all built on top of this fix and then taken back out.
Only the stands fix was kept from that round.

**Goal frame restored from history, same day.** The goal was then asked to match a reference image
(`img/color-intensity.png`), and the answer was already in the repository: `GoalObject` rendered its
own pieces until commit `3f0dd3f`, which replaced that with a bare wire outline. `git checkout
ba71bd4 -- GoalObject.h GoalObject.cpp` restores it exactly, and the diff between that commit and
`3f0dd3f` is *only* the removal, so nothing newer was lost by going back.

| Surface | Reference | Restored render | Delta |
|---|---|---|---|
| Interior back wall | `(41,23,12)` | `(44,25,9)` | 8 |
| Front band, unlit face | `(92,50,23)` | `(87,52,21)` predicted, measured on the +Z goal | ~5 |
| Front band, sunlit face | — | `(132,76,28)`, matches the shader to the digit | 0 |

- **The frame is `teamColor`; the floor and back of the net are `teamColor / 3`.** That one-third is
  the whole scheme: a bright shell around a dark interior, which is exactly what the reference shows.
- **It is drawn through a lit `boxModel` with `DrawModelEx`, not `DrawCubeV`.** That is what gives
  the bright top face and the darker front band - flat immediate-mode cubes render one uniform tone
  and read as a coloured card rather than a solid object. It also means `~GoalObject` must call
  `lighting::Detach` before `UnloadModel`, or the two goals destroy the one shared lit shader; the
  restored destructor does, and teardown was checked (three shader programs unloaded: lit, Bright,
  Blur).
- **The two goals are legitimately different brightnesses, and it is the sun, not a bug.** The sun
  points `(0.30, 0.90, 0.32)`, so the -Z goal's front face takes `key = 0.317` and renders
  `(132,76,28)`, while the +Z goal's front face faces away, takes `key = 0` and renders `(87,52,21)`.
  The reference image is of the unlit-face end. Both were verified against the shader arithmetic
  rather than by eye.

Milestone 17 title screen, implemented on `feat/new-menu` on 2026-08-04. It brought the texture half
of the asset pipeline with it: nothing had ever been cooked as a texture before, so `.evtex`,
`TextureAsset` and the cooker's PNG step are all new here.

The format first. A temporary probe decoded every cooked `.evtex` back in Python and compared it
against the source PNG (removed afterwards):

| Texture | Cooked size | Payload | Of raw RGBA | Lossless | Cook time |
|---|---|---|---|---|---|
| budget-league-logo | 512 x 512 | 222 KB | 22% | yes, byte for byte | 0.11 s |
| metal-grid-wall | 512 x 512 | 210 KB | 21% | yes | 0.11 s |
| stylized-grass-lowpoly | 512 x 512 | 361 KB | 35% | yes | 0.14 s |
| stylized-stone | 512 x 512 | 334 KB | 33% | yes | 0.13 s |

Then the screen itself, from screenshots and a temporary probe that logged the ball and the camera
every 100 frames (also removed):

| Check | Result |
|---|---|
| Boot flow | launch opens `TitleScene`; the smoke test still goes straight to the match |
| The mark | the cooked badge on the left and BUDGET over LEAGUE on its right, centred as one block, as on the reference |
| Prompt | pulses between 0.65 and 1.0 alpha, clear of both the ball and the goal |
| Live 3D | the real arena, both nets and the ball, lit and bloomed exactly as the match is |
| Composition | orange goal left of centre, ball resting bottom right with its contact shadow |
| Idle | the camera yaws 2.5 degrees either side over 24 s; measured drifting 10.2 → 8.5 → 11.9 |
| No gameplay | ball at (16.0000, 1.2500, -40.0000) for 13.35 s, unchanged to four decimals |
| Fade in | the screen is dark at 0.2 s and clear by 0.6 s |
| Any button | forcing a press at 1.0 s lands in the main menu, drawn correctly |
| 1280x720 / 1920x1080 / 960x540 | logo, prompt and credits stay centred and inside the window |
| Missing `assets/Textures` | logs `TEXTURE: could not read ...`, draws the name alone centred, keeps running |
| Debug / Development / Release | build with no game-code warnings, smoke test exits 0, no warnings or errors in the log |

Milestone 18 main menu showcase, implemented on `feat/new-menu` on 2026-08-04, verified from
screenshots and a temporary probe that logged each pick and built the scene twice in one process
(both removed afterwards):

| Check | Result |
|---|---|
| Live 3D behind the menu | the real arena and both nets, with one cooked car parked at the centre spot |
| Angle | 3/4 front, camera low at 1.9 m and off to the left, so the car fills the right of the frame |
| Turntable | 12 degrees a second, the only thing in the scene that moves |
| Random pick | `Cop` blue then `SUV` orange from two entries in one process; `SportsCar2`, `Taxi`, `NormalCar1` and `NormalCar2` all seen across runs |
| Re-roll on re-entry | yes - the pick is in `Initialize`, and entering the menu is what runs it |
| Menu | Play / How to play / Settings / Exit down the left, keyboard and mouse, accent bar on the selected row |
| Weekly Challenges panel | not built, as CLAUDE.md requires |
| Settings | opens centred over the showcase, closes back to the list |
| Credits and build string | bottom left, as in the base menu |
| 1280x720 / 1920x1080 / 960x540 | the column, the title and the credits all hold their place |
| Missing `assets/Models` | falls back to the box stand-in in the team colour, no crash |
| Teardown across two menu scenes | Debug: no Jolt assert, exactly three shader programs unloaded at exit (lit, Bright, Blur) |
| Debug / Development / Release | build with no game-code warnings, smoke test exits 0, no warnings or errors in the log |

Milestone 19 settings screen, implemented on `feat/new-menu` on 2026-08-04, verified from screenshots
of both contexts (temporary probes that forced each one open, removed afterwards):

| Check | From the main menu | From the pause menu |
|---|---|---|
| Game title above the panel | gone | never had one |
| Panel position | centred on both axes | unchanged, 105 units from the top |
| Behind the panel | the showcase, arena and car still visible | the paused match under the pause overlay |
| Dimming | the module lays down 0.35 of its own | none of its own; the scene already dims 0.65 |
| 1280x720 / 1920x1080 / 960x540 | centred at all three | unchanged |
| Rows, sections and Back | the same widget, so identical in both | identical |
| Debug / Development / Release | build with no game-code warnings, smoke test exits 0, no warnings or errors in the log | |

Milestone 20 soundtrack, implemented on `feat/ost` on 2026-08-04. It is the first thing in the game
that streams audio from disk; every sound effect is still generated at startup. Verified with a
temporary probe that seeked each track to a second from its end, so a whole playlist ran in a minute
(removed afterwards):

| Check | Result |
|---|---|
| Tracks cooked | all 6, copied into `assets/Music` in every configuration, 33 MB |
| Playback | one track streams at launch and the playlist runs unattended |
| Advance on end | **50 advances in 60 s**, every one of them at the end of a track |
| Same track twice in a row | **0 of 50** |
| Spread over the 6 tracks | 6, 8, 8, 9, 9, 10 - even, and all six were reached |
| Music volume slider | `settings.musicVolume` reaches the stream live: measured 0.60 → 0.25 → 1.00 while a track played |
| Settings panel | the new row sits under SFX volume and the panel grows with it; Back stays inside |
| Missing `assets/Music` | logs the directory warning once, reports `0 music track(s)`, game runs silent and exits 0 |
| No audio device | the playlist is never even scanned - it is behind the same `ready` gate as the cues |
| Debug / Development / Release | build with no game-code warnings, smoke test exits 0, no errors in the log |

Final Milestone 6.1 handling and feel, implemented on `fix/final-milestone` on 2026-08-06. Measured
with a temporary harness that stepped the match loop directly - no rendering - and was removed
afterwards. The README's polish list had all three items ticked already; measuring them showed two of
the three were not actually true, which is the whole reason this pass exists.

**Bumps were already right and nothing was changed for them.** A bump is a one-off angular kick, which
is what a wheel-less box actually takes out of a glancing contact - the arena is smooth by design, so
there is no lip to drive over. The car was kicked about its own forward axis at 25 m/s:

| Roll kick | Worst lean | Back to level | Speed kept |
|---|---|---|---|
| 2 rad/s | 0.0 deg | 0.12 s | 32.0 m/s |
| 4 rad/s | 4.4 deg | 0.12 s | 32.0 m/s |
| 6 rad/s | 6.8 deg | 0.12 s | 32.0 m/s |
| 8 rad/s | 8.9 deg | 0.12 s | 32.0 m/s |
| 12 rad/s | 12.6 deg | 0.12 s | 32.0 m/s |

The offset centre of mass and `tumbleDamping` between them mean even the hardest kick never gets near
tipping the car, and none of them costs it any speed at all.

**Flips did not work, and that is what was fixed.** Left to air control and the body's angular damping,
a flip's spin decays at about 3 rad/s combined, so it stalled long before the rotation finished:

| Flip | Half turn | Full turn | Lands upright |
|---|---|---|---|
| Forward, before | never | never | 2.54 s |
| Forward, after | 0.37 s | 0.70 s | 2.23 s |
| Backward, before | never | never | 1.25 s |
| Backward, after | 0.38 s | 0.70 s | 2.04 s |
| Side, before | 0.76 s | never | 0.99 s |
| Side, after | 0.39 s | 0.70 s | 0.93 s |

Traced through a forward flip after the fix, sampled every 0.1 s: the car sweeps the whole turn under
the held spin, comes out at uprightness **+0.987 at 0.71 s**, holds exactly that for the entire 1.6 s
descent from 6.6 m, and lands flat at 2.31 s.

**The ball was left heavy but stopped being sluggish.** Only the rolling resistance moved; every hit
number is untouched, which was the point.

| Measurement | Before | After |
|---|---|---|
| Roll from 15 m/s | 25.4 m in 7.0 s | 31.6 m in 8.9 s |
| Roll from 20 m/s | 35.6 m in 7.7 s | 43.7 m in 9.8 s |
| Roll from 30 m/s | 58.2 m in 8.7 s | 69.7 m in 11.0 s |
| Hit at 16.4 m/s | 19.6 m/s (1.19x), 13.6 m | unchanged |
| Hit at 25.9 m/s | 29.8 m/s (1.15x), 34.2 m | unchanged |
| Hit at 32.0 m/s | 37.8 m/s (1.18x), 50.3 m | unchanged |
| Hit at 46.2 m/s, boosted | 52.4 m/s (1.13x), 80.2 m | unchanged |
| Bounce apexes from 12 m | 5.61 / 3.16 / 2.11 / 1.66 m | unchanged |
| Resting height | exactly `radius` (1.2500 m) | unchanged |

**The kickoff reset now moves nothing at all.**

| Check | Before | After |
|---|---|---|
| Car jumps returned by the reset | **no** - `jumpUsed` and `doubleJumpUsed` both stayed set | yes, both cleared |
| Car drift over 2 s of idle play | 0.0100 m | **0.0000 m** |
| Car peak speed over the same | 0.3839 m/s | **0.0000 m/s** |
| Car resting height vs spawn | 0.3500 against a 0.3600 spawn | 0.3500 against a 0.3500 spawn |
| Ball drift and peak speed | 0.0000 m, 0.0000 m/s | unchanged |
| Car tilt | 0.00000 | unchanged |

| Check | Result |
|---|---|
| Debug / Development / Release | build with no game-code warnings, smoke test exits 0, screenshot non-blank, no errors in the log |
| `flipDuration` on the F1 panel | yes, in `Car air` beside `flipImpulse` and `flipSpin`, mirrored onto the bot |

Final Milestone 6.2 camera, implemented on `fix/final-milestone` on 2026-08-06. Measured with a
temporary harness that ran the **real** `ChaseCamera` over scripted driving and then judged the
result by asking Jolt and the projection directly — never by asking the camera about itself
(removed afterwards). Every routine reports: whether the eye was inside static geometry
(`CollidePoint`), whether anything solid sat within a 0.25 m bubble of it (`CollideShape`), whether
arena geometry stood between the eye and the car, whether the car / ball / attacked goal projected
inside the window, the biggest camera move in a frame and the biggest change in that.

Milestone 10 had already verified the camera at a set of *static* placements, which is why the
defects below survived: they only appear while something is moving.

**Clipping, before and after.** Zeros in every column are the after; the numbers are the before.

| Routine | inSolid | nearClip | car hidden | car off screen | worst jump | worst jerk | min clearance |
|---|---|---|---|---|---|---|---|
| open field, straight | 0 → 0 | 0 → 0 | 0 → 0 | 0 → 0 | 0.529 → 0.529 | 0.030 → 0.030 | 3.75 → 3.75 |
| open field, hard turn | 0 → 0 | 0 → 0 | 0 → 0 | 0 → 0 | 0.529 → 0.529 | 0.095 → 0.095 | 3.70 → 3.70 |
| into the +X side wall | **33 → 0** | 73 → 0 | 33 → 0 | 58 → 0 | 1.406 → 0.735 | 0.984 → 0.574 | 0.00 → 3.45 |
| into the -X side wall | **61 → 0** | 76 → 0 | 61 → 0 | 61 → 0 | 1.348 → 0.723 | 0.777 → 0.540 | 0.00 → 0.79 |
| into the +Z back wall | **26 → 0** | 58 → 0 | 26 → 0 | 41 → 0 | 1.312 → 0.748 | 1.036 → 0.598 | 0.00 → 1.32 |
| into the -Z back wall | **41 → 0** | 58 → 0 | 41 → 0 | 0 → 0 | 1.320 → 0.738 | 0.891 → 0.637 | 0.17 → 0.92 |
| boosted up the +X wall | 0 → 0 | 0 → 0 | 0 → 0 | 0 → 0 | 0.746 → 0.748 | 0.074 → 0.075 | 3.75 → 1.65 |
| into each of the 4 corners | 0 → 0 | 43-68 → 0 | 0 → 0 | 0 → 0 | 1.53 → 0.85 | 1.02 → 0.36 | 0.00 → 0.61 |
| into the +Z / -Z net | 0 → 0 | 0 → 0 | 0 → 0 | 0 → 0 | 0.53 → 0.59 | 0.06 → 0.06 | 3.74 → 0.61 |
| hugging the +X wall | **28 → 0** | 92 → 0 | 28 → 0 | 53 → 0 | 0.764 → 0.678 | 0.550 → 0.212 | 0.00 → 0.34 |

The first two rows are the point of the other twelve: in open field every clamp is inert and the
numbers are identical to the digit, so the flat-ground framing Milestone 02 tuned is untouched.

**Ball cam, before and after.** Two routines were added while measuring, because the original "high
ball" case turned out to be two different failures wearing one hat — a ball the car *trails* and a
ball the car *overtakes* — and only splitting them showed that both failed for the same reason.

| Routine | car off screen | longest run | ball off screen | worst jump | worst jerk |
|---|---|---|---|---|---|
| ball cam, slow chase | 0 → 0 | 0 → 0 | 0 → 0 | 0.533 → 0.622 | 0.034 → 0.041 |
| ball cam, fast chase | 0 → 0 | 0 → 0 | 0 → 0 | 0.534 → 0.533 | 0.066 → 0.066 |
| ball cam, boosted chase | 0 → 0 | 0 → 0 | **7 → 0** | **1.738 → 0.767** | **1.458 → 0.089** |
| ball cam, high ball trailed | **21 → 0** | 12 → 0 | 2 → 0 | **3.067 → 0.534** | **2.533 → 0.066** |
| ball cam, high ball overtaken | **33 → 0** | 20 → 0 | 6 → 66 | **3.035 → 0.534** | **2.535 → 0.059** |
| ball cam, ball off to the side | 0 → 0 | 0 → 0 | **32 → 0** | 0.893 → 0.862 | 0.115 → 0.062 |
| ball cam, arriving at the ball | 0 → 0 | 0 → 0 | 0 → 8 | **1.741 → 1.698** | **1.442 → 0.166** |
| chase cam, same fast chase | 0 → 0 | 0 → 0 | 0 → 0 | 0.534 → 0.534 | 0.065 → 0.065 |

**The car is now on screen for every frame of every routine in both tables**, and the worst
single-frame camera move across the whole ball-cam set fell from 3.07 m to 0.53 m outside the one
arrival case.

The one row that went backwards is deliberate and is the trade the turn rate buys: driving *past*
the ball at 32 m/s and continuing away from it now costs 66 frames with the ball out of shot,
where before it cost 20 unbroken frames with the **car** out of shot while the view slid across at
3 m per frame. Losing sight of a ball you have chosen to drive away from is ordinary; having the
car ripped out from under you is not.

The turn rate was swept rather than guessed:

| `ballCamTurnRate` | car off screen, overtake | longest run | ball off, arriving | goal off, arriving |
|---|---|---|---|---|
| 1.5 | 0 | 0 | 73 | 106 |
| 2.5 | 0 | 0 | 11 | 49 |
| **4.0** | **0** | **0** | **8** | **28** |
| 6.0 | 4 | 4 | 4 | 16 |
| 9.0 | **38** | **36** | 0 | 6 |

4.0 is the last value that never loses the car at all; by 9.0 the swing is throwing the view around
again, which is the thing being fixed.

Two candidate changes were measured and **rejected**, which is worth recording so they are not tried
again. A `ballCamRangeStretch` that pulled the camera back as the car and ball separated made
"ball off to the side" strictly worse (ball off screen 0 → 24-31) and did not fix the high ball at
all (car off screen 35 → 23, still bad), so it was taken back out. Raising `ballCamNearRange` from
7 to 12 fixed the boosted-chase snap on its own, but once the direction rotation went in it made no
measurable difference to anything, so `ballCamNearRange` stayed at 7 — one fewer value moved.

| Check | Result |
|---|---|
| Debug / Development / Release | build with no game-code warnings, smoke test exits 0, screenshot non-blank, no errors in the log |
| Kickoff framing | unchanged on a screenshot, as the open-field rows predict |

Fullscreen at launch and a bigger goal interior, on `fix/final-milestone` on 2026-08-06. Neither is a
CLAUDE.md milestone; both were asked for directly.

**The game now opens fullscreen.** `GameSettings::fullscreen` defaults to true and `App::Initialize`
acts on it once, after `InitWindow`.

- **The window is resized to the monitor before the toggle.** `ToggleFullscreen` keeps the current
  window size as the fullscreen resolution, so without the `SetWindowSize` the game would fill the
  screen with a stretched 1280x720 image. Verified: `fullscreen=1 size=2560x1080` on this monitor.
- **It is `ToggleFullscreen`, not `ToggleBorderlessWindowed`.** `SettingsMenu::ApplyGraphics` compares
  the setting against `IsWindowFullscreen()`, which only knows about the former; using the borderless
  variant would leave the panel's Fullscreen row reading "Yes" over a window the API calls windowed,
  and the first toggle would then fight it.
- **The smoke test deliberately stays windowed.** It exists to render a fixed 1280x720 frame and write
  it out, and its screenshots are compared across configurations — going fullscreen would make them
  whatever size the machine running them happens to be. Verified: 0 fullscreen transitions under
  `--smoke-test`, and the screenshots are still 1280x720.
- `App::windowWidth/windowHeight` still exist and are still what a windowed launch gets, so turning
  Fullscreen off in Settings lands on the same 1280x720 it always did.

**The goal interiors were far too small to drive into**, and are now the Rocket League reference goal
in the same 100 uu = 1 m the field already uses (Milestone 16): 1786 x 642.775 x 880 uu.

| | Before | After |
|---|---|---|
| Interior, w x h x d | 14.00 x 5.00 x 4.00 m | **17.86 x 6.43 x 8.80 m** |
| Depth in car lengths | 1.25 | **2.75** |
| Depth against the car's 3.62 m rotation diameter | 1.10x — no usable margin | **2.43x** |
| Driving in at speed, deepest point reached | 2.55 m past the line | **7.40 m** |
| Car stayed flat on the way in | yes | yes |
| Reversing back out | 0.64 s | 0.91 s |

The rotation diameter is the row that explains the complaint: a 1.7 x 3.2 m box needs 3.62 m of
clear space to turn on the spot, and the old recess was 4.00 m deep. Measured with a temporary
harness that drove a car in and back out at both sizes (removed afterwards).

Two things about that harness are worth keeping, because both cost a run. **A car cannot turn round
on the spot in either goal, and that is the steering model rather than the goal**: steering authority
ramps in with speed (`steerSpeedFloor`), so a stationary car nosed into the back of a net has no yaw
rate at all — the same fact `BotController` handles by reversing. The escape test therefore has to
reverse, as a player does. And **the two sizes have to be tested from the same depth**: starting each
at `depth - 2` made the deeper goal look worse purely because the car began further in.

| Check | Result |
|---|---|
| Lattice or seam lines drawn across the mouth | none — see the note below |
| Stands still clear of the deeper recess | yes, they derive from `goalDepth`; first tier now starts 2.8 m behind the net |
| Debug / Development / Release | build with no game-code warnings, smoke test exits 0, screenshot non-blank, no errors or warnings in the log |

Final Milestone 6.3 feedback and juice, implemented on `fix/final-milestone` on 2026-08-06. All three
of its README items were ticked and **all three turned out to be unimplemented or half-implemented**.
Measured with a temporary harness that forced each piece of feedback into view, screenshotted it and
read the camera numerically (removed afterwards).

**There was no screen punch at all.** Only the goal colour flash in `HUD.cpp` existed; the camera
itself never moved on any event.

| Event | Eye moved | Aim step per frame | Peak aim offset | Car off screen |
|---|---|---|---|---|
| Idle, before and after | 0.3235 m | 0.040 deg | 0.38 deg | 0 / 60 |
| Big ball hit, **before** | 0.0003 m | 0.034 deg | — | 0 / 60 |
| Big ball hit, **after** | 0.0003 m | **0.973 deg** | **0.97 deg** | 0 / 60 |
| Goal, **before** | 0.0000 m | 0.034 deg | — | 0 / 60 |
| Goal, **after** | **0.0000 m** | **2.503 deg** | **2.50 deg** | 0 / 60 |

The "before" rows for a goal and for idle are identical to three decimals, which is the measurement
that says the feature was simply absent.

**The boost flame did not scale with anything.** `DrawBoostFlame`'s own comment claimed `strength`
made a tapped boost look different from a held one, but the only thing ever passed was a flicker, so
the cone was the same size at 0.15 s and at 1.5 s of hold — checked on screenshots as well as in the
arithmetic.

| Boost held | Intensity | Flame scale | Cone length | Trail per frame |
|---|---|---|---|---|
| Before, any hold | — | 1.00 | 2.30 m | 3 |
| 0.05 s | 0.11 | 0.42 | **0.97 m** | 1 |
| 0.15 s | 0.33 | 0.57 | 1.30 m | 2 |
| 0.30 s | 0.67 | 0.78 | 1.80 m | 3 |
| 0.45 s and beyond | 1.00 | 1.00 | **2.30 m** | 4 |

**The boost pads told you ready or not-ready and nothing else.** Two different colours, but every pad
on cooldown looked like every other one: a pad taken a moment ago and a pad about to come back were
the same flat dark disc, so there was nothing to plan a run around. The lit disc now grows back with
the charge, and a ready pad breathes.

| Pad state | Before | After |
|---|---|---|
| Ready | flat bright disc | bright disc, gently pulsing |
| Just taken (95% of cooldown left) | flat dark disc | dark seat, no light |
| Nearly back (20% left) | **the same flat dark disc** | **dark seat with the light 80% grown back** |

| Check | Result |
|---|---|
| Punch ever loses the car | no — 0 of 60 frames on a goal, a big hit and idle alike |
| Punch ever moves the eye | no — 0.0000 m on a goal, by construction |
| Debug / Development / Release | build with no game-code warnings, smoke test exits 0, screenshot non-blank, no errors or warnings in the log |

Final Milestone 6.4 bot, implemented on `fix/final-milestone` on 2026-08-06. Measured with a temporary
harness that played whole matches with no rendering at all - the match state machine and the physics,
stepped directly - and with a provoked test that deliberately wedged the bot (both removed
afterwards). **Milestone 12's bot numbers were all stale**: the field, the ball's rolling damping and
the goals have all changed since, so everything below was re-measured from scratch.

**Getting stuck was already solved, and nothing was changed for it.** Eight deliberate wedges, ten
seconds each:

| Provocation | Free and driving in | Resets | Ended up |
|---|---|---|---|
| Nose into the +X wall | 0.37 s | 0 | 39 m away |
| Nose into the -X wall | 0.33 s | 0 | 37 m away |
| Nose into its own back wall | 0.12 s | 0 | 13 m away |
| Nose into the +X-Z corner | 0.34 s | 0 | 68 m away |
| Nose into the -X+Z corner | 0.34 s | 0 | 11 m away |
| Deep inside its own net | 0.12 s | 0 | 12 m away |
| Deep inside the far net | 0.32 s | 0 | 42 m away |
| Upside down at midfield | 1.29 s | 0 | 6 m away |

**The failure was that the bot only ever scored from the kickoff.** Every goal in every match -
52 of 52 against an idle opponent, 54 of 54 against a goalie, 6 of 6 against a mirror of itself -
landed within 6 s of the field going live, at an average of 3.1 to 3.3 s. In the mirror match that
left **199 seconds of contested open play that produced three shots on target and no goals at all**.
That is the "not trivial" half of CLAUDE.md 6.4 failing: survive the kickoff and the bot has nothing.

The cause is that it always drove at the ball, including when the ball was already past it in its own
half — which is also the over-commitment the milestone names. Two cars shoving the same ball from
opposite sides move it nowhere: the ball was under 3 m/s for 35% of open play. **The car that is out
of position backing off is what gives either of them a clean run at it.**

The fix is one branch: in its own half, if the ball is already past it and further away than
`recoverMinRange`, it drives to a point in front of its own goal instead of at the ball, and picks
the attack straight back up as soon as it is goal-side again.

**A mirror match cannot measure this and it was a mistake to try.** Two identical agents are a chaotic
system: the same parameter measured 6-9, then 11-6, then 47-52 across runs that differed only in
rounding. Everything below is instead **six decorrelated three-minute matches against a fixed
opponent — the bot exactly as it was before this change** — with the kickoff nudged sideways per run
so the matches are genuinely independent.

| Bot under test | Goals | Of those, open play | Conceded | Goal difference | Shots | Worst stall | Resets |
|---|---|---|---|---|---|---|---|
| **Before** (always chases) | 3.2 | 1.0 | 4.3 | **-1.1** | 1.7 | 1.58 s | 0 |
| **After** (recovers goal-side) | **6.8** | **2.0** | 6.0 | **+0.8** | 1.8 | **1.33 s** | 0 |

It went from losing to the old bot to beating it, doubled its open-play goals, and got *less* prone
to stalling while doing it. It is still beatable: it never jumps, never goes for an aerial, reads the
ball 0.25 s ahead and keeps a boost reserve, all of which are unchanged.

Both new numbers were swept against that same fixed opponent rather than guessed:

| `defendStandOff` | Goals | Open play | Goal difference | Worst stall |
|---|---|---|---|---|
| 8 m | 5.5 | 1.2 | -1.2 | 1.93 s |
| **12 m** | **6.8** | **2.0** | **+0.8** | **1.33 s** |
| 18 m | 2.3 | 1.5 | -0.9 | 3.78 s |

| `recoverMinRange` | Goals | Open play | Goal difference | Worst stall | Resets |
|---|---|---|---|---|---|
| 0 m (never contest) | 5.0 | 1.5 | 0.0 | 1.93 s | 0 |
| **8 m** | **6.8** | **2.0** | **+0.8** | **1.33 s** | **0** |
| 14 m | 4.7 | 1.5 | -0.5 | 5.11 s | 1 |

| Check | Result |
|---|---|
| Debug / Development / Release | build with no game-code warnings, smoke test exits 0, screenshot non-blank, no errors or warnings in the log |

Final Milestone 6.5 performance and stability, implemented on `fix/final-milestone` on 2026-08-06.
Measured with a temporary harness that timed whole frames with the frame rate cap lifted and timed
each pass inside them (removed afterwards), plus fault injection on the post-processing and full
matches run in all three configurations.

**Read the hardware caveat before quoting any of these numbers.** They were taken on an **NVIDIA
RTX 3060**, which is not the "common laptop" CLAUDE.md 6.5 asks about. What they establish is the
shape of the cost, and that is the part that carries across machines.

Release, uncapped, at the resolutions that matter:

| Resolution | Bloom on | Bloom off | Frames over 16.7 ms |
|---|---|---|---|
| 1280 x 720 | 0.61 ms (1626 fps) | 0.49 ms (2033 fps) | 0 of 840 |
| 1920 x 1080 | 0.60 ms (1677 fps) | 0.51 ms (1952 fps) | 0 of 840 |
| 2560 x 1080 (the fullscreen default) | 0.62 ms (1613 fps) | 0.54 ms (1846 fps) | 0 of 840 |

Bloom costs 0.08 to 0.12 ms, which agrees with the 0.1 ms Milestone 14 measured for it.

**The game is not fill-bound**, which is the finding that matters for a weaker GPU. Pushing the
resolution far past anything real:

| Resolution | Megapixels | Mean frame |
|---|---|---|
| 1280 x 720 | 0.92 | 0.59 ms |
| 2560 x 1440 | 3.69 | 0.85 ms |
| 3840 x 2160 | 8.29 | 1.32 ms |
| 5120 x 2880 | 14.75 | 1.91 ms |

Sixteen times the pixels costs 3.2 times the frame, so the cost is about **0.50 ms fixed plus
0.096 ms per megapixel** on this GPU. The fixed half is CPU submission, which barely varies with the
GPU at all:

| Pass | Submission cost per frame |
|---|---|
| 3D objects | 0.23 ms |
| Glass walls and their lattice | 0.17 ms |
| Bloom resolve | 0.04 ms |
| HUD | 0.007 ms |
| Effects | 0.006 ms |

So on a machine with, say, twenty times less fill rate than this one, 1080p would be roughly
0.45 ms of submission plus 4 ms of fill - still four times inside a 60 fps budget. That is an
extrapolation from the slope above, not a measurement, and it is the honest limit of what this
machine can say.

**Post-processing degraded gracefully in one direction and badly in the other.** The missing-shader
path was already right. The missing-*targets* path was not: `EnsureTargets` never checked whether
`LoadRenderTexture` had actually succeeded.

| Fault | Before | After |
|---|---|---|
| Bloom shaders missing | warns once, draws unbloomed, exit 0 | unchanged |
| Render targets fail to allocate | **no check at all** | warns once, draws unbloomed, exit 0 |
| Warnings logged over 300 frames of the target fault | **300** | **1** |
| `LoadRenderTexture` calls per frame while failing | **3, forever** | 3 once, then none |

Both faults were injected and run, not reasoned about: with the targets forced to fail the game
writes its screenshot and exits 0 with a single warning, and the same with `Bright.fs` deleted.

**A full match logs nothing.** Two rounds of this. First 20000 frames - 333 seconds of continuous
play, 39 goals and therefore 39 celebrations, resets and kickoffs - in each configuration, Debug
included, so with `JPH_ENABLE_ASSERTS` on. Then 32000 frames in Debug and Release, which is long
enough to run the clock all the way down and reach the full time screen, since that transition is
exactly where an end-of-match bug would live:

| Run | Configuration | ERROR | WARNING | Asserts, aborts, segfaults | Exit |
|---|---|---|---|---|---|
| 333 s, mid-match | Debug | 0 | 0 | 0 | 0 |
| 333 s, mid-match | Development | 0 | 0 | 0 | 0 |
| 333 s, mid-match | Release | 0 | 0 | 0 | 0 |
| To full time | Debug | 0 | 0 | 0 | 0 |
| To full time | Release | 0 | 0 | 0 | 0 |

The full time run finishes 0 - 54 with the FULL TIME panel, ORANGE WINS and both buttons drawn
correctly, screenshotted. The scoreline is what an idle player concedes - the smoke test never
presses a key - not a statement about the bot.

Resident memory was sampled twice during those runs and did not move by a single kilobyte
(Debug 209004 kB, Development 146356 kB, Release 144840 kB both times), which is what the fixed
particle pool and the fixed ramp meshes should give.

## Milestone 22 — Gamepad Support

One new module and a wire-up; no gameplay or rendering was touched.

- **`GamepadInput.h/cpp`** (namespace `gamepad`) — the only file that knows raylib's gamepad ids, the
  layout, the dead zone and the trigger curve. Everything else asks in game terms: `Throttle()`,
  `Jump()`, `MenuConfirm()`, `Rumble()`.
- **`PlayerController`** merges the pad into `CarInput` beside the keyboard.
- **`UserInterface::MenuList`** gained the pad once, so every menu got it at once.
- **`MatchScene`** (pause, rumble), **`ChaseCamera`** (ball cam), **`CarSelectScene`**,
  **`HowToPlayScene`**, **`MainMenuScene`**, **`HUD`** (full time) each picked up their one button.
- **`SettingsMenu`** gained a **Controls** section: gamepad on/off, stick dead zone, vibration, held
  in `GameSettings` beside everything else and shared by both menu contexts.
- **`App::Run`** calls `gamepad::Update(settings)` once per frame, before the scene.

### The layout, as built

| Action | Button |
|---|---|
| Accelerate / reverse | RT / **LT** |
| Steer | Left stick |
| Jump, double jump, flip | A |
| Boost | B |
| Ball cam | Y |
| Pitch and yaw (airborne) | Left stick |
| Air roll (airborne) | X held, the stick rolls instead of yawing |
| Reset car | RB |
| Pause | Start |
| Menu move / change value | Left stick or D-pad |
| Menu confirm / cancel | A / B |

**One deliberate departure from the table in CLAUDE.md 22.1:** that table lists **RT** for both
Accelerate and Reverse, which cannot be right — one trigger cannot be both. Reverse is on **LT**, as
in Rocket League itself. If the table meant something else, `GamepadInput.cpp` is the only file to
change.

### What was verified, and what could not be

There is **no controller on this machine**, so nothing that needs a button pressed has been run. What
was proven, by a temporary harness that called the real module and drew the real screens (removed
afterwards):

| Check | Result |
|---|---|
| Radial dead zone: inside is zero, full is full, half past the zone is half | ok |
| A diagonal push keeps a magnitude of 1 and does not clip to the square corner | ok |
| Trigger curve, signed convention: rest 0.00, half 0.47, full 1.00 | ok |
| Trigger curve, unsigned convention: rest 0.00, half 0.47, full 1.00 | ok |
| An unpolled axis (frame zero) is **zero throttle**, not half | ok — see below |
| An idle pad contributes nothing: no throttle, steer, air control, buttons or menu input | ok |
| `PlayerController` with no keys held and a pad enabled is all zero | ok |
| Settings switch off makes the pad unavailable | ok |
| `Rumble` with no pad is a silent no-op | ok |
| Settings panel with the new Controls section: 678 px of a 720 px window, all rows inside | ok, screenshotted |
| How-to-play sheet in both languages, lines inside their panels | ok, screenshotted both |
| Debug / Development / Release build with no game-code warnings, smoke tests exit 0 | ok |

**Still unproven, and it needs a real pad:** that the buttons are mapped where a player expects them,
that the analogue feel is right, that vibration fires, and that unplugging mid-match leaves the car
drivable (the code path is there — an absent pad is simply not found and every value stays at its
zero — but it has not been watched happen).

### Two things worth knowing before touching this

- **A trigger reads 0 before raylib has polled the pad, and 0 through the signed remap is half
  throttle.** raylib's axis array starts zeroed and is filled in `EndDrawing`, so the first frame
  would have driven the car off on its own. `Trigger()` therefore only uses the -1..1 remap once it
  has seen that trigger resting at a real negative value, and otherwise reads the raw number as
  already 0..1 — which is both what an unpolled axis wants and what a driver that reports triggers
  unsigned wants. This was not theoretical: the harness caught it on the first run.
- **This machine enumerates its keyboard as a six-axis gamepad** (`RDMCTMZT CIDOO QK75 System
  Control`), so `IsGamepadAvailable(0)` is true here with no controller attached. Do not read "a pad
  is available" here as "a controller is plugged in".
- **`gamepad::Update` therefore reads every pad that is present, not the first one — and that is a
  fix, not a nicety.** The first version took the lowest present index, which on this machine is that
  keyboard device, and a real controller plugged in behind it produced **no response at all**:
  reported by the user, and the reason the module now merges. `IsGamepadAvailable` is only
  `glfwJoystickPresent`, so "present" means any joystick. Merging is safe because a joystick GLFW has
  no gamepad mapping for gets an all-zero state from `glfwGetGamepadState` — buttons clear, sticks at
  zero, triggers forced to their resting -1 — so it contributes nothing to the strongest-wins merge.
  This is not local multiplayer, which is still out of scope: it is one player and a machine that
  lies about its hardware. The pad that last did something is the one that gets rumbled.

## Layout

```
Game/Source/
  Main.cpp, App.h/cpp          entry point, window, settings, scene switching, smoke test
  Scene.h/cpp                  camera + GameObjects + Jolt PhysicsSystem, fixed step loop
  TitleScene.h/cpp             the launch screen: logo and prompt over the live arena
  MainMenuScene.h/cpp          the showcase menu: arena and a turntabling car behind the list
  HowToPlayScene.h/cpp         the how-to-play screen: six panels of rules and controls
  CarSelectScene.h/cpp         the car picker: six cooked cars on a 2 x 3 grid of pedestals
  MatchScene.h/cpp             the gameplay scene and the pause overlay
  ImGuiRaylib.h/cpp            namespace imgui: the whole Dear ImGui backend, dev builds only
  TuningPanel.h/cpp            the F1 tuning panel and its Tuning.cfg, dev builds only
  HUD.h/cpp                    namespace hud: every in-match readout, plus the full time screen
  Effects.h/cpp                namespace effects: particles, boost flame, ball highlight, contact shadows
  AudioSystem.h/cpp            namespace audio: the ten procedural cues and the boost stream
  PostProcess.h/cpp            namespace postprocess: the optional bloom chain
  Match.h/cpp                  score, clock, kickoff/goal/full time state machine
  GameSettings.h               the settings struct App owns
  MenuAction.h                 what a scene asks App to do next
  UserInterface.h/cpp          namespace uistyle: colors, text helpers, MenuList widget
  SettingsMenu.h/cpp           settings panel reused by both menus
  GameObject.h/cpp             base object, body transform helpers
  StaticModelAsset.h/cpp       .evmodel loader + assets::Path, team repaint
  TextureAsset.h/cpp           .evtex loader: the QOI decoder and the upload
  Lighting.h/cpp               namespace lighting: the one lit shader, Apply/Detach
  GameObjects/CarObject.h/cpp  arcade car (single box body + cooked car model)
  GameObjects/BallObject.h/cpp the ball (single dynamic sphere)
  GameObjects/GoalObject.h/cpp net geometry + the fully-across-the-line test
  GameObjects/BoostPadObject.h/cpp refill pad, no physics body, cooldown + glow
  GameObjects/ArenaObject.h/cpp floor, walls, ceiling, back walls with goal openings, edge ramps
  CarController.h              CarInput + abstract controller
  PlayerController.h/cpp       keyboard input
  BotController.h/cpp          the AI opponent: approach point, boost, stuck recovery
  ChaseCamera.h/cpp            third person follow camera, ball cam and the arena clamps
  PhysicsLayers.h/cpp          namespace physics: layers and Jolt filters
Game/ThirdParty/   raylib, Jolt, glm, imgui     (cloned, git-ignored)
Game/Assets/       Cars-Park/ (OBJ, FBX, Blends, License.txt, Preview.png)
                   Icon/ (budget-league-logo.png), Textures/ (three unused arena PNGs)
                   Shaders/ (Lit.vs, Lit.fs, Bright.fs, Blur.fs)
Tools/             AssetCooker.py, FbxReader.py, requirements.txt
Build/<Config>/    ArcadeCarSoccer + assets/Models/*.evmodel + assets/Textures/*.evtex
                   + assets/Shaders/*
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
or **Space** starts the match, **Esc** goes back to the main menu, right click picks a car and
**left click on a car picks it and starts the match in one go**.

`make clean` removes the build output. `make clean-thirdparty` additionally forces a raylib rebuild.
The first build takes a few minutes (raylib + 153 Jolt translation units per config); after that it
is incremental.

To cross-compile the Windows executable from Linux (Milestone 21; full notes in
[docs/CrossPlatformBuild.md](docs/CrossPlatformBuild.md)):

```sh
make -C Game/ThirdParty/raylib/src clean      # only when switching toolchains
make release TARGET_OS=Windows \
     CXX=x86_64-w64-mingw32-g++ CC=x86_64-w64-mingw32-gcc AR=x86_64-w64-mingw32-ar
# -> Build/Windows/Release/ArcadeCarSoccer.exe + assets/
```

Use the `release`/`development`/`debug` goal, not a bare `CONFIG=Release` — the default goal
re-invokes make with `CONFIG=Development` and discards it.

## Publishing

[docs/ItchPage.md](docs/ItchPage.md) holds the itch.io store page copy — metadata (tagline, tags,
platforms) and the page body, written from the README and this document. Two things about it that
are not obvious: the page claims **Linux and Windows only**, because macOS is written into the
`Makefile` but has never been built here, and every upload must zip the executable **with its
`assets/` folder**, since without it the game falls back to box stand-ins and runs silent. Keep the
file in sync with `README.md` whenever the feature set moves.

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
- **Cross-platform support is `Makefile`-only.** `Game/Source/` has no platform `#ifdef`, no POSIX
  header and no shell call — every filesystem touch already goes through raylib — so Milestone 21
  changed no game code at all. `TARGET_OS` (`?=`, so it can be overridden on the command line)
  selects the link line, the `.exe` suffix and raylib's `PLATFORM_OS`; `ARCH` drops `SIMD_FLAGS` on
  ARM, where `-msse4.2` would not compile and Jolt uses NEON anyway.
- **Non-Linux targets nest under `Build/<TargetOS>/`.** Linux keeps the flat `Build/<Config>/` it
  always had, so every path in the docs stays right; without the nesting a native `.o` and a
  cross-compiled `.o` would share a filename and silently poison each other.
- **`CC` must be passed alongside `CXX` when cross-compiling** — raylib is C, and `CC`/`AR` are what
  get forwarded to its Makefile. And raylib leaves its `.o` files in its own source tree, so switching
  toolchains needs `make -C Game/ThirdParty/raylib/src clean` first even though the archives are
  per-target.
- **The Windows link is fully static** (`-static -static-libgcc -static-libstdc++`). raylib's own
  list has none of these because raylib is C; without them the C++ executable would need three MinGW
  runtime DLLs shipped beside it.

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
- **That 1.2 was tuned against the 80 m field and Milestone 16 left it behind.** The field became
  102.41 m long, so the same 20 m/s pass reached barely a third of the way down it and the ball read
  as sluggish - the "heavy but never sluggish" item in CLAUDE.md 6.1. **0.85** puts the roll back at
  the fraction of the field 1.2 was chosen for: 43.7 m instead of 35.6 m. Nothing else about the ball
  moved, and that is deliberate - the hit transfer (1.13-1.19x), the bounce apexes and the resting
  height all measure identically before and after, because none of them is a rolling number. Any
  future change to the arena length should look at this value again; it is the one ball tunable that
  is really a ratio to the size of the pitch.
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
- **A flip is a committed move, held for `flipDuration` rather than left to decay.** Setting the spin
  once and handing the car back to air control does not work: `airDamping` (0.8/s) and the body's
  `angularDamping` (2.2/s) together bleed about 3 rad/s, and measured, **no flip ever swept even half
  a turn** - the forward and backward ones stalled part way round and dropped the car on its roof.
  While `flipTimeRemaining` is running the angular velocity is re-set to `flipAxis * flipSpin` every
  step and air control is skipped entirely, so `flipSpin * flipDuration` is exactly the angle swept:
  12.0 x 0.52 is one turn.
- **The spin is cancelled when the flip ends, not released.** This is the sharp edge in it. Letting
  the timer expire and simply falling through to air control hands 12 rad/s back to a car that has
  just come round to level, and it carries straight past upright: traced, the car ended a clean full
  rotation at uprightness +0.999 and was at -0.9 a third of a second later, landing inverted every
  time. Zeroing the angular velocity on the step the timer reaches zero is what makes the car come
  out of a flip flat and stay flat all the way down.
- **Landing clears `flipTimeRemaining`**, so a flip that puts a wheel back on the ground early ends
  there rather than fighting the ground contact for the rest of its window.
- **The flip's bonus on the ball is applied a step *after* contact, along the direction the ball was
  just sent, and never on approach.** An impulse on approach is the obvious implementation and it is
  wrong: it shoves the ball out of the car's way before the two ever meet. Measured, the same flip
  left the ball at **8.5 m/s and 20 m** with a pre-contact impulse against **26.2 m/s and 52 m** with
  no bonus at all — the bonus was worse than nothing. Reading the hit as a jump in the ball's speed
  and adding to it is both correct and the idiom `MatchScene` already uses for the effects.
- **The proximity test on that bonus is against the box, not a radius.** The body is 3.2 m long and
  1.7 m wide, so a sphere big enough to reach past the nose claims hits a metre clear of the flank —
  and with two cars on the pitch that test is also what says the hit was this car's.
- **A flip is ballistic: the floor it rotates through may not add to its climb.** This is the whole of
  the 2026-08-18 fix, and it is not a tuning value — it is the reason the flip worked at all. The body
  is 3.2 m long and a flip fired straight after a jump starts about half a metre up, so as the car
  comes over, its nose or tail sweeps well below the floor and Jolt answers the overlap by throwing
  the car clear: measured, a forward flip left the ground climbing at 4.4 m/s and was climbing at
  9.9 m/s a tenth of a second later, peaked **6 m** up and hung there for **2.2 s**. While the flip
  runs, its vertical speed is therefore capped at what it had when it fired, falling away with gravity
  — the climb can only decay, never be added to.
- **Only the vertical is held; the speed along the ground is left alone.** Holding that as well was
  tried and is much worse: forcing the dodge speed back every step drives the nose deeper into the
  floor, and the car is thrown **7 m** up the moment the flip releases it. Same for dropping the
  body friction to zero for the flip (**9 m**). So the scrape still costs a very early flip most of
  its speed — that is unchanged from before the fix, and it is in the known-deviations list.
- **`flipCapped` is decided when the flip fires, by a ray a car length straight down, and never
  re-tested.** A flip high in the air has nothing to scrape and must keep every bit of the climb its
  boost is giving it, so the cap has to be conditional — but testing the condition every step lets a
  car that leaves probe range mid rotation and drops back into it slip a launch through the gap
  (measured, 4.5 m and 1.8 s). `nearGround` is the wrong probe for this: it stops 1.65 m down, while
  the body swings `halfExtents.z` below a centre of mass that is itself below the box centre, so a
  flip at the very top of a jump read as a clean aerial and was launched like all the rest.
- **The flip hold is tested before the grounded gate, and the flip takes `jumpLockout` like a jump
  does.** Inside the gate, a flip fired a few hundredths of a second after the jump — a fast
  Space, W, Space — was cancelled by the landing reset on the very step it fired, because the ground
  probe reaches 0.7 m below a car that has only climbed 0.3 m. That left the raw 12 rad/s spin with no
  cap on it, and the floor threw the car **4.4 m** up to tumble for **3.75 s**. The lockout is what
  says the probe is not a landing yet.
- **`CarObject::ResetTo` clears the car's transient state, not just its transform.** A reset that only
  teleports leaves a player who spent both jumps before a goal **kicking off unable to jump at all**,
  and carries a jump lockout or a half-finished flip through the countdown. `jumpHeldPrevious` is
  deliberately *not* cleared: setting it false would make a jump key still held across the reset read
  as a fresh press the instant play starts, and the car would jump on the kickoff whistle.
- **The cars spawn at exactly `halfExtents.y`, not a hair above it.** The old 0.36 was described as
  "half-height plus a hair so it does not visibly drop" - but a hair above the resting height *is* a
  drop, and it is the only thing that ever moved at a kickoff: measured, 0.0100 m of fall peaking at
  0.3839 m/s. `convexRadius` is inside the box in Jolt, so the resting height is the half extent
  exactly, and spawning there makes two seconds of idle play after a reset move the car 0.0000 m. The
  ball was already spawning at exactly `radius` and was already perfect.
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
- **Z is the goal-to-goal axis, X is sideline to sideline.** Milestone 16 sets the supplied
  Rocket League reference dimensions directly: 76.81 m wide, 102.41 m long and 20.73 m high.
  Blue defends +Z, orange defends -Z, and the player kicks off in front of blue.
- **The arena and each goal are one Jolt compound body each**, built from a `Piece` list that
  physics and rendering both read. That list is the single source of truth, so the collision cannot
  drift away from what is drawn — worth preserving when Milestone 14 adds trim and stands.
- **The ceiling has collision but is never drawn.** Drawing it would put a slab between the chase
  camera and the field the moment the car climbs a wall.
- **The stadium stands must start beyond the goal recess, and `ArenaObject::goalDepth` is what tells
  them where that is.** The recess reaches `goalDepth` behind the back wall, and `AddStadium` used to
  place the end banks `4 + tier * 5` metres out from `halfLength` — which put two opaque, non-solid
  boxes *inside* both nets, spanning the full mouth. That is the black slab that hid every car and
  ball more than about 1.6 m into a goal. Both of the previous attempts at it went to `GoalObject`
  and made its own enclosure invisible, which could never have worked: the enclosure was already
  invisible, and hiding it only exposed the stands behind it more. If the recess is ever deepened
  again, the stands move with it automatically — that is the whole reason the depth is a field on
  `ArenaObject` rather than only on `GoalObject`.
- **The side banks are extruded out to meet the end banks, not to `halfLength + out`.** Once the ends
  moved back the ring stopped closing, and a notch opened at each of the four corners.
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
- **The floor ramp radius is 5 m and the ceiling's is 3.5 m**, so the flat playing surface is
  66.81 x 92.41 m inside the 76.81 x 102.41 m arena, and the wall is truly vertical only between
  y = 5 and y = 17.23. `FlatHalfWidth()` / `FlatHalfLength()` report that flat area; anything that has to lie on
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
  nothing and keep both. The back-wall seam **used** to work out for free
  because `goalHeight` and `floorRampRadius` were both 5.0, so nothing ever crossed the opening. The
  goal is taller than the ramp now, so `DrawGlassWalls` skips the mouth explicitly: any seam or
  lattice segment that overlaps it in X while any part of it sits below `goalHeight` is left out, and
  the seam is carried across the top of the mouth instead so the ramp line stays continuous. Glass
  never writes depth, so being "behind" the opening would not have hidden them — they had to be not
  drawn at all.
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
  does not duplicate them. The car's spawn Y is its box half extent exactly, so it does not drop at
  all when the countdown ends - see the Milestone 6.1 note on that below.
- **Physics runs at a fixed 120 Hz** with an accumulator (`Scene::StepPhysics`, 0.25 s clamp).
  `GameObject::Update` is called once per *fixed step*, right before the simulation runs — this
  matters because Jolt forces only persist for one step, so applying them per frame would be wrong.
- **Field scale:** 76.81 m (X) x 102.41 m (Z) x 20.73 m (Y), with a 1.7 x 0.7 x 3.2 m car and
  32 m/s non-boosted top speed.
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

### Main menu showcase
- **The showcase car is a `StaticModelAsset` drawn directly, not a `CarObject`.** A `CarObject` would
  want a `PhysicsSystem`, a body and a controller for a prop that only turns on the spot. This is the
  same reasoning - and very nearly the same code - as the car picker's six previews.
- **The scene still calls `InitializePhysics`, because `ArenaObject` builds a body**, and physics is
  never stepped. It is the title screen's arrangement exactly: the bodies exist so the arena has its
  `pieces` list and the contact shadow has something to ray cast against.
- **All seven cooked cars are in the showcase pool, `SportsCar2` included.** The six-car restriction
  is the picker's (CLAUDE.md Milestone 08 says to use six of the seven); a showcase has no reason to
  hide one, and it is the one car the player can never otherwise see up close.
- **The pick is in `Initialize`, which is what "re-rolled every time the menu is re-entered" means
  here**: `App::SetScene` builds a fresh `MainMenuScene` every time, so there is no separate re-roll
  to write. Verified by building the scene twice in one process - `Cop` blue, then `SUV` orange.
- **Nothing re-seeds the random sequence**, so two launches inside the same second can show the same
  car: raylib seeds from the clock in `InitWindow`. Within a session it always differs.
- **The menu rows moved left but the widget did not change.** `uistyle::MenuList` lays out from the
  rectangle it is handed, so the column is a change to where `Draw` puts `row` - which is what
  CLAUDE.md means by wiring the pieces that already exist rather than building new UI.
- **"How to play" stays on the list.** CLAUDE.md Milestone 18 names Play, Settings and Exit, and what
  it explicitly rules out is the reference's online/shop/pass/garage/profile entries and its Weekly
  Challenges panel. Dropping the row would strand the how-to-play screen with no way in, so it was
  kept; delete it only together with `HowToPlayScene`.
- **The credits and build string were commented out in this file and are drawn again**, bottom left,
  because the milestone asks for them.
- **The settings panel stays centred rather than moving into the column.** It is the shared widget at
  its own preferred size, and while it is open it is what the screen is about - the same treatment it
  gets in the pause menu.

### Soundtrack
- **The playlist is found by scanning `assets/Music`, not by a list in the code.** Adding a track is
  dropping a `.mp3` into `Game/Assets/Sounds/`; the cooker copies it and the game picks it up. That
  is also why the file names with spaces in them are no problem - nothing ever types one.
- **The tracks are copied, not cooked.** They are streamed from disk by raylib a buffer at a time,
  so decoding them at cook time would trade 33 MB of MP3 for hundreds of MB of PCM for no gain. This
  is the same reasoning that leaves the shaders as plain text.
- **`track.looping = false` is what makes it a playlist.** raylib defaults music streams to looping,
  and with it on the first song would simply never end. With it off `UpdateMusicStream` stops the
  stream at the end, so `!IsMusicStreamPlaying` is the whole end-of-track test - no timers, no
  comparing `GetMusicTimePlayed` against the length.
- **One stream is loaded at a time**, and the previous one is unloaded before the next is loaded. The
  playlist holds paths, so it costs nothing however long it gets.
- **The shuffle never repeats a track back to back**: it re-rolls while the pick equals the current
  one. Measured over 50 advances - 0 repeats, and all six tracks reached. With a single track in the
  folder it plays that one again, which is the only sensible thing left to do.
- **`audio::UpdateMusic` must be called every frame**, from `App::Run`. It is what refills the stream
  as well as what notices a track ending; skipping frames would starve the buffer and stutter.
- **Music keeps playing through the pause menu and the whole match.** It is a soundtrack, not a
  gameplay cue, so nothing in the scenes touches it - unlike the boost loop, which every path that
  stops updating the car has to release.
- **The music volume is its own setting** (`GameSettings::musicVolume`, default 60), pushed from
  `App::Run` every frame beside the master and SFX volumes, so the slider is heard as it moves. It
  defaults under the effects because the cues are short and the music is continuous.

### Settings screen
- **The panel is one widget with a background flag, not two panels.** `SettingsBackground` says what
  the panel is being drawn over, and that is genuinely the only difference between its two homes:
  the pause menu has already darkened the whole screen, the main menu has not.
- **The flag decides who dims.** In `Showcase` the module lays down 0.35 itself, so the panel is the
  focal element while the arena and the car still read behind it; in `Dimmed` it draws nothing extra,
  because `MatchScene` has already drawn its 0.65 pause overlay and a second one would stack.
- **The main-menu panel is centred from `PreferredWidth/PreferredHeight`**, which are derived from the
  layout constants, so adding a settings row keeps it centred instead of pushing it off the bottom.
- **The game title moved inside the else branch rather than being deleted.** It is the menu's header,
  not the screen's, so it comes back the moment the panel closes.

### Title screen and the texture pipeline
- **The title is a real scene showing the real arena, not a picture of one.** It builds
  `ArenaObject`, both `GoalObject`s and `BallObject` exactly as the match does, so it can never show
  a stadium the game no longer looks like. It also gets the lit shader and the bloom for free,
  because both are global.
- **Physics is never stepped, and that is what "no gameplay happens on the title" means in code.**
  The bodies exist only so the arena builds its `pieces` list and the ball has a transform; with no
  `StepPhysics` call, gravity never runs. Measured: the ball sat at exactly its spawn position for
  13 s, to four decimals.
- **Both goals are built even though only one is in shot.** The opening in the back wall is a hole,
  and without a net behind it the middle of the frame is a black rectangle. It is the same ten lines
  `MatchScene::Initialize` runs; they are repeated rather than shared, because the alternative was a
  new abstraction over two call sites.
- **The camera stands still and only yaws.** A camera that orbits has to be kept out of the walls,
  which is the whole problem `ChaseCamera` exists to solve; a fixed point that sweeps 2.5 degrees
  either side over 24 s reads as alive and cannot collide with anything.
- **The composition is the reference shot's, and it is the reason for every number in the file.**
  The goal sits left of centre and the ball bottom right specifically so the centred logo and the
  prompt land on empty pitch and empty wall. Move `CAMERA_POSITION`, `LOOK_AT` or `BALL_SPOT` and all
  three overlap again - that was the state of the first four attempts.
- **Any button is three sources, and `GetKeyPressed` must be read exactly once**, because it pops the
  key queue. `GetGamepadButtonPressed` returns `GAMEPAD_BUTTON_UNKNOWN` when nothing is down, not
  -1, so comparing against it is correct. The press is ignored during the fade in, so a key still
  held from launching the game does not skip the screen before it is visible.
- **`DrawShadowedText` moved from `HUD.cpp` into `uistyle`.** Every line the title draws sits over
  the pitch rather than over a panel, which is exactly the case that helper exists for. Same story as
  `uistyle::Button`, which was a file-static in `CarSelectScene.cpp` until a second screen needed it.
- **The mark is the badge and the name side by side**, as on the reference: the shield on the left,
  BUDGET over LEAGUE to its right, the pair centred as one block. Everything is measured from the
  name (`NAME_SIZE` through `uistyle::FontSize`), so the badge follows the text and the whole mark
  scales with the rest of the UI; with no logo the badge is simply zero wide and the name centres by
  itself, which is also the missing-texture fallback.
- **The badge is drawn at 1.85x the name block, not the reference's 1.5.** The shield fills only 82%
  of its own image - measured on the alpha channel - and the rest is the glow margin. That margin is
  also what spaces the badge off the name, so there is no gap of its own.
- **The name is `DrawShadowedText`, not `uistyle::DrawTitle`.** DrawTitle centres one line and draws
  the accent underline under it, which is the menu treatment; the reference mark is two left-aligned
  lines with no rule, over an arena that can be any brightness.
- **The logo goes through the cooked texture pipeline** (CLAUDE.md 3.2), which did not exist before:
  `.evtex` is the magic `EVTXQOI1`, width, height, channels and payload length, then a QOI chunk
  stream. raylib reads PNG natively, so loading the source file directly would have been one line -
  but the format is what CLAUDE.md 2.6 and 3.2 specify, and it is what any other UI texture will use.
- **The stream carries neither QOI's own 14 byte header nor its end marker.** The header would
  duplicate the size that is already in the `.evtex` header, and the decoder stops at the pixel count
  rather than at a marker. Both encoder and decoder must agree on that: they are `EncodeQoi` in
  `Tools/AssetCooker.py` and `DecodeQoi` in `TextureAsset.cpp`, and the round trip is what proves it.
- **QOI was verified by decoding the cooked files back in Python and comparing them to the source
  PNGs**, not by looking at the screen. All four are byte for byte identical (see the table above);
  a subtly wrong `QOI_OP_LUMA` would still have looked plausible in a screenshot.
- **Textures are capped at 512 px** (CLAUDE.md 3.2), so the 1024 px logo is resized at cook time.
  Nothing in the game draws it larger than about a third of the window height.
- **The pixels are handed to the GPU and dropped.** `LoadTextureFromImage` copies, so the decode
  buffer is a `std::vector` that goes out of scope - there is deliberately no `Image` kept and no
  `UnloadImage` call to match.
- **The cooker skips `Cars-Park`** when looking for PNGs: the pack ships a preview thumbnail of
  itself, and the models are what that folder is for.
- **The cooker now needs Pillow**, which is the first real Python dependency the build has had. A
  missing one fails the texture step loudly rather than silently shipping no logo; models and shaders
  are unaffected.

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
  light never moves. They are deliberately not on the tuning panel, which carries the physics and
  camera values CLAUDE.md lists.

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
- **`minDistance` (1.5 m) used to be the one clamp that could still put the camera in a wall, and
  Milestone 6.2 took that away from it.** It sat on the *final* clamp, where `clear < minDistance`
  meant "sit at 1.5 m" whether or not there were 1.5 m to sit in — so along a blocked line it did
  not floor the pull-in, it authorised the eye to cross a surface. Measured, that put the camera
  inside the arena's own collision for **11-25% of the frames** of every wall-contact routine, with
  the car hidden behind geometry for exactly the same frames. It now floors the *desired* position
  instead, which is what it was always described as doing, and the final clamp has no floor at all:
  the surface is the only thing that decides. Raising it back towards the old 2.5 m still
  re-introduces the ramp clipping it was measured to cause, so leave it at 1.5.
- **The final clamp keeps the full `wallMargin` when the gap can pay for it and half the gap when it
  cannot** (`max(solid - wallMargin, solid * 0.5)`). Both branches are strictly inside the free
  space, which is the property the old floor lacked; taking `solid - wallMargin` alone would go
  negative whenever the surface was closer than the margin.
- **`SolidReach` reports the raw distance plus a `blocked` flag rather than folding the margin in.**
  Only one of its two callers wants the margin, and the original note still applies to that one:
  subtracting it when nothing was hit creeps the camera in every frame and compounds, because the
  clamped position is what gets stored.
- **The camera lifts along the surface the car is standing on, not along the world.** `CarObject`
  already computes that normal every step for its own driving (`alignTo`); 6.2 stores it as
  `CarObject::surfaceNormal` and `ChaseCamera` smooths a copy of it into `surfaceUp`. On the flat
  floor the two vectors are identical, so nothing about ordinary framing changes — verified, the
  open-field rows match to the digit. On a wall it is what stops the camera climbing the wall
  alongside the car: a car that stalls high on one sits in the ceiling fillet, and a world-up offset
  drove the view straight into it, collapsing the trail to 0.37 m and holding the car off screen for
  the rest of the run.
- **`surfaceUp` is smoothed at 3/s, slower than the position at 7/s.** The normal under the car can
  flick between facets on a ramp, and an unsmoothed lift would hand that straight to the framing.
- **A car on a wall or the ceiling has no usable flat forward, and normalising what is left of one
  is a bug, not a fallback.** `FollowDirection` flattened the car's nose to XZ and normalised
  whatever remained; on a vertical wall that is a vector a couple of centimetres long whose
  *direction* is numerical noise, and it swung the camera through a half circle between frames. It
  now keeps the last usable direction below `flatForwardMinimum` (0.25). The old guard was 0.001,
  which is far too small to catch this — by the time the vector is that short the direction has been
  meaningless for a long while.
- **A car turning over has no usable flat forward either, and that one is not short — it is long and
  pointing exactly backwards.** Half way through a forward flip the car is upside down and its nose
  points back down the pitch, so `FollowDirection` handed the camera a direction reversed by 180
  degrees and the view whipped round to the front of the car and back on every single flip (measured
  on the identical flip, old camera against new: **180.0 deg off, 0.38 s in**). The length guard
  cannot catch it, because the vector is full length the whole way. So the nose is also rejected when
  the car has rolled away from the up the camera is framing against — `surfaceUp`, not the world's
  up, which is why driving upside down on the ceiling is unaffected: there the car's up and the
  ceiling's own up agree. Verified: boosting up the side wall to 20.4 m holds the car within 9.7 deg
  of centre and never puts the eye behind a surface, identical to before the change.
- **The velocity blend is tested against the direction the camera is using, not the car's own nose.**
  It used to gate on `GetForwardSpeed() > 0`, which reads negative while the car is inverted, so a
  flipping car stopped feeding the camera its travel direction exactly when the nose was also being
  rejected and the view had nothing left to follow. Testing the velocity against the direction in
  use keeps the camera live through a flip and still fails, as before, when the car is reversing.
- **Ball cam rotates the side it sits on rather than interpolating its position, and that is the
  single biggest fix in 6.2.** The direction can genuinely reverse: the moment the car overtakes the
  ball, the ball is behind it and the view belongs on the other side. Smoothing the camera's
  *position* between those two sides draws a straight line, and that line runs through the car —
  measured, the view crossed at 3.07 m a frame and the car was off screen for 20 unbroken frames.
  Rotating the direction instead swings the camera round the car at its own distance and never
  crosses it. It applies in ball cam only: chase mode's direction follows the car's heading, and
  chase framing is left exactly as it was. (That was written as "which cannot reverse", and it was
  wrong — a flip reverses it, which is the bug fixed on 2026-08-18 above. Rotation is still the right
  answer for ball cam and still unnecessary for chase, because the heading no longer reverses at all.)
- **`ballCamActive` seeds the rotation instead of smoothing it on the first frame in ball cam**, so
  pressing `C` never starts a swing from a direction the camera was never actually at.
- **The near-range handover is a smoothstep, not the raw ratio.** A linear weight changes the follow
  direction fastest exactly as the ball crosses the car, which is the worst possible moment for it;
  smoothstep is flat at both ends.
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

### Car selection: click to start
- **Left click on a car is the whole choice in one action** — that car, and start the match — while
  right click still only highlights, so the grid can be browsed with either button. Left clicking the
  car that is already highlighted therefore behaves exactly like pressing START MATCH. The two
  buttons were the other way round at first and were swapped on request.
- **The car cells and the START MATCH / BACK buttons cannot overlap**, which is what makes it safe for
  a cell to act on the same button the buttons use. Measured at 720p: the lower row of cells ends at
  y = 519 and the buttons start at y = 624, and both scale with the window, so a click on a button
  never lands on a car as well.
- The click is collected in the same cell loop that does hover and right click, into `startClicked`,
  and applied after the buttons are drawn. It cannot be handled inside the loop, because `selected`
  is what the start path reads and the buttons have not been drawn yet at that point.

### HUD
- **`hud` is a namespace, not a class, because the HUD owns no state at all.** Everything it shows is
  read from the `Match` and the car on the frame it draws, including both animations: the kickoff
  digit swells from `match.stateTimer`'s fraction and the goal band wipes open and shut from the same
  timer against `match.celebrationTime`. Adding a timer of its own would be a second clock to keep in
  step with the one the match already runs.
- **`Draw` and `DrawFullTime` are separate because only one of them takes input.** `MatchScene` skips
  the full time screen while the pause menu is up, so its buttons never draw hover states underneath
  the dimmer. That is also why `DrawFullTime` returns a `MenuAction` rather than setting
  `pendingAction` itself — scenes own that field, as everywhere else.
- **A rematch is `MenuAction::StartMatch` from inside the match**, which makes `App` tear the scene
  down and build a fresh one. No reset path was added; `Match::Begin` already does everything a new
  match needs and the scene has nothing worth keeping.
- **There is no boost-pad hint, and that is deliberate.** A marker pointing at the nearest ready pad
  was built, measured and then **removed on request**. If it is ever wanted again, three things it
  took to work are worth knowing: a pad behind the camera projects *mirrored through the screen
  centre*, so it has to be flipped back before being clamped to a screen edge or it points exactly
  the wrong way (`GetWorldToScreen` gives no sign of this — the test is the dot product of
  camera-to-pad against camera forward); the marker has to be outlined, because it is the same yellow
  as the pads it points at and vanished into them; and it has to be suppressed under about 8 m, or it
  sits on top of the car pointing at a pad already filling the screen.
- **`DrawShadowedText` exists because HUD text over the field has no controlled background.** It
  draws a dark copy offset by two scaled pixels first. The kickoff countdown needs it — the digit
  lands over the far goal, the one bright thing at the centre of the screen. Anything else drawn onto
  the field rather than onto a panel should use it too.
- **Speed graduated from debug text to a HUD element; GROUNDED/AIRBORNE and the camera mode did not.**
  Those two stay in `MatchScene::Draw` behind `GAME_DEV_TOOLS`, so Release draws neither — verified on
  a Release screenshot.

### Effects and post-processing
- **The screen punch rotates the aim and never moves the eye.** That is the whole reason it is safe:
  Milestone 6.2's clipping guarantees are all statements about where the *eye* is, so a punch that
  only moves the look-at point cannot put the camera through a wall however hard it is hit — measured
  at 0.0000 m of eye movement on a goal. A positional shake would have had to be re-validated against
  all fourteen 6.2 routines, and would have fought the pull-in every time it fired near a wall.
- **The shake is two sine pairs at incommensurate rates, not random numbers.** It stays smooth at any
  frame rate, it is reproducible between runs, and — the non-obvious one — it does not draw from
  raylib's global random sequence, which the particle bursts and the audio cues share. Pulling from
  it here would shift every random number they take.
- **`ChaseCamera::Shake` takes the strongest pending punch rather than accumulating.** A scramble in
  front of the net fires a big-hit punch several times a second; adding them would compound into
  nausea, and the goal that follows has to still read as bigger than any of them.
- **Strength is squared before it is applied and decays at 3.4/s**, so the punch lands hard and is out
  of the way in about a third of a second. The goal is the only full-strength one in the game
  (2.50 degrees of aim offset); a big hit reaches 0.97.
- **The punch is cleared with the field on a kickoff**, next to `effects::Clear()` — the same reason
  the particles are: everything has just been re-centred and nothing should carry over.
- **`CarObject::boostHeldTime` is what makes the flame scale, and it is reset by `ResetTo`.** The
  flame took a `strength` argument from the day it was written and the caller only ever passed a
  flicker, so the documented behaviour never existed. Intensity ramps over `boostRampTime` (0.45 s)
  and drives the cone, the ember count, their size and their life together, so a tap is a puff and a
  held boost is a stream.
- **The boost pad's lit disc grows back with the charge.** Ready versus not-ready was already two
  colours; what was missing was *how far off* a pad was, which is the thing a player routes around.
  The dark seat is always drawn underneath so the pad never vanishes from the floor, and the ready
  pulse is what separates "full" from "nearly full" without introducing a third colour.
- **`effects` is a dumb particle pool that decides nothing.** `MatchScene::UpdateEffects` watches the
  match and the ball and calls `Burst` on the changes it sees, exactly as it does for the HUD. Every
  effect therefore fires from a *change*, not from a state, which is why the goal burst does not
  re-fire every frame of the celebration.
- **A big hit is a jump in the ball's speed, not a contact callback.** The size of the jump is exactly
  how hard the hit was, so it sizes the burst for free, and the physics stays free of listeners -
  the same reasoning that keeps goal detection analytic.
- **`CarObject::jumpPending` is a latch, not a per-step flag.** `Update` runs twice per rendered frame
  at 120 Hz, so a flag set and cleared inside the step would be missed by half the jumps. It is set by
  the car and cleared by whoever consumes it.
- **Shadows are contact shadows, not a shadow map.** A ray is cast down from each car and the ball,
  and a dark disc is laid on the surface it finds, rotated onto that surface's normal so it follows
  the ramps rather than cutting into them. It costs one ray per object against geometry that is
  already there, and in a flat-shaded low-poly arena it reads as well as a shadow map would. A real
  shadow map is the upgrade path if the arena ever gets more vertical detail.
- **The particle meshes deliberately skip `lighting::Apply`.** They are meant to read as light, and
  flat-shading a flame cone turns it into a solid orange traffic cone.
- **The trail matters more than the flame, because of where the camera is.** A chase camera sits
  almost directly down the axis of the exhaust, so the flame is largely hidden behind the car; the
  embers spreading sideways are what actually communicate boost at a distance.
- **The stands and light rigs are `Piece`s with `solid = false`.** They are drawn from the same list
  as everything else, so they cannot drift from the arena, but they are skipped when the compound
  shape is built. That also keeps them out of the camera's occlusion ray, which tests the same body -
  otherwise standing near a wall would have pulled the camera in on the stands behind it.
- **Bloom wraps the 3D pass only.** The HUD is drawn afterwards at full resolution, so text never goes
  through a blur. `postprocess::Begin` returns false when it is off or unavailable and `End` then does
  nothing, so `MatchScene` needs no branch of its own.
- **The bloom chain runs at a quarter of each axis.** It is blurred anyway, so the resolution buys
  nothing; measured, the whole chain costs about 0.1 ms a frame.
- **A render target that fails to allocate has to be detected too, and the detection has to happen
  before the size is cached.** `LoadRenderTexture` answers failure with an id of 0 rather than
  loudly, and `EnsureTargets` recorded `targetWidth`/`targetHeight` regardless — so every later frame
  would have matched the cache, skipped the rebuild and rendered into an invalid framebuffer for the
  rest of the run. `IsRenderTextureValid` on all three targets is the check.
- **The failed size is remembered, and that is not the same as a flag.** Without it the fallback
  retried three `LoadRenderTexture` calls every single frame and logged its warning every single
  frame: measured, **300 warnings in 300 frames**. Keying it on the size means a machine that cannot
  give these targets is asked once, while a resize to something the driver *will* give still gets its
  own attempt. Both post-processing faults were injected and run rather than reasoned about.
- **Detecting missing bloom shaders needs the same two checks the lit shader needed** - raylib answers
  a missing file with its *default* shader, whose id is perfectly valid, so `IsShaderValid` alone
  silently passes. Compare against `rlGetShaderIdDefault()`.
- **A render texture is stored bottom up, so every draw of one flips the source rectangle.** Getting
  it wrong is silent - the image is simply upside down - so it lives in one helper.
- **The render targets are rebuilt when the window size changes, not created at load.** The resolution
  setting can change at any time, and a stale target would stretch the scene.

### Audio
- **Every sound is synthesised at startup; there are no audio files and the cooker has no audio
  step.** One `CueSpec` table in `AudioSystem.cpp` is the whole sound design: a swept oscillator,
  an optional second oscillator a fifth up, some third harmonic, some lowpassed noise, and an
  attack/decay envelope. A thump is a fast downward sweep, a chime an upward one, a crunch is mostly
  noise. Adding a cue is a line in the table and an entry in `AudioCue` - nothing else.
- **The one thing every cue must satisfy is starting and ending at zero**, or it clicks. The envelope
  handles the start and a 4 ms linear fade handles the end; both were measured (see the table above)
  rather than assumed, because a click is the kind of thing that is obvious on hardware and invisible
  in code review.
- **The boost is an `AudioStream` with a callback, not a looping `Sound`.** raylib's sound buffers do
  not loop (`raudio.c` sets `looping = false`), so a held cue would have to be re-triggered every
  time it ran out, which either gaps or clicks at every seam. Generating the roar straight into the
  stream has neither, and it makes intensity a single float. `SetBoost` only moves that float; the
  callback smooths it over about 25 ms, so the game thread and the audio thread never have to agree
  on a frame.
- **The stream is played once at startup and left running at a gain of zero.** Starting and stopping
  it is exactly what would be heard as a click.
- **Nothing in the callback calls into raylib**, which is why the noise comes from a local LCG rather
  than `GetRandomValue`. The same generator builds the cues, and that is deliberate too: drawing from
  raylib's global sequence at startup would shift every random number the effects later take.
- **Three voices per cue, as aliases.** `PlaySound` on a sound that is already playing restarts it,
  so two impacts in quick succession would cut each other off. `LoadSoundAlias` shares the sample
  data and only duplicates the playback state, so the pool costs nothing. Voice 0 owns the data;
  `Unload` frees the aliases first.
- **`ready` gates every entry point, and a missing device is not an error.** `InitAudioDevice`
  followed by `IsAudioDeviceReady` is the check; without one the game runs silent and still exits 0,
  which is what the smoke test needs.
- **The cues are fired from `MatchScene::UpdateEffects`, beside the particle bursts.** Every one of
  them is a *change* that function already watches, so no new event, listener or flag was added to
  the physics or the objects: the goal is the state edge, the ball hit is the jump in the ball's
  speed, the jump is the existing `jumpPending` latch, the countdown ticks come off `match.stateTimer`.
- **A boost pad pickup is read as the tank going up.** Boost only ever increases from a pad, so
  `previousBoostAmount` is the whole detector and `BoostPadObject` needed no latch of its own. It has
  to be seeded in `Initialize`, or the first frame reads a full tank as a pickup - it did, and the
  probe caught it.
- **A wall or car impact is the mirror of the ball-hit test: a sudden *drop* in the player's speed.**
  The threshold is 5 m/s in one frame, and braking cannot reach it - `brakeForce` over the car's mass
  is 39 m/s², which is 0.65 m/s in a 60 Hz frame. It is suppressed during the kickoff, where the
  reset sets the velocity to zero in one step.
- **The volumes are pushed every frame from `App::Run`**, the same treatment `cameraSensitivity`
  gets, so both sliders are heard as they move. Master goes to `SetMasterVolume`, SFX is multiplied
  into each `PlaySound` and into the stream's own volume.
- **`SetBoost(false)` has to be called on every path that stops updating the car** - pause, the F1
  panel and `Shutdown` - because it is the one cue that is held rather than fired. The kickoff freeze
  needs no such call: the car is not stepped, so `boosting` is already false.
- **Only the player's car makes a sound.** There is no spatialisation, so a bot impact across the
  arena would be as loud as one under the camera. The ball hit is the exception, and it is fine
  because it is the ball's own speed that is being watched, whoever hit it.

### Tuning panel and the ImGui backend
- **The sliders point straight at the live fields.** An `Entry` holds a `float *` into the real
  `CarObject` / `BallObject` / `ChaseCamera`, so there is no second copy of the tuning to keep in
  step, and one table drives all three of drawing the panel, writing the config and reading it back.
  Adding a tunable is one `entries.push_back` and nothing else.
- **Every car entry carries the bot's matching field as a `mirror`.** The two cars are separate
  objects with their own copies of every handling number, so without it, tuning the player's
  acceleration would quietly leave the bot faster than the player.
- **Three values are not a field of anything** — gravity and the two pad refill amounts — so the
  panel owns them and an `Apply` step pushes them into the physics system and across the pad list.
  `Apply` also calls the existing `ApplyTuning` on the ball and both cars, because Jolt caches
  restitution, friction and damping on the body rather than reading the object every step.
- **`BoostPadObject::fullRefill` was added so a refill slider knows which pads it means.** The two
  kinds were only distinguishable by their amount, which is exactly the thing being edited.
- **The config is loaded at `MatchScene::Initialize`, not at startup.** The values live on objects
  that are built per match, so that is the only point where there is something to load them onto.
  Practically it is better as well: a rematch picks up whatever was last saved.
- **The config is parsed by splitting on `=`, not by scanning a token.** Section names contain
  spaces (`Car drive.maxSpeed`), and the first version used `sscanf("%[^= ] = %f")`, which stops at
  the space, read half a key and silently dropped every car line while the ball lines worked. The
  round-trip check is what caught it.
- **rlgl batches vertices, but `rlScissor` is immediate GL state, so the batch has to be flushed
  before every scissor change.** Without the flush, each ImGui command's geometry is drawn under
  whatever rectangle a *later* command set. The symptom was window title bars silently missing —
  clipped away by a scissor belonging to a slider further down the window. This is the single
  sharpest edge in the backend.
- **ImGui draws through rlgl's immediate-mode batch rather than a shader and buffers of its own.**
  A panel is a few thousand 2D triangles, which is exactly what that batch is for, and it keeps the
  backend free of GL calls. rlgl splits the batch at triangle boundaries by itself, so a command of
  any length is safe between one `rlBegin` and `rlEnd`.
- **ImGui 1.92 hands textures over on demand.** The backend sets
  `ImGuiBackendFlags_RendererHasTextures` and services `WantCreate` / `WantUpdates` / `WantDestroy`
  each frame; the older single-atlas `GetTexDataAsRGBA32` path is not what this version expects.
  Updates re-upload the whole texture rather than the requested sub-rectangle, because ImGui's
  `Pixels` array is the whole texture and a rectangle out of it is not contiguous.
- **`io.IniFilename` is null.** ImGui would otherwise write `imgui.ini` into whatever directory the
  game was launched from.
- **The whole of `ImGuiRaylib.cpp` and `TuningPanel.cpp` is inside `#ifdef GAME_DEV_TOOLS`**, so
  Release links neither and the panel's strings are absent from the binary — checked, not assumed.

### Bot opponent
- **The bot aims by choosing where to drive, not by deciding when to hit.** Its target is a point
  `approachOffset` behind the ball on the far side from the goal it is attacking, so simply driving
  through that point *is* the shot. There is no separate shooting decision, no timing and no
  prediction of contact - which is also why it is beatable.
- **`CarController::Poll` gained a `deltaTime`.** The bot needs timers (stuck detection) and `Poll`
  runs once per fixed step, so the step is the only honest source of time available to it -
  `GetFrameTime()` would be the frame, which is a different clock. `PlayerController` ignores it.
- **The bot's target is clamped to the flat floor** (`fieldHalfWidth`/`fieldHalfLength`, taken from
  `ArenaObject::FlatHalfWidth/FlatHalfLength`). Unclamped, a ball rolling towards the bot's own net
  puts the approach point *inside the goal recess*: measured, the bot followed it in, wedged itself
  in the goal mouth beside the opening and never came out - 94% of a two minute match spent stuck.
  The clamp also keeps it off the ramps, which it has no reason to climb.
- **Backing out cannot free a wedged car, so a five second jam takes the reset.** The unstick reverses
  for 0.8 s, which handles the ordinary case of driving into a wall; a car actually jammed in geometry
  reverses into the same wedge forever. `stuckResetTime` escalates to `CarInput::reset`, the same
  reset the player has on **R**, which teleports the bot to its own spawn. It is the only guarantee
  behind "the bot never gets stuck against a wall for long", and it fired twice in five minutes.
- **The jam timer runs through the unstick attempts, the stuck timer does not.** They measure
  different things: one is "how long since it last moved at all", the other is "how long since the
  last unstick attempt". Resetting the jam timer on each attempt would let a wedged car alternate
  forever without ever reaching the escalation.
- **A stationary car cannot steer, which is why the recovery reverses rather than turns.** Steering
  authority ramps in with speed (`steerSpeedFloor`), so a bot nosed into a wall at 0 m/s has no yaw
  rate available at all. Reversing gives it both the speed to turn and somewhere to turn into.
- **Steering is `atan2` of the heading error, and the sign works out to a direct steer.** Positive yaw
  turns left and `CarObject` negates the steer, so a positive error in the XZ plane is a positive
  steer. Reversing mirrors the mapping in `CarObject`, so both the unstick and the swing-round branch
  invert their steer to compensate.
- **The bot drives `SportsCar2`, the one cooked car the picker never offers.** It can therefore never
  turn up in the same model as the player's pick, without any logic to exclude one.
- **The bot is not given `settings->playerCarModel`.** That field is the player's choice; the bot's
  model is fixed in `MatchScene::Initialize`.
- **Boost is spent only on long, straight approaches** - grounded, more than `boostMinDistance` away,
  within `boostMaxAngle` of the target, and never below `boostReserve`. Boosting through a turn just
  makes it overshoot the ball.
- **It never jumps.** Nothing in the milestone needs it, and a bot that flips has to recover from
  landing on its roof. It is the single biggest thing to add if it is ever wanted to be sharper.
- **It gives up a chase it has already lost, and that is what stopped it being a kickoff-only bot.**
  Before Milestone 6.4 it drove at the ball unconditionally, including when the ball was past it in
  its own half — a chase it cannot win, run while its own net stands open. Measured, *every* goal it
  ever scored came within 6 s of a kickoff and 199 s of contested open play produced three shots and
  no goals. The recovery is one branch and it is a recovery, not a mode: the moment it is goal-side
  again it attacks.
- **`recoverMinRange` (8 m) is what keeps the recovery from being cowardice.** Inside it the bot plays
  the ball whatever the geometry says, because it is close enough to contest and turning away is the
  worse mistake. At 0 it never contests and the goal difference falls to zero; at 14 m it abandons
  balls it could have reached and produced a 5.11 s stall and a reset — the only reset in the whole
  sweep.
- **A hysteresis margin on "goal side" was built, measured and removed.** The idea was to stop it
  flip-flopping when the car and the ball are level. Every value tried made it worse (at 3 m the goal
  difference went to -2.9 and open-play goals to 0.7), and its best value was zero, so it is not
  shipped as a knob that does nothing.
- **The bot must never be tuned against a mirror of itself.** Two identical agents are chaotic: the
  same build measured 6-9, 11-6 and 47-52 across runs differing only in rounding. Every number in the
  6.4 section comes from six decorrelated matches against a *fixed* opponent — the previous bot — with
  the kickoff nudged sideways per run. Use that shape of experiment for any future bot work.
- **The provoked wall test gets slower with the recovery, and that is the recovery working.** Nose
  into a side wall with the ball deep in its own half now takes 1.95 s to get back up to speed rather
  than 0.39 s, because the bot is turning towards its own goal instead of towards the ball. With the
  ball up the pitch the same wedge still frees in 0.37 s, and in real matches the worst stall actually
  *fell* from 1.58 s to 1.33 s. It is a different target, not a stall.

### Gamepad
- **The pad is merged into the keyboard's `CarInput`, never chosen between.** Whichever source is
  pushing hardest wins each analogue field and the booleans are or-ed, so both devices stay live at
  once and there is no mode to be in or to switch. With no pad connected the merge block reads zeros
  and changes nothing, which is why keyboard play is untouched by all of this.
- **Air control comes off the left stick, not off the throttle.** The keyboard derives pitch from W/S
  because they are the same keys; on a pad the throttle is a trigger, and a trigger cannot pitch the
  nose up. The inversion is kept: pushing the stick forward puts the nose down, as W does.
- **Everything is sampled once per frame by `gamepad::Update`, and every query is a read of what it
  stored.** Edges — a button going down, a stick leaving the centre — can only be found by comparing
  against the previous frame, and a settings row that asks `MenuRight()` while another row asks it
  too must get the same answer. `PlayerController::Poll` runs at 120 Hz, twice per frame, and reads
  the same stored values both times.
- **The stick is a set of four buttons to a menu, with no auto-repeat.** It counts as pressed when it
  leaves the centre past 0.55 and re-arms when it comes back, so holding it does not run down a list.
- **The dead zone is radial, not per axis.** A per-axis zone squares off the circle the stick moves
  in, so a diagonal push clips to the corner and reads as full deflection on both axes.
- **Keyboard activity is detected with `IsKeyDown` over the game's own keys, never `GetKeyPressed`.**
  That call pops raylib's queue, and the title screen is reading the same queue to leave itself
  (Milestone 17): draining it here would strand the player on the title card.

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

## Flip physics, 2026-08-18 (branch `fix/jump-physic`)

Reported: the somersault done with jump, W, jump turns over in slow motion and the car floats far too
long before it lands. Both were true, and the float was a bug rather than a tuning value — the car was
being launched by the floor it was rotating through (see the flip bullets under Decisions made).

Two changes in `CarObject`, one tuning and one behavioural:

- `flipSpin` 9.0 -> 12.0 rad/s and `flipDuration` 0.70 -> 0.52 s. The product is still one whole turn,
  taken a third quicker. Both are still on the F1 tuning panel.
- A flip is now ballistic while the floor is in reach, is held before the grounded gate rather than
  inside it, and takes the jump lockout when it fires.

Measured by a temporary harness that drove the real `MatchScene` with a scripted controller and
stepped physics directly (removed afterwards). The car runs up to 14.9 m/s, jumps, and flips after
the delay in the first column; a full turn is 360 deg.

| Flip | Rotation, before -> after | Peak, before -> after | Airborne, before -> after | Speed 0.5 s after landing |
|---|---|---|---|---|
| Forward, 0.05 s after the jump | cancelled on the step it fired -> **308 deg in 0.52 s** | 6.69 m -> **1.91 m** | 2.78 s -> **1.11 s** | 1.1 -> 0.3 m/s |
| Forward, 0.10 s | 351 deg in 0.70 s -> **320 deg in 0.52 s** | 5.93 m -> **1.83 m** | 2.16 s -> **1.02 s** | 0.2 -> 3.8 m/s |
| Forward, 0.20 s | 353 -> **338 deg** | 5.03 m -> **1.60 m** | 1.97 s -> **0.88 s** | 3.9 -> 4.4 m/s |
| Forward, 0.30 s | 355 -> **344 deg** | 3.22 m -> **1.39 m** | 1.52 s -> **0.77 s** | 6.0 -> 6.7 m/s |
| Forward, 0.45 s (top of the jump) | 356 -> **338 deg** | 2.62 m -> **1.46 m** | 1.32 s -> **0.70 s** | 6.3 -> 6.3 m/s |
| Side | 352 -> **346 deg** | 1.23 m -> **1.41 m** | 0.93 s -> **0.92 s** | 5.2 -> 5.4 m/s |
| Backward | 348 -> **308 deg** | 5.10 m -> **1.76 m** | 2.00 s -> **1.02 s** | 0.0 -> 3.8 m/s |
| Diagonal | 347 -> **302 deg** | 6.68 m -> **1.75 m** | 2.31 s -> **1.12 s** | 1.4 -> 0.0 m/s |

Every flip lands upright, before and after. The side flip is the one that barely moves, and that is
the tell: it rolls about the car's 1.7 m width rather than pitching about its 3.2 m length, so it was
never scraping the floor and was never being launched.

Nothing outside the flip moved:

| Check | Result |
|---|---|
| Single jump | peaks 1.49 m, 1.09 s airborne — unchanged |
| Double jump | peaks 3.85 m, 1.62 s airborne — unchanged |
| Boosted aerial flip at 8.1 m, climbing 16.0 m/s | climbs to 18.3 m still gaining, 17.2 m/s against 12.5 m/s before — the cap does not touch it |
| Debug / Development / Release | build with no game-code warnings |
| `--smoke-test 60 --screenshot` in all three | exits 0, screenshot non-blank, no errors in the log |

### A taller jump and the flip's hit on the ball, same day

Asked for after the two fixes above: a slightly bigger jump, and a flip that really shifts the ball.

- `jumpImpulse` 1000 -> 1100 N s. Nothing else moved; the double jump gets the extra height for free
  because it stacks on the first.
- `CarObject::ball`, set by `MatchScene`, plus `flipHitImpulse` (450 N s, on the F1 panel). A flip
  that connects adds it to the ball on top of the collision.

| Measurement | Before | After |
|---|---|---|
| Single jump | 1.49 m, 1.09 s airborne | **1.80 m, 1.20 s** |
| Double jump | 3.85 m, 1.62 s | **4.40 m, 1.72 s** |
| Forward flip at the top of a jump: speed 0.5 s after landing | 6.3 m/s | **8.2 m/s** — the extra height is extra clearance, so the nose scrapes less |
| Every flip still lands upright, in 0.72-1.18 s | yes | yes |

The hit, measured by driving at a resting ball and either going through it or flipping into it. "Ball
leaves at" is the speed out of the contact, before `BallObject::maxSpeed` (55 m/s) clamps it on the
next step:

| Shot | Car at contact | Ball leaves at |
|---|---|---|
| Drive through, no flip | 29.9 m/s | 42.2 m/s |
| Flip from 11 m out, bonus off | 13.3 m/s | 28.4 m/s |
| Flip from 11 m out, bonus on | 13.3 m/s | **34.2 m/s** |
| Flip from 6 m out, bonus off | 30.2 m/s | 57.1 m/s (clamped to 55) |
| Flip from 6 m out, bonus on | 30.2 m/s | 64.9 m/s (clamped to 55) |

So the bonus is worth about a fifth of the shot in the middle of the range, and nothing at all at the
top of it, where the ball is already on its speed cap — which is the cap doing its job, not the bonus
failing. The juice follows for free: `MatchScene` sizes the burst, the screen punch and the thump
from the ball's speed jump, so a bigger jump is a bigger hit on screen and in the speakers with no
extra code.

### The camera followed the car round the flip

Reported straight after: the camera swings from behind the car to the front during a flip. Same root
cause, one step further on — `ChaseCamera::FollowDirection` takes the car's nose, flattens it to the
pitch and sits opposite it, and half way through a flip the car is upside down with its nose pointing
back down the field. The camera went there.

Fixed in `ChaseCamera.cpp` only: the flattened nose is rejected while the car has rolled away from
the up the camera frames against, exactly as it already was when the nose is too vertical to mean
anything, and the velocity blend now tests against the direction in use rather than the car's own
forward. Measured by a second temporary harness that ran the **real** `ChaseCamera` over scripted
driving (removed afterwards). Because the fix touches no physics, the same flip could be replayed
against both builds and the two camera tracks diffed directly.

| Check | Old camera | New camera |
|---|---|---|
| Chase cam, forward flip: worst the camera sat off where the mode wants it | **180.0 deg**, 0.38 s in | **0.0 deg** |
| Chase cam, forward flip: mean over the 1.5 s after the flip | 22.6 deg | **0.0 deg** |
| Chase cam, side flip | 10.2 deg | 14.0 deg |
| Ball cam, ball 4 m to the side: swing a flip adds over the same run without one | 14.7 deg | **5.3 deg** |
| Ball cam, ball 9 m away | 8.3 deg | 8.9 deg |
| Ball cam, ball 20 m away | 16.3 deg | 16.3 deg — the nose has no say past `ballCamNearRange` (7 m) |
| Boosting up the side wall to 20.4 m | car within 9.7 deg of centre, eye never behind a surface | identical |

The chase camera is the one that was broken outright; ball cam only leaks the nose in within
`ballCamNearRange`, so it swung less, and what is left there after the fix is the **overtake
handover**, not the flip: a car that flips past the ball genuinely puts the ball behind it, and ball
cam is supposed to swing round to the other side. That swing is limited by `ballCamTurnRate` and
takes about a second at 3 m — it is 6.2's design, not a bug, and it is worth knowing about before
anyone tries to "fix" it a second time.

## Known deviations / things to be aware of

- **A flip fired in the first tenth of a second after the jump scrubs the car's speed.** The body is
  3.2 m long and rotates about a centre of mass 0.28 m below the box centre, so a pitch flip needs
  about 1.9 m of clear air under the car and a jump only ever lifts it 1.84 m — a low forward flip
  always drags its nose along the floor. Measured, a 25 m/s dodge lands at about 5 m/s. It is not new
  (before the 2026-08-18 fix it landed at 0.2 m/s) and it is legible on screen, but if it ever needs
  fixing, the fix is the car's proportions or the jump height, not another velocity rule: holding the
  dodge speed, and dropping the flip's friction, were both tried and both ended in 7-9 m launches.
- **Assets folder name.** CLAUDE.md says `Game/Assets/Cars-pack/`; on disk it is
  `Game/Assets/Cars-Park/`.
- **The shadows are contact shadows, not cast shadows.** Each car and the ball drops a disc onto the
  surface below it; nothing casts onto anything else, so a car under a light rig or beside another
  car is unshadowed. It is deliberate (see the effects decisions) and it is the thing to replace if
  the arena ever needs real shading. Do **not** bake shading into vertex colours as a substitute —
  the lit shader would then double-darken the model.
- **The OBJ+MTL copies in `Game/Assets/Cars-Park/OBJ/` are now unused by the build.** They are worth
  keeping as the reference the FBX reader is validated against
  (`Tools/FbxReader.py` output matched them exactly), but nothing loads them at runtime.
- **`R` resets the car only.** It briefly also re-centred the ball while the field had no walls;
  now that the arena is closed the ball cannot be lost, so that was dropped again.
- **`GameSettings::matchDurationMinutes` is now read** (at `MatchScene::Initialize`), so changing it
  mid-match has no effect until the next match. That is the intended behaviour, not a bug.
- **Full time offers a rematch and the main menu, and nothing else.** There are no post-match stats.
  While the pause menu is open the full time screen is not drawn at all, so the way out of a finished
  match is either its own two buttons or the pause menu, never both at once.
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
  `reset`. The bot uses `throttle`, `steer`, `boost` and `reset`; the air axes and `jump` are the
  player's alone so far.
- **CLAUDE.md 2.3 lists "boost pad hints" as a HUD element and there are none.** The pads communicate
  their own state by glowing in the world; the screen-space hint was removed on request. See the HUD
  decisions above before adding one back.
- **The boost flame is drawn from `MatchScene::DrawEffects`, not from `CarObject::Draw`.** Every
  effect is drawn in one place, after the objects and before the glass, so the blending order is
  decided once rather than per object.
- **Nothing fires an effect while the match is frozen.** `UpdateEffects` runs from `MatchScene::Update`
  after the pause and F1 returns, so a paused match holds its particles exactly where they were.
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
- **F1 freezes the match, which is at odds with "changing a slider immediately changes gameplay
  feel".** CLAUDE.md's Milestone 13 asks for both: the verify line wants live feel, and the line
  added later says the game is paused while the panel is open. The pause is what is implemented,
  since it is the more specific and more recent instruction. It still works in practice because the
  sliders write to the live objects as they move, so closing F1 lets the change be felt at once —
  but there is no way to feel a slider *while dragging it*. Worth revisiting if it gets in the way.
- **F1 currently opens the tuning panel, and EDITOR.md's Arena Editor wants the same key.** That
  editor is its own milestone and is not built. When it lands, the two belong behind one F1 overlay
  with the panel as one window in it, not two keys.
- **The tuning panel does not expose the audio either.** The cue table is a table of constants, so
  every sound is a rebuild away. It is the obvious second addition to the panel after the bot.
- **The three PNGs in `Game/Assets/Textures/` are cooked but nothing loads them.** They are the arena
  textures from the reverted texture experiment (commit `a4ada93`); the cooker takes every PNG under
  `Game/Assets/`, so they now ship as `.evtex` in every build. Harmless, about 900 KB, and they are
  the obvious test material if a surface is ever textured.
- **There is no music and no engine sound.** CLAUDE.md's Milestone 15 asks for a specific list of
  cues and both are outside it; the boost roar is the only continuous sound in the game.
- **The tuning panel does not expose the bot.** CLAUDE.md lists exactly what the panel should carry
  and `BotController` is not on it. Its fields are the hardest thing in the project to judge by
  reading, so they are the obvious first addition if the panel is ever extended.
- **`Tuning.cfg` is per build configuration**, because it sits next to the executable in
  `Build/<Config>/`. Debug and Development do not share one. That is deliberate - it keeps a wild
  experiment in one build - but it does mean saving in Debug does not change Development.
- **The bot can teleport.** A five second jam sends it back to its own spawn (see the bot decisions
  above). It is deliberate and it is the only hard guarantee that it never sits stuck, but it is a
  thing a player cannot do, so it is worth knowing before anyone reports it as a bug.
- **The goal recess had no floor until Milestone 12.** The arena floor slab stops at the goal line, so
  anything that followed the ball into a net fell out of the world for good — the player included,
  not just the bot. `GoalObject` now builds its own floor, flush with the arena's at y = 0.
- **`GameSettings::botEnabled` is read once, at `MatchScene::Initialize`.** Toggling it in the pause
  menu therefore does nothing until the next match, exactly like `matchDurationMinutes`.
- **Every setting is consumed now.** The master and SFX volumes were the last two that were stored
  and unread; Milestone 15 pushes them into the audio system every frame.
- **Settings live for the session only.** Nothing is written to disk; Milestone 13 introduces the
  config file, and that is the natural place to persist them. The picked car is in the same struct,
  so it is forgotten on exit along with everything else.
- **`SportsCar2` is deliberately not in the picker** (CLAUDE.md Milestone 08 says to use six of the
  seven). It is still cooked and still loadable by name.
- **All six previews are painted blue,** because the player is always on the blue team. If teams ever
  become selectable, `SetPaintColor` in `CarSelectScene::Initialize` is the one place to change.

## Next steps — Milestone 22 is open, the rest of the list is finished

**Milestone 22 — Gamepad Support** was added to CLAUDE.md section 5 on 2026-08-18 (branch
`feat/gamepad`) and is spec only: no code has been written. It is blocked on its own **section 22.1**,
which is a reserved placeholder — the Rocket League button layout table has still to be supplied, and
`GamepadInput.cpp` is meant to match it exactly, so wait for it before mapping any button.

Milestones 01-21 in CLAUDE.md section 5 are done, and section 6 is being worked through a subsection at
a time. **6.1 through 6.4 are done**; 6.5 is next, and their README items should be
treated the way 6.1's, 6.2's and 6.3's were - as claims to be measured rather than as work already
finished. Two of 6.1's three ticked items turned out not to be true when a harness was pointed at
them, both of 6.2's did, and so did all three of 6.3's.

Beyond section 6, the four worth picking up first, in order of how much they would be felt:

1. **The bot never jumps** (see the bot decisions). It is the single biggest thing standing between
   the current opponent and one that feels like a player.
2. **Nothing is written to disk except `Tuning.cfg`.** Settings, the picked car and the volumes are
   all forgotten on exit; that config file is the natural place to persist them, and it already has a
   loader.
3. **The tuning panel covers neither the bot nor the audio**, which are the two hardest parts of the
   game to judge by reading numbers.
4. **The arena is a rounded rectangle, not an octagon.** Rocket League's 45 degree corner walls are
   arena-shape work that `AddFillet` and `AddCorner` were built general enough to take.

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

Four things Milestone 15 learnt about the shim, all of which cost a run:
- **Adding `-include` to the Makefile does not rebuild anything.** The objects depend on the sources,
  not on the flags, so only the files edited afterwards pick the shim up — and a shim that reaches
  `UserInterface.cpp` but not `PlayerController.cpp` looks exactly like a bug in the game. Delete the
  configuration's game objects (`find Build/Intermediate/<Config> -name '*.o' -not -path
  '*ThirdParty*' -delete`) after adding it.
- **`IsKeyPressed` must stay true for every query in the frame, not just the first.** `MenuList::Item`
  asks for Enter once per row and only the selected row acts on it, so a shim that consumes the press
  on first ask silently swallows the activation. Drive it from a per-frame tick called out of
  `App::Run` rather than from `GetTime()`, which moves within a frame.
- **`--smoke-test` starts in the match**, so it cannot reach the menus at all. Run the game without it
  and script the way out (pause → Exit game) to test menu input.
- **There is a live mouse pointer on `DISPLAY=:1` and it moves.** It hijacks `MenuList::selected`
  through the hover path, so a scripted keyboard walk through a menu is not reproducible. Script
  gameplay, and read menu behaviour from what the probe logs rather than from where the script
  thought it was.
