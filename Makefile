# Makefile for Smart Dictionary & Autocomplete Engine
# Builds two independent executables:
#   smart_dict.exe      — original terminal (CLI) interface
#   smart_dict_gui.exe  — GTK3 graphical interface
#
# Requires GTK3 via MSYS2:
#   pacman -S mingw-w64-x86_64-gtk3
#
# Usage:
#   make              -- build both CLI and GUI
#   make cli          -- build terminal version only
#   make gui          -- build GTK3 version only
#   make run          -- build and run CLI
#   make run-gui      -- build and run GUI
#   make clean        -- remove all build artefacts
#   make rebuild      -- clean then build both

# ── Compiler settings ──────────────────────────────────────────
CC     = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -g

# ── GTK3 flags (from pkg-config) ──────────────────────────────
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0 2>/dev/null)
GTK_LIBS   = $(shell pkg-config --libs   gtk+-3.0 2>/dev/null)

# ── Shared source files (no GTK dependency) ───────────────────
SHARED_SRCS = dictionary.c utils.c bst.c avl.c tbt.c \
              loader.c autocomplete.c benchmark.c
SHARED_OBJS = $(SHARED_SRCS:.c=.o)

# ── CLI target ────────────────────────────────────────────────
CLI_TARGET = smart_dict.exe
CLI_OBJS   = main.o $(SHARED_OBJS)

# ── GUI target ────────────────────────────────────────────────
GUI_TARGET = smart_dict_gui.exe
GUI_OBJS   = gui_main.o $(SHARED_OBJS)

# ── Default: build both ───────────────────────────────────────
all: $(CLI_TARGET) $(GUI_TARGET)

# ── CLI build ─────────────────────────────────────────────────
cli: $(CLI_TARGET)

$(CLI_TARGET): $(CLI_OBJS)
	$(CC) $(CFLAGS) -o $(CLI_TARGET) $(CLI_OBJS)
	@echo Build complete: $(CLI_TARGET)

# ── GUI build ─────────────────────────────────────────────────
gui: $(GUI_TARGET)

$(GUI_TARGET): $(GUI_OBJS)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -mwindows \
	    -o $(GUI_TARGET) $(GUI_OBJS) $(GTK_LIBS)
	@echo Build complete: $(GUI_TARGET)

# gui_main.o needs GTK3 include flags — explicit rule takes priority
# over the generic pattern rule below.
gui_main.o: gui_main.c config.h dictionary.h utils.h \
            bst.h avl.h tbt.h loader.h autocomplete.h benchmark.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c gui_main.c -o gui_main.o

# ── Pattern rule: compile shared .c files to .o ───────────────
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ── Explicit header dependencies for shared modules ───────────
main.o:         main.c config.h dictionary.h utils.h \
                bst.h avl.h tbt.h loader.h autocomplete.h benchmark.h
dictionary.o:   dictionary.c dictionary.h config.h utils.h
utils.o:        utils.c utils.h config.h
bst.o:          bst.c bst.h dictionary.h config.h utils.h
avl.o:          avl.c avl.h dictionary.h config.h utils.h
tbt.o:          tbt.c tbt.h dictionary.h config.h utils.h
loader.o:       loader.c loader.h dictionary.h bst.h avl.h tbt.h config.h utils.h
autocomplete.o: autocomplete.c autocomplete.h bst.h avl.h tbt.h \
                dictionary.h config.h utils.h
benchmark.o:    benchmark.c benchmark.h bst.h avl.h tbt.h \
                dictionary.h config.h utils.h

# ── Phony targets ─────────────────────────────────────────────
.PHONY: all cli gui clean run run-gui rebuild

run: $(CLI_TARGET)
	$(CLI_TARGET)

run-gui: $(GUI_TARGET)
	$(GUI_TARGET)

clean:
	del /Q $(SHARED_OBJS) main.o gui_main.o \
	    $(CLI_TARGET) $(GUI_TARGET) 2>nul || true

rebuild: clean all
