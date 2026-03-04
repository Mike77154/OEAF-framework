CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c99

INCLUDES = -Iinclude -I.
LIB_OEAF = build/liboeaf.a
DEMO = build/oeaf_demo

all: $(DEMO)

build:
	@mkdir -p build

build/oeaf_core.o: src/oeaf_core.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build/oeaf_dispatch.o: src/oeaf_dispatch.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build/oeaf_sdl.o: src/oeaf_sdl.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build/oeaf_pixels.o: src/oeaf_pixels.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build/oeaf_rwops.o: src/oeaf_rwops.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build/oeaf_image.o: src/oeaf_image.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build/oeaf_ttf.o: src/oeaf_ttf.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build/oeaf_mixer.o: src/oeaf_mixer.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build/oeaf_sdl_audio.o: src/oeaf_sdl_audio.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build/null_backend.o: backends/null/oeaf_null_backend.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(LIB_OEAF): build/oeaf_core.o build/oeaf_dispatch.o build/oeaf_sdl.o \
	build/oeaf_pixels.o build/oeaf_rwops.o build/oeaf_image.o build/oeaf_ttf.o build/oeaf_mixer.o build/oeaf_sdl_audio.o \
	build/null_backend.o | build
	ar rcs $@ build/oeaf_core.o build/oeaf_dispatch.o build/oeaf_sdl.o \
		build/oeaf_pixels.o build/oeaf_rwops.o build/oeaf_image.o build/oeaf_ttf.o build/oeaf_mixer.o build/oeaf_sdl_audio.o \
		build/null_backend.o

build/demo.o: examples/oeaf_demo/demo.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(DEMO): $(LIB_OEAF) build/demo.o | build
	$(CC) build/demo.o -o $(DEMO) $(LIB_OEAF)

clean:
	rm -rf build

.PHONY: all clean
