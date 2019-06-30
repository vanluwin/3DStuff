.cpp:
	g++ -DUNICODE -I/usr/include/SDL2 -lSDL2 -pthread -std=c++11 $< -o $@.out `sdl2-config --cflags --libs`

clean:
	find . -name *.o -delete; find . -name *.out -delete