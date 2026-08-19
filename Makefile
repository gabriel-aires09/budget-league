# Arcade Car Soccer - build (Linux, g++)
#
#   make                    build Development (default)
#   make debug|development|release
#   make all                build the three modes
#   make run [CONFIG=...]   build and run
#   make clean              remove Build/ artifacts (keeps ThirdParty)
#   make clean-thirdparty   also rebuild raylib next time
#
# Cross-compiling the Windows executable from Linux (see
# docs/CrossPlatformBuild.md):
#
#   make CONFIG=Release TARGET_OS=Windows CXX=x86_64-w64-mingw32-g++ \
#        CC=x86_64-w64-mingw32-gcc AR=x86_64-w64-mingw32-ar

CONFIG ?= Development

# Native by default; override TARGET_OS to cross-compile.
ifeq ($(OS),Windows_NT)
    TARGET_OS ?= Windows
else
    TARGET_OS ?= $(shell uname -s)
endif
ARCH := $(shell uname -m)

ROOT       := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
SRC_DIR    := $(ROOT)/Game/Source
TP_DIR     := $(ROOT)/Game/ThirdParty
ASSETS_DIR := $(ROOT)/Game/Assets

# Linux keeps the historical Build/<Config> layout; other targets nest under
# their own folder so a native and a cross object never share a path.
ifeq ($(TARGET_OS),Linux)
    OUT_ROOT := $(ROOT)/Build
else
    OUT_ROOT := $(ROOT)/Build/$(TARGET_OS)
endif

ifeq ($(TARGET_OS),Windows)
    EXE_EXT := .exe
endif

BUILD_DIR  := $(OUT_ROOT)/$(CONFIG)
OBJ_DIR    := $(OUT_ROOT)/Intermediate/$(CONFIG)
TP_OBJ_DIR := $(OBJ_DIR)/ThirdParty
# No space in the name on purpose: make splits its targets and prerequisites on
# whitespace, so "Budget League" cannot be a build target at all.
TARGET     := $(BUILD_DIR)/BudgetLeague$(EXE_EXT)

# raylib is third party and never debugged, so it is built once (release) and
# shared by the three configurations.
RAYLIB_DIR := $(TP_DIR)/raylib
RAYLIB_OUT := $(OUT_ROOT)/Intermediate/ThirdParty/raylib
RAYLIB_LIB := $(RAYLIB_OUT)/libraylib.a

ifeq ($(TARGET_OS),Windows)
    RAYLIB_OS := WINDOWS
else ifeq ($(TARGET_OS),Darwin)
    RAYLIB_OS := OSX
else
    RAYLIB_OS := LINUX
endif

# Asset cooker runs as a build event; use the venv interpreter when present.
# The cooker runs on the host, so this keys off the host and not TARGET_OS.
ifeq ($(OS),Windows_NT)
    PYTHON := $(if $(wildcard $(ROOT)/.venv/Scripts/python.exe),$(ROOT)/.venv/Scripts/python.exe,python)
else
    PYTHON := $(if $(wildcard $(ROOT)/.venv/bin/python),$(ROOT)/.venv/bin/python,python3)
endif

# ---------------------------------------------------------------- build modes

# SSE level must be identical for Jolt and for every file that includes it.
# x86 only: Jolt autodetects NEON on ARM and these flags do not compile there.
ifeq ($(filter arm64 aarch64,$(ARCH)),)
    SIMD_FLAGS := -msse4.2 -mpopcnt
else
    SIMD_FLAGS :=
endif

ifeq ($(CONFIG),Debug)
    CONFIG_FLAGS := -O0 -g -DGAME_DEBUG -DGAME_DEV_TOOLS
    JOLT_DEFINES := -DJPH_PROFILE_ENABLED -DJPH_DEBUG_RENDERER -DJPH_OBJECT_STREAM -DJPH_ENABLE_ASSERTS
    LDFLAGS_MODE :=
else ifeq ($(CONFIG),Development)
    CONFIG_FLAGS := -O2 -g -DGAME_DEVELOPMENT -DGAME_DEV_TOOLS
    JOLT_DEFINES := -DJPH_PROFILE_ENABLED -DJPH_DEBUG_RENDERER -DJPH_OBJECT_STREAM
    LDFLAGS_MODE :=
else ifeq ($(CONFIG),Release)
    CONFIG_FLAGS := -O3 -DNDEBUG -DGAME_RELEASE
    JOLT_DEFINES := -DJPH_OBJECT_STREAM
    # Apple's linker rejects -s.
    ifeq ($(TARGET_OS),Darwin)
        LDFLAGS_MODE :=
    else ifeq ($(TARGET_OS),Windows)
        # -mwindows builds against the GUI subsystem instead of the console one.
        # Without it Windows opens a terminal alongside the game and fills it
        # with raylib's start-up log, which is a development tool showing up in
        # a shipped build. Release only: Debug and Development keep their
        # console, which is where the log is wanted.
        LDFLAGS_MODE := -s -mwindows
    else
        LDFLAGS_MODE := -s
    endif
else
    $(error Unknown CONFIG '$(CONFIG)'. Use Debug, Development or Release)
endif

# Third party headers are -isystem so their warnings do not pollute the game build.
INCLUDES := -I$(SRC_DIR) -isystem $(RAYLIB_DIR)/src -isystem $(TP_DIR)/Jolt \
            -isystem $(TP_DIR)/glm -isystem $(TP_DIR)/imgui

CXXFLAGS := -std=c++17 $(SIMD_FLAGS) $(CONFIG_FLAGS) $(JOLT_DEFINES) $(INCLUDES) -MMD -MP
GAME_CXXFLAGS := $(CXXFLAGS) -Wall -Wextra

