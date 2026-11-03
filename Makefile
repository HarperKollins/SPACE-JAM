# ══════════════════════════════════════════════════════════════
# SPACE JAM — Makefile
# Targets: all, run, clean, release, debug
#
# Toolchain: MSYS2/MinGW-w64 UCRT64 (Windows)
#            GCC (Linux)
# ══════════════════════════════════════════════════════════════

PROJECT   := spacejam
SRC_DIR   := src
BUILD_DIR := build

# ── Source files ─────────────────────────────────────────────
SOURCES := \
	$(SRC_DIR)/main.c       \
	$(SRC_DIR)/game.c       \
	$(SRC_DIR)/world.c      \
	$(SRC_DIR)/player.c     \
	$(SRC_DIR)/enemy.c      \
	$(SRC_DIR)/asteroid.c   \
	$(SRC_DIR)/projectile.c \
	$(SRC_DIR)/particles.c  \
	$(SRC_DIR)/powerup.c    \
	$(SRC_DIR)/collision.c  \
	$(SRC_DIR)/hud.c        \
	$(SRC_DIR)/menu.c       \
	$(SRC_DIR)/audio.c      \
	$(SRC_DIR)/progress.c   \
	$(SRC_DIR)/relic.c      \
	$(SRC_DIR)/highscore.c  \
	$(SRC_DIR)/settings.c

OBJECTS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SOURCES))

# ── Platform detection ────────────────────────────────────────
ifeq ($(OS),Windows_NT)
    PLATFORM := windows
    EXE      := $(BUILD_DIR)/$(PROJECT).exe
    CC       := gcc
    # MSYS2 ucrt64 raylib paths
    RAYLIB_INCLUDE := /ucrt64/include
    RAYLIB_LIB     := /ucrt64/lib
    RAYLIB_LIBS    := -lraylib -lopengl32 -lgdi32 -lwinmm
    EXTRA_CFLAGS   :=
    EXTRA_LDFLAGS  := -mwindows
    RES_OBJ        := $(BUILD_DIR)/spacejam_res.o
    EXTRA_OBJS     := $(RES_OBJ)
else
    PLATFORM := linux
    EXE      := $(BUILD_DIR)/$(PROJECT)
    CC       := gcc
    RAYLIB_INCLUDE := /usr/local/include
    RAYLIB_LIB     := /usr/local/lib
    RAYLIB_LIBS    := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    EXTRA_CFLAGS   :=
    EXTRA_LDFLAGS  :=
    EXTRA_OBJS     :=
endif

# ── Compiler flags ────────────────────────────────────────────
COMMON_CFLAGS := \
	-std=c11            \
	-Wall               \
	-Wextra             \
	-Wpedantic          \
	-I$(SRC_DIR)        \
	-I$(RAYLIB_INCLUDE) \
	$(EXTRA_CFLAGS)

DEBUG_CFLAGS   := $(COMMON_CFLAGS) -g -O0 -DDEBUG
RELEASE_CFLAGS := $(COMMON_CFLAGS) -O2 -DNDEBUG

LDFLAGS := -L$(RAYLIB_LIB) $(RAYLIB_LIBS) $(EXTRA_LDFLAGS) -lm

# Default to release build
CFLAGS := $(RELEASE_CFLAGS)

# ── Targets ──────────────────────────────────────────────────

.PHONY: all run clean release debug help

## all: Build the game (release mode)
all: $(EXE)

## release: Build optimised release binary
release: CFLAGS := $(RELEASE_CFLAGS)
release: clean $(EXE)
	@echo "[release] Build complete: $(EXE)"

## debug: Build with debug symbols and no optimisation
debug: CFLAGS := $(DEBUG_CFLAGS)
debug: $(EXE)
	@echo "[debug] Build complete: $(EXE)"

## run: Build and run the game
run: all
	@echo "[run] Launching $(EXE)..."
	cd $(BUILD_DIR) && ./$(notdir $(EXE))

## clean: Remove build artefacts
clean:
	@echo "[clean] Removing build directory..."
	rm -rf $(BUILD_DIR)
	@echo "[clean] Done."

## help: Show available targets
help:
	@echo "Available targets:"
	@grep "^##" Makefile | sed 's/## /  /'

# ── Build rules ───────────────────────────────────────────────

$(EXE): $(OBJECTS) $(EXTRA_OBJS)
	@echo "[link] $@"
	$(CC) -o $@ $(OBJECTS) $(EXTRA_OBJS) $(LDFLAGS)
ifeq ($(PLATFORM),windows)
	@cp -u /ucrt64/bin/libraylib.dll /ucrt64/bin/glfw3.dll $(BUILD_DIR)/ 2>/dev/null || true
	@cp -u /ucrt64/bin/libraylib.dll /ucrt64/bin/glfw3.dll . 2>/dev/null || true
	@cp -u $(EXE) . 2>/dev/null || true
endif
	@cp -ru assets $(BUILD_DIR)/ 2>/dev/null || true

$(BUILD_DIR)/spacejam_res.o: spacejam.rc spacejam.ico | $(BUILD_DIR)
	@echo "[rc]   $<"
	windres spacejam.rc -O coff -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "[cc]   $<"
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ── Dependency auto-generation ────────────────────────────────
# Rebuild when headers change
$(BUILD_DIR)/%.d: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -MM -MT $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$<) $< > $@

-include $(OBJECTS:.o=.d)

