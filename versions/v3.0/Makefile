# Makefile for Smart Dictionary v3.0 — Final Release
#
# Usage:
#   make          -- build smart_dict.exe
#   make run      -- build and run
#   make clean    -- remove build artefacts
#   make rebuild  -- clean then build

CC     = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -g

TARGET = smart_dict.exe
SRCS   = main.c dictionary.c utils.c bst.c avl.c tbt.c \
         loader.c autocomplete.c benchmark.c
OBJS   = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
	@echo Build complete: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Header dependencies
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

.PHONY: all clean run rebuild

run: $(TARGET)
	$(TARGET)

clean:
	del /Q $(OBJS) $(TARGET) 2>nul || true

rebuild: clean all
