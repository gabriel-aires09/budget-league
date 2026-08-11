# Budget League

## About
A Rocket League-like game with low-poly graphics. The main idea behind the project was to create a "demake" of Rocket League with visuals inspired by games from the early 90s. I tried to recreate some of its core mechanics, such as boosting and driving up walls and ramps. There are still some bugs, and the game is still under development.

If everything goes well, I want to implement a split screen two-player mode  (2v2 - 1v1) and online multiplayer as a proof of concept. The arena, ball, goals, boost pads, UI and every sound effect are procedural — the sound cues are synthesised at startup from a table. The only imported art is the Quartenius Cars Pack

## Stack

- **Platform:** Linux
- **Language:** C++ (GCC / Makefile), with Python for the build tools
- **Graphics:** [raylib](https://github.com/raysan5/raylib)
- **Physics:** [Jolt](https://github.com/jrouwe/JoltPhysics)
- **Math:** [glm](https://github.com/g-truc/glm)
- **Debug/tuning UI:** [Dear ImGui](https://github.com/ocornut/imgui)
- **Assets:** cooked by `Tools/AssetCooker.py` as a build event

## Controls

### Driving

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
