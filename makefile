all:
	gcc-5 -L/usr/local/lib -I/usr/local/include -lstdc++ -lm -lpng -fopenmp -O3 -o card _main.cpp ToPng.cpp