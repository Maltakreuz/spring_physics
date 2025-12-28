CXX = g++
CXXFLAGS = -Wall -Wextra -O2
SDL = `sdl2-config --cflags --libs` -lSDL2_ttf

all:
	$(CXX) $(CXXFLAGS) spring.cxx $(SDL) -o spring
	$(CXX) $(CXXFLAGS) spring_both_sides.cxx $(SDL) -o spring_both_sides