LDFLAGS := $(LDFLAGS_MODE)

# Values taken from raylib's own Makefile for each platform. The Windows link is
# fully static so the executable does not need the MinGW runtime DLLs beside it.
ifeq ($(TARGET_OS),Windows)
    LDLIBS := $(RAYLIB_LIB) -static -static-libgcc -static-libstdc++ \
              -lopengl32 -lgdi32 -lwinmm
else ifeq ($(TARGET_OS),Darwin)
    LDLIBS := $(RAYLIB_LIB) -framework OpenGL -framework Cocoa -framework IOKit \
              -framework CoreAudio -framework CoreVideo
else
    LDLIBS := $(RAYLIB_LIB) -lGL -lm -lpthread -ldl -lrt -lX11
endif

# ------------------------------------------------------------------- sources

GAME_SRCS := $(shell find $(SRC_DIR) -name '*.cpp')
JOLT_SRCS := $(shell find $(TP_DIR)/Jolt/Jolt -name '*.cpp')
IMGUI_SRCS := $(TP_DIR)/imgui/imgui.cpp $(TP_DIR)/imgui/imgui_draw.cpp \
              $(TP_DIR)/imgui/imgui_tables.cpp $(TP_DIR)/imgui/imgui_widgets.cpp

GAME_OBJS  := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(GAME_SRCS))
TP_OBJS    := $(patsubst $(TP_DIR)/%.cpp,$(TP_OBJ_DIR)/%.o,$(JOLT_SRCS) $(IMGUI_SRCS))
OBJS       := $(GAME_OBJS) $(TP_OBJS)

# The icon in the executable itself, which only Windows has: it is a resource
# compiled into the PE. ELF carries no icon at all and macOS keeps one in a
# bundle, so on those two platforms the window icon set in App::Initialize is
# the whole of it. The .ico is generated from the same logo the game draws.
ifeq ($(TARGET_OS),Windows)
    ICON_PNG := $(ASSETS_DIR)/Icon/budget-league-logo.png
    ICON_ICO := $(OBJ_DIR)/BudgetLeague.ico
    ICON_OBJ := $(OBJ_DIR)/BudgetLeague.res.o
    # Whatever compiler is in use names its own windres: the MinGW cross build
    # has a prefixed one, a native MSYS2 build has it unprefixed on the path.
    WINDRES ?= $(if $(findstring mingw32,$(CXX)),x86_64-w64-mingw32-windres,windres)
    OBJS += $(ICON_OBJ)
endif
DEPS       := $(OBJS:.o=.d)

# ------------------------------------------------------------------- targets

.PHONY: default all debug development release game cook run clean clean-thirdparty

default: development

debug:
	@$(MAKE) --no-print-directory CONFIG=Debug game
development:
	@$(MAKE) --no-print-directory CONFIG=Development game
release:
	@$(MAKE) --no-print-directory CONFIG=Release game

all: debug development release

game: $(TARGET) cook

$(TARGET): $(OBJS) $(RAYLIB_LIB)
	@mkdir -p $(@D)
	@echo "[link] $@"
	@$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	@echo "[$(CONFIG)] $<"
	@$(CXX) $(GAME_CXXFLAGS) -c $< -o $@

# Regenerated when the logo or the tool changes, like every other cooked asset.
$(ICON_ICO): $(ICON_PNG) $(ROOT)/Tools/MakeIcon.py
	@mkdir -p $(@D)
	@$(PYTHON) $(ROOT)/Tools/MakeIcon.py --input $(ICON_PNG) --output $@

# windres finds the .ico through the include directory, so the .rc can name it
# without knowing which configuration is being built.
$(ICON_OBJ): $(SRC_DIR)/BudgetLeague.rc $(ICON_ICO)
	@mkdir -p $(@D)
	@echo "[$(CONFIG)] $<"
	@$(WINDRES) --include-dir $(dir $(ICON_ICO)) -i $< -o $@

$(TP_OBJ_DIR)/%.o: $(TP_DIR)/%.cpp
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(RAYLIB_LIB):
	@mkdir -p $(RAYLIB_OUT)
	@echo "[thirdparty] building raylib"
	@$(MAKE) --no-print-directory -C $(RAYLIB_DIR)/src \
		PLATFORM=PLATFORM_DESKTOP PLATFORM_OS=$(RAYLIB_OS) CC=$(CC) AR=$(AR) \
		RAYLIB_LIBTYPE=STATIC RAYLIB_BUILD_MODE=RELEASE \
		RAYLIB_RELEASE_PATH=$(RAYLIB_OUT)

# Build event: cooked assets live next to the executable of each configuration.
cook:
	@$(PYTHON) $(ROOT)/Tools/AssetCooker.py --assets $(ASSETS_DIR) --output $(BUILD_DIR)/assets

run: game
	@cd $(BUILD_DIR) && ./BudgetLeague$(EXE_EXT)

clean:
	@rm -rf $(OUT_ROOT)/Debug $(OUT_ROOT)/Development $(OUT_ROOT)/Release \
	        $(OUT_ROOT)/Intermediate/Debug $(OUT_ROOT)/Intermediate/Development \
	        $(OUT_ROOT)/Intermediate/Release
	@echo "[clean] done"

clean-thirdparty: clean
	@$(MAKE) --no-print-directory -C $(RAYLIB_DIR)/src clean
	@rm -rf $(OUT_ROOT)/Intermediate/ThirdParty
	@echo "[clean] thirdparty done"

-include $(DEPS)
