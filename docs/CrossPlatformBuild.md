# Cross-Platform Builds — Windows, Linux and macOS

Reference material for **Milestone 21** (CLAUDE.md section 5). It lists every change needed
to produce a separate runnable executable on each of the three platforms.

> **Scope.** CLAUDE.md still declares Linux as the platform (project header, and §1.1
> "**Platform:** Linux"). Those lines are deliberately unchanged — Milestone 21 is written as
> an explicit, scoped exception to them. Linux remains the platform the game is developed and
> verified on.

> **What is verified here.** The Makefile changes in section 2 are **implemented**. Linux and
> the cross-compiled Windows executable are both backed by working builds — the Windows one
> was cross-compiled with `x86_64-w64-mingw32` and its smoke test run under Wine (section 4).
> **The macOS instructions have not been compiled or run** — there is no Mac in this project's
> environment. Treat that column as written-but-unverified until someone builds it.

---

## 1. No source code changes are needed

This is the important finding, and it is why the port is a build-system job rather than a
port in the usual sense. Verified by inspection of `Game/Source/`:

| Check | Result |
|---|---|
| `_WIN32`, `__APPLE__`, `__linux__`, `WIN32` | no matches |
| `unistd.h`, `dirent.h`, `sys/*`, `pthread.h`, `dlfcn.h` | no matches |
| X11 or any windowing/GL header used directly | no matches |
| `system()`, `popen`, `fork`, `exec*` | no matches |

Every filesystem touch already goes through raylib, which implements each of these per
platform:

| Call | Where |
|---|---|
| `GetApplicationDirectory` | `StaticModelAsset.cpp:11` (`assets::Path`), `TuningPanel.cpp:19` |
| `LoadFileData` | `StaticModelAsset.cpp:55`, `TextureAsset.cpp:93` |
| `LoadDirectoryFilesEx` / `UnloadDirectoryFiles` | `AudioSystem.cpp:229` (the music playlist scan) |
| `TakeScreenshot` | `App.cpp:126` |

Jolt, glm and Dear ImGui are cross-platform, and the ImGui backend in `ImGuiRaylib.cpp` draws
through rlgl rather than a platform backend, so it carries no OS code either.

`Tools/AssetCooker.py` and `Tools/FbxReader.py` use `pathlib`, Pillow and numpy — already
portable. No change.

**Non-issue, for the record:** the playlist scan filters on `".mp3"`, but raylib's
`IsFileExtension` lowercases the extension before comparing (`raylib/src/rcore.c:2351`), so a
file named `.MP3` is matched correctly on a case-insensitive filesystem.

---

## 2. Makefile changes

Six touch points, all now in `Makefile`. Line numbers below are against commit `94afbc9`,
before the change.

### Platform detection

There was none. Added near the top, before the build-mode block:

```make
ifeq ($(OS),Windows_NT)
    TARGET_OS ?= Windows
else
    TARGET_OS ?= $(shell uname -s)   # Linux | Darwin
endif
ARCH := $(shell uname -m)            # x86_64 | arm64 | aarch64
```

`TARGET_OS` is a `?=` rather than a `:=` precisely so a cross-compile can override it from the
command line; everything below keys off it and off `ARCH`.

### The six changed lines

| # | Line | Now | Change to | Why |
|---|---|---|---|---|
| 1 | 33 | `SIMD_FLAGS := -msse4.2 -mpopcnt` | empty when `ARCH` is `arm64`/`aarch64` | **These are x86-only and will not compile on Apple Silicon.** Jolt autodetects ARM NEON. The Makefile's own comment already warns this value must be identical for Jolt and every file including it, so it must stay a single variable. |
| 2 | 59 | `LDLIBS := $(RAYLIB_LIB) -lGL -lm -lpthread -ldl -lrt -lX11` | per platform, see below | `-lGL`, `-lrt` and `-lX11` do not exist on Windows or macOS. |
| 3 | 19 | `TARGET := $(BUILD_DIR)/ArcadeCarSoccer` | append `.exe` on Windows | Windows will not run an extensionless binary. |
| 4 | 116 | `@cd $(BUILD_DIR) && ./ArcadeCarSoccer` | append `.exe` on Windows | Same, for `make run`. |
| 5 | 28 | `$(ROOT)/.venv/bin/python` | `$(ROOT)/.venv/Scripts/python.exe` on Windows | Python venvs use `Scripts/` on Windows. Affects the `cook` build event only. |
| 6 | 46 | `LDFLAGS_MODE := -s` (Release) | drop it on macOS, use `-Wl,-x` or a separate `strip` | Apple's linker rejects `-s`. |

