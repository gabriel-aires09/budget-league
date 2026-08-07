# Cross-Platform Builds — Windows, Linux and macOS

Reference material for **Milestone 21** (CLAUDE.md section 5). It lists every change needed
to produce a separate runnable executable on each of the three platforms.

> **Scope.** CLAUDE.md still declares Linux as the platform (project header, and §1.1
> "**Platform:** Linux"). Those lines are deliberately unchanged — Milestone 21 is written as
> an explicit, scoped exception to them. Linux remains the platform the game is developed and
> verified on.

> **What is verified here.** The Linux column is backed by a working build. **The Windows and
> macOS instructions have not been compiled or run** — there is no Windows or macOS machine in
> this project's environment. Treat them as written-but-unverified until someone builds them.

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

Six touch points. Line numbers are against `Makefile` as of commit `94afbc9`.

### Platform detection

There is currently none. Add near the top, before the build-mode block:

```make
ifeq ($(OS),Windows_NT)
    PLATFORM := Windows
else
    PLATFORM := $(shell uname -s)   # Linux | Darwin
endif
ARCH := $(shell uname -m)           # x86_64 | arm64 | aarch64
```

Everything below keys off `PLATFORM` and `ARCH`.

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
ifeq ($(PLATFORM),Windows)
    LDLIBS := $(RAYLIB_LIB) -static-libgcc -lopengl32 -lgdi32 -lwinmm
else ifeq ($(PLATFORM),Darwin)
    LDLIBS := $(RAYLIB_LIB) -framework OpenGL -framework Cocoa -framework IOKit \
                            -framework CoreAudio -framework CoreVideo
else
    LDLIBS := $(RAYLIB_LIB) -lGL -lc -lm -lpthread -ldl -lrt -lX11
endif
```

Note macOS needs **no** `AudioToolbox` — raylib's own list does not include it.

### What does *not* change

The `$(RAYLIB_LIB)` rule needs no edit. raylib's Makefile detects `PLATFORM_OS` from `uname`
by itself, so `make -C raylib/src PLATFORM=PLATFORM_DESKTOP` is correct on all three.

---

## 3. Path A — native build on each OS (recommended)

Build Linux on Linux, Windows on Windows, macOS on macOS. This is the fully supported route
and the `Build/<Config>/` layout needs no change.

| Platform | Toolchain | Notes |
|---|---|---|
| Linux | g++, make, Python 3, OpenGL + X11 dev packages | Unchanged; this is the current build. |
| Windows | **MSYS2 / MinGW-w64** (`mingw-w64-x86_64-gcc`, `make`) or Git Bash | The Makefile uses `find`, `mkdir -p` and `rm -rf`, so **`cmd.exe` and PowerShell will not work.** |
| macOS | Xcode Command Line Tools (`xcode-select --install`), Python 3 | Ships GNU make 3.81, which is enough. Apple Silicon needs change #1 above. |

**`libraylib.a` is not portable between platforms.** `Build/Intermediate/ThirdParty/` must be
deleted and rebuilt on each OS, or the link will fail with unresolved or malformed symbols.
`make clean-thirdparty` does this.

Third-party sources are git-ignored (`Game/ThirdParty/`), so clone them per machine at the
pinned versions listed in README.md before the first build.

---

## 4. Path B — cross-compiling Windows from Linux

Possible, and reasonable for this project since there is no platform code to worry about.

```sh
sudo pacman -S mingw-w64-gcc          # or: apt install g++-mingw-w64-x86-64

make CONFIG=Release \
     CXX=x86_64-w64-mingw32-g++ \
     AR=x86_64-w64-mingw32-ar
```

Two things must be handled or the build is wrong:

1. **raylib must be rebuilt with the same cross-compiler.** Pass the same `CC`/`AR` down to
   the `$(RAYLIB_LIB)` rule, and use a separate output directory from the native one.
2. **`Build/<Config>/` collides.** A native Linux object and a Windows object have the same
   path under the current layout. Change the intermediate and output directories to
   `Build/<Platform>/<Config>/` before cross-compiling, or clean between builds.

The asset cooker runs on the host as normal — the cooked formats are plain binary and
platform-independent.

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
