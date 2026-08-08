# itch.io page copy — Budget League

Copy-paste source for the itch.io project page. Keep it in sync with `README.md`.

---

## Page metadata

| Field | Value |
|---|---|
| **Title** | Budget League |
| **Short description / tagline** | Rocket-powered cars playing football in a full-size arena. Drive the walls, fly the ceiling, score. |
| **Classification** | Game |
| **Kind of project** | Downloadable |
| **Release status** | Released |
| **Pricing** | Free (or "name your own price") |
| **Platforms** | Linux, Windows |
| **Genre** | Sports |
| **Tags** | car-soccer, arcade, sports, 3d, low-poly, singleplayer, raylib, physics, cars, football |
| **Input** | Keyboard |
| **Player count** | Singleplayer |
| **Average session** | A few minutes |

---

## Page body

### Budget League

**Rocket-powered cars playing football.** Pick a car, kick off in a full-size glass arena, and put a
heavy, bouncy ball across the other team's goal line before the clock runs out.

It is an arcade game before it is a simulation. The car is a single rigid body pushed around by
arcade forces rather than a suspension sim, so acceleration is brutal, the back end steps out when
you ask it to, and a tumble rights itself instead of ending your run. Boost, jump, double-jump, flip,
and steer yourself in mid-air on all three axes.

**Every edge of the arena is ramped.** Drive into the side of the pitch and you carry straight up the
wall — with enough boost in the tank, all the way across the ceiling and down the other side. The car
measures the surface under it, not the world, so the same throttle, steering and grip work upside
down.

### Features

- **A full-size pitch.** 76.81 m sideline to sideline, 102.41 m goal to goal, 20.73 m of headroom,
  built to the Rocket League reference measurements.
- **Six cars to pick from,** low-poly and painted in your team colour on a 2 × 3 selection grid.
- **Boost that matters.** A 0–100 meter, about three seconds of it flat out, refilled by 18 pads
  spread across the field — small ones top you up, the four corner pads fill you completely.
- **Aerials.** Jump, jump again, flip in whatever direction you are holding, and hit the ball out of
  the air.
- **A bot that actually plays.** It drives at the ball from the side its shot is coming from, boosts
  down the long approaches, stops chasing a ball that is already behind it and recovers goal-side
  instead. It scores in open play and it can be beaten. Switch it off in Settings for solo practice
  with both goals still scoring.
- **Two cameras.** A chase cam that stays out of the walls by ray-casting the real collision
  geometry, and a ball cam on **C** that keeps the ball and your car lined up in the same frame.
- **Juice.** Boost flames that grow with the throttle, ember trails, contact shadows, goal
  explosions, a screen kick scaled to how hard you hit the ball, and optional bloom.
- **A shuffled soundtrack** running through menus and matches alike, with its own volume slider.
- Full-time screen with an instant rematch, so the next match is one key away.

### Controls

| Input | Action |
|---|---|
| **W / S** or **↑ / ↓** | Accelerate / brake and reverse |
| **A / D** or **← / →** | Steer |
| **Space** | Jump — press again in the air to double jump, or flip if you are holding a direction |
| **Shift** (hold) | Boost |
| **WASD** (airborne) | Pitch and yaw |
| **Q / E** (airborne) | Air roll |
| **C** | Chase cam / ball cam |
| **R** | Reset your car |
| **Esc** or **P** | Pause |

Menus take the arrow keys or WASD, Enter or Space to confirm, Esc to go back — or just use the mouse.

### How to run

**Linux** — unzip, then run the executable. Keep the `assets` folder next to it.

```sh
chmod +x ArcadeCarSoccer
./ArcadeCarSoccer
```

Needs a working OpenGL driver and X11 — nothing else is installed.

**Windows** — unzip and run `ArcadeCarSoccer.exe`, with its `assets` folder beside it. No runtime to
install; everything is linked statically.

The game opens fullscreen at your monitor's own resolution. Settings turns it back into a 1280×720
window, and holds the camera sensitivity, match length, bot toggle, post-processing and the three
volume sliders.

### Built with

Native C++ with [raylib](https://github.com/raysan5/raylib) for graphics,
[Jolt Physics](https://github.com/jrouwe/JoltPhysics) for the simulation,
[glm](https://github.com/g-truc/glm) for maths and
[Dear ImGui](https://github.com/ocornut/imgui) for the in-engine tuning panels. No engine.

The arena, ball, goals, boost pads, UI and every sound effect are procedural — the sound cues are
synthesised at startup from a table, and there is not a single `.wav` in the build. The only imported
art is the car pack.

### Credits

Car models from the Cars-Park low-poly pack. Everything else built for this project.

---

## Notes for whoever uploads this

- **macOS is written into the `Makefile` but has never been built** — there is no Mac in this
  project's environment. Do **not** tick the macOS platform box or upload a Mac build until one has
  actually been produced and smoke-tested on hardware.
- Upload the **Release** configuration only, and always zip the executable **together with its
  `assets/` folder** — the game falls back to box stand-ins and silence without it.
- Suggested screenshots: the title screen, the main-menu car showcase, the car picker grid, a boosted
  wall drive, and a goal with the celebration banner up.
