CC=nvcc
CFLAGS= -lOpenCL
DEPS = check.h reader.h mreader.h sparse-out.h sparse.h
OBJ =  main-out.o reader.o mreader.o sparse-out.o

all: preproc main

preproc: main.cpp sparse.cpp sparse.h kernel.cl
	python ../preproc2.py main.cpp sparse.cpp kernel.cl

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
  
clean: 
	rm *.o
	rm main
