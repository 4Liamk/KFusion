CC=nvcc
CXX=g++ -std=gnu++0x -g
CFLAGS=-lOpenCL -lm -lGL -lglut -lGLU -lGLEW -g
DEPS = check.h reader.h mreader.h physics-out.h
OBJ =  main-out.o reader.o physics-out.o

all:preproc main

preproc: main.cpp physics.cpp physics.h kernel.cl
	python ../preproc2.py main.cpp physics.cpp kernel.cl

main: $(OBJ)
	$(CC)  -o $@ $^ $(CFLAGS)
 
%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< 
 
clean: 
	rm *.o
	rm main