### Link lines

Taken from raylib 6.0's own `Makefile` (lines 631–656), which is the authoritative source for
what raylib itself needs:

```make
ifeq ($(TARGET_OS),Windows)
    LDLIBS := $(RAYLIB_LIB) -static -static-libgcc -static-libstdc++ \
              -lopengl32 -lgdi32 -lwinmm
else ifeq ($(TARGET_OS),Darwin)
    LDLIBS := $(RAYLIB_LIB) -framework OpenGL -framework Cocoa -framework IOKit \
                            -framework CoreAudio -framework CoreVideo
else
    LDLIBS := $(RAYLIB_LIB) -lGL -lm -lpthread -ldl -lrt -lX11
endif
```

Note macOS needs **no** `AudioToolbox` — raylib's own list does not include it.

`-static-libstdc++` and `-static` are not in raylib's list and are added here: the game is C++
built with MinGW, so without them the executable would need `libstdc++-6.dll`,
`libgcc_s_seh-1.dll` and `libwinpthread-1.dll` copied beside it. With them, `objdump -p` shows
only system DLLs (`KERNEL32`, `USER32`, `GDI32`, `SHELL32`, `WINMM`, `OPENGL32` and the UCRT).

### The `$(RAYLIB_LIB)` rule

This *does* need an edit for cross-compiling, contrary to the first draft of this document.
raylib detects `PLATFORM_OS` from `uname`, which on a Linux host cross-compiling for Windows
gives the wrong answer, and it would otherwise build with the host compiler:

```make
	@$(MAKE) -C $(RAYLIB_DIR)/src \
		PLATFORM=PLATFORM_DESKTOP PLATFORM_OS=$(RAYLIB_OS) CC=$(CC) AR=$(AR) \
		RAYLIB_LIBTYPE=STATIC RAYLIB_BUILD_MODE=RELEASE \
		RAYLIB_RELEASE_PATH=$(RAYLIB_OUT)
```

where `RAYLIB_OS` maps `TARGET_OS` onto raylib's own spelling (`WINDOWS` / `OSX` / `LINUX`).

### Output directories

`Build/<Config>/` and `Build/Intermediate/<Config>/` are shared by every target, so a native
object and a cross object have the same path. Linux keeps the historical layout and everything
else nests one level deeper:

```make
ifeq ($(TARGET_OS),Linux)
    OUT_ROOT := $(ROOT)/Build
else
    OUT_ROOT := $(ROOT)/Build/$(TARGET_OS)
endif
```

so the Windows build lands in `Build/Windows/Release/` and the Linux paths in README.md stay
correct. `RAYLIB_OUT` moves under `OUT_ROOT` for the same reason.

---

## 3. Path A — native build on each OS (recommended)

Build Linux on Linux, Windows on Windows, macOS on macOS. This is the fully supported route
and the `Build/<Config>/` layout needs no change.

| Platform | Toolchain | Notes |
|---|---|---|
| Linux | g++, make, Python 3, OpenGL + X11 dev packages | Unchanged; this is the current build. |
| Windows | **MSYS2 / MinGW-w64** (`mingw-w64-x86_64-gcc`, `make`) or Git Bash | The Makefile uses `find`, `mkdir -p` and `rm -rf`, so **`cmd.exe` and PowerShell will not work.** |
| macOS | Xcode Command Line Tools (`xcode-select --install`), Python 3 | Ships GNU make 3.81, which is enough. Apple Silicon needs change #1 above. |

**`libraylib.a` is not portable between platforms.** Each target now archives its own under
`$(OUT_ROOT)/Intermediate/ThirdParty/`, so the three do not overwrite each other; but raylib's
`.o` files live in its own source tree, so run `make -C Game/ThirdParty/raylib/src clean` when
switching toolchains on one machine. `make clean-thirdparty` clears the current target's copy.

