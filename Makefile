all:
	g++ spring.cxx -o a.exe `sdl2-config --cflags --libs` -lSDL2_ttf
