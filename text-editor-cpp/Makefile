CXX := g++
SDL_ROOT := F:/Interpreter/CPPLibrary/SDL3

CPPFLAGS := -I$(SDL_ROOT)/include
CXXFLAGS := -Wall -Werror -std=c++11 -pedantic -ggdb
LDFLAGS := -L$(SDL_ROOT)/lib
LIBS := -lSDL3 -lm

TARGET := te.exe
CXX_SRC := main_cpp.cc Editor.cc La.cc
OBJS := $(CXX_SRC:.cc=.o)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS) $(LIBS)

%.o: %.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(TARGET) $(OBJS)