Third-party sources are git-ignored (`Game/ThirdParty/`), so clone them per machine at the
pinned versions listed in README.md before the first build.

---

## 4. Path B — cross-compiling Windows from Linux

This is the route that is actually verified. There is no platform code to worry about, so it
is a straightforward toolchain swap.

```sh
sudo pacman -S mingw-w64-gcc          # or: apt install g++-mingw-w64-x86-64

# raylib keeps its objects in raylib/src, so clear a previous host build first.
make -C Game/ThirdParty/raylib/src clean

make release TARGET_OS=Windows \
     CXX=x86_64-w64-mingw32-g++ \
     CC=x86_64-w64-mingw32-gcc \
     AR=x86_64-w64-mingw32-ar
```

`CC` matters as much as `CXX`: raylib is C, and it is `CC` that gets forwarded to its Makefile.

**Use the `release` / `development` / `debug` goal, not `CONFIG=Release` on its own.** With no
goal the default is `default: development`, which re-invokes make with `CONFIG=Development` and
silently discards the `CONFIG` on the command line.

The `make -C raylib/src clean` is needed only when switching toolchains. raylib archives
`libraylib.a` into the per-target `RAYLIB_OUT`, but its `.o` files stay in its own source tree,
where a stale Linux object would otherwise be archived into the Windows library.

The asset cooker runs on the host as normal — the cooked formats are plain binary and
platform-independent, and `Build/Windows/<Config>/assets/` is written by the same build event.

### Verified result

| Check | Result |
|---|---|
| `make release TARGET_OS=Windows …` | exit 0, no warnings from `Game/Source` |
| `file ArcadeCarSoccer.exe` | `PE32+ executable for MS Windows, x86-64` |
| `objdump -p` imports | system DLLs only — no MinGW runtime to ship |
| `wine ./ArcadeCarSoccer.exe --smoke-test 60 --screenshot SmokeTest.png` | exit 0, non-blank screenshot of the arena at kickoff |
| Linux `make all` after the change | unchanged, smoke test still exit 0 |

Wine is a proxy for Windows, not Windows. It exercises the executable, the cooked assets, the
OpenGL 3.3 path and the audio device, but a real machine is still the last word — particularly
for fullscreen and multi-monitor behaviour (section 7).

The `-Wstringop-overflow` warnings the Windows build emits come from Jolt's atomics through
MinGW's libstdc++ headers at `-O3`, not from game code, which builds clean under `-Wall
-Wextra` on both platforms.

---

## 5. macOS is native-only

Cross-compiling macOS from Linux needs Apple's SDK and frameworks, which are not
redistributable. Toolchains such as osxcross exist but are out of scope here. **Build macOS
on a Mac.**

---

## 6. Running the result

The executable needs its cooked `assets/` folder **beside it** — `assets::Path` resolves
against `GetApplicationDirectory()`, not the working directory. The `cook` build event
already writes `Build/<Config>/assets/`, so shipping means copying the executable and that
folder together.

The existing smoke test is the cross-platform check and needs no display-server plumbing on
Windows or macOS:

```sh
./ArcadeCarSoccer --smoke-test 60 --screenshot SmokeTest.png
```

It should exit 0 and write a non-blank screenshot.

---

## 7. Caveats to expect on the new platforms

- **`Tuning.cfg` is written next to the executable** (`TuningPanel.cpp:19`). Under
  `C:\Program Files` that fails on Windows without elevation. It is a dev-tools feature and is
  absent from Release builds, so it only affects Debug/Development.
- **Fullscreen at launch** uses `GetCurrentMonitor` + `SetWindowSize` + `ToggleFullscreen`
  (`App.cpp`). All three are cross-platform in raylib, but multi-monitor behaviour is worth
  re-checking per OS.
- **OpenGL 3.3 is deprecated on macOS** (though still functional, including on Apple Silicon).
  If Apple ever removes it, raylib's Metal/ANGLE backends are the migration path — not
  something this game's code would need to change.
