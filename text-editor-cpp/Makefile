CXX := g++
SDL_ROOT := F:/Interpreter/CPPLibrary/SDL3

CPPFLAGS := -I$(SDL_ROOT)/include
CXXFLAGS := -Wall -Werror -std=c++11 -pedantic -ggdb
LDFLAGS := -L$(SDL_ROOT)/lib
LIBS := -lSDL3 -lm

TARGET := te.exe
CXX_SRC := main_cpp.cc Editor.cc La.cc

$(TARGET): $(CXX_SRC)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CXX_SRC) -o $(TARGET) $(LDFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(TARGET)