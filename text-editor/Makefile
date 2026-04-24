CC := gcc

# SDL3 root configured for local Windows install
SDL_ROOT := F:/Interpreter/CPPLibrary/SDL3
GL_ROOT := F:/Interpreter/CPPLibrary/GLEW3
FT_ROOT := F:/Interpreter/CPPLibrary/FreeType
SDL_DLL := $(SDL_ROOT)/bin/SDL3.dll
GL_DLL := $(GL_ROOT)/bin/glew32.dll
FT_DLL := $(FT_ROOT)/bin/freetype.dll
FT_LIB := $(FT_ROOT)/lib/freetype.lib

CFLAGS := -Wall -Werror -std=c11 -pedantic -ggdb -I$(SDL_ROOT)/include -I$(GL_ROOT)/include -I$(FT_ROOT)/include -I$(FT_ROOT)/include/freetype2
LDFLAGS := -L$(SDL_ROOT)/lib -L$(GL_ROOT)/lib -L$(FT_ROOT)/lib
LIBS := -lSDL3 -lm -lglew32 -lopengl32 "$(FT_LIB)"

TARGET := te
SRCS := src/main.c src/la.c src/editor.c src/font.c src/sdl_extra.c src/file.c src/head.c src/tileGlyph.c src/freeGlyph.c src/CursorRenderer.c

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS) $(LIBS) 

.PHONY: clean

clean:
	rm -f $(TARGET)