all:
	g++ spring.cxx `sdl2-config --cflags --libs` -lSDL2_ttf
