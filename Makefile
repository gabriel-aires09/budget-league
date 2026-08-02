# Arcade Car Soccer - build (Linux, g++)
#
#   make                    build Development (default)
#   make debug|development|release
#   make all                build the three modes
#   make run [CONFIG=...]   build and run
#   make clean              remove Build/ artifacts (keeps ThirdParty)
#   make clean-thirdparty   also rebuild raylib next time

CONFIG ?= Development

ROOT       := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
SRC_DIR    := $(ROOT)/Game/Source
TP_DIR     := $(ROOT)/Game/ThirdParty
ASSETS_DIR := $(ROOT)/Game/Assets
BUILD_DIR  := $(ROOT)/Build/$(CONFIG)
OBJ_DIR    := $(ROOT)/Build/Intermediate/$(CONFIG)
TP_OBJ_DIR := $(OBJ_DIR)/ThirdParty
TARGET     := $(BUILD_DIR)/ArcadeCarSoccer

# raylib is third party and never debugged, so it is built once (release) and
# shared by the three configurations.
RAYLIB_DIR := $(TP_DIR)/raylib
RAYLIB_OUT := $(ROOT)/Build/Intermediate/ThirdParty/raylib
RAYLIB_LIB := $(RAYLIB_OUT)/libraylib.a

# Asset cooker runs as a build event; use the venv interpreter when present.
PYTHON := $(if $(wildcard $(ROOT)/.venv/bin/python),$(ROOT)/.venv/bin/python,python3)

# ---------------------------------------------------------------- build modes

# SSE level must be identical for Jolt and for every file that includes it.
SIMD_FLAGS := -msse4.2 -mpopcnt

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
    LDFLAGS_MODE := -s
else
    $(error Unknown CONFIG '$(CONFIG)'. Use Debug, Development or Release)
endif

# Third party headers are -isystem so their warnings do not pollute the game build.
INCLUDES := -I$(SRC_DIR) -isystem $(RAYLIB_DIR)/src -isystem $(TP_DIR)/Jolt \
            -isystem $(TP_DIR)/glm -isystem $(TP_DIR)/imgui

CXXFLAGS := -std=c++17 $(SIMD_FLAGS) $(CONFIG_FLAGS) $(JOLT_DEFINES) $(INCLUDES) -MMD -MP
GAME_CXXFLAGS := $(CXXFLAGS) -Wall -Wextra

LDFLAGS := $(LDFLAGS_MODE)
LDLIBS  := $(RAYLIB_LIB) -lGL -lm -lpthread -ldl -lrt -lX11

# ------------------------------------------------------------------- sources

GAME_SRCS := $(shell find $(SRC_DIR) -name '*.cpp')
JOLT_SRCS := $(shell find $(TP_DIR)/Jolt/Jolt -name '*.cpp')
IMGUI_SRCS := $(TP_DIR)/imgui/imgui.cpp $(TP_DIR)/imgui/imgui_draw.cpp \
              $(TP_DIR)/imgui/imgui_tables.cpp $(TP_DIR)/imgui/imgui_widgets.cpp

GAME_OBJS  := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(GAME_SRCS))
TP_OBJS    := $(patsubst $(TP_DIR)/%.cpp,$(TP_OBJ_DIR)/%.o,$(JOLT_SRCS) $(IMGUI_SRCS))
OBJS       := $(GAME_OBJS) $(TP_OBJS)
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

$(TP_OBJ_DIR)/%.o: $(TP_DIR)/%.cpp
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(RAYLIB_LIB):
	@mkdir -p $(RAYLIB_OUT)
	@echo "[thirdparty] building raylib"
	@$(MAKE) --no-print-directory -C $(RAYLIB_DIR)/src \
		PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=STATIC RAYLIB_BUILD_MODE=RELEASE \
		RAYLIB_RELEASE_PATH=$(RAYLIB_OUT)

# Build event: cooked assets live next to the executable of each configuration.
cook:
	@$(PYTHON) $(ROOT)/Tools/AssetCooker.py --assets $(ASSETS_DIR) --output $(BUILD_DIR)/assets

run: game
	@cd $(BUILD_DIR) && ./ArcadeCarSoccer

clean:
	@rm -rf $(ROOT)/Build/Debug $(ROOT)/Build/Development $(ROOT)/Build/Release \
	        $(ROOT)/Build/Intermediate/Debug $(ROOT)/Build/Intermediate/Development \
	        $(ROOT)/Build/Intermediate/Release
	@echo "[clean] done"

clean-thirdparty: clean
	@$(MAKE) --no-print-directory -C $(RAYLIB_DIR)/src clean
	@rm -rf $(ROOT)/Build/Intermediate/ThirdParty
	@echo "[clean] thirdparty done"

-include $(DEPS)
