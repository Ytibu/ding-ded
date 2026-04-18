CC := gcc

# SDL3 root configured for local Windows install
SDL_ROOT := F:/Interpreter/CPPLibrary/SDL3
SDL_DLL := $(SDL_ROOT)/bin/SDL3.dll

CFLAGS := -Wall -Werror -std=c11 -pedantic -ggdb -I$(SDL_ROOT)/include
LDFLAGS := -L$(SDL_ROOT)/lib
LIBS := -lSDL3 -lm

TARGET := te
SRC := main.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS) $(LIBS) la.c editor.c

.PHONY: clean

clean:
	rm -f $(TARGET)