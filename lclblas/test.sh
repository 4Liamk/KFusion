#!/bin/bash

for i in 16 32 64 128 256 1024 2048 4096 8192 16284
do
	./main 0 1 $i 1000 > gpu_results.$i.txt
done

for i in 16 32 64 128 256 1024 2048 4096 8192 16284
do
	./main 1 0 $i 1000 > cpu_results.$i.txt
done

