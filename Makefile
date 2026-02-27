# Makefile for Smart Dictionary & Autocomplete Engine
# Target  : Windows 11, MinGW-w64 gcc
#
# Usage:
#   mingw32-make           -- build the executable
#   mingw32-make clean     -- remove build artefacts
#   mingw32-make run       -- build and run
#   mingw32-make rebuild   -- clean then build
#
# Install MinGW-w64 if not present:
#   winget install MinGW.MinGW
# Then add its bin folder (e.g. C:\msys64\mingw64\bin) to your PATH.

# ── Compiler settings ──────────────────────────────────────────
CC     = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -g

# ── Output ────────────────────────────────────────────────────
TARGET = smart_dict.exe

# ── Source files ──────────────────────────────────────────────
SRCS = main.c \
       dictionary.c \
       utils.c \
       bst.c \
       avl.c \
       tbt.c \
       loader.c \
       autocomplete.c \
       benchmark.c

# ── Object files ──────────────────────────────────────────────
OBJS = $(SRCS:.c=.o)

# ── Default target ────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
	@echo Build complete: $(TARGET)

# ── Pattern rule: compile each .c to .o ──────────────────────
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ── Explicit header dependencies ─────────────────────────────
# Ensures that changing any .h triggers recompilation of all
# .c files that include it (prevents stale-build bugs).
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
.PHONY: all clean run rebuild

clean:
	del /Q $(OBJS) $(TARGET) 2>nul || true

run: $(TARGET)
	$(TARGET)

rebuild: clean all
