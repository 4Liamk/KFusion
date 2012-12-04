#!/bin/bash

#This is the basic test code, it tests for various vector sizes on different platforms.  This assumes each system as a CPU and GPU, if you do not, feel free to remove on set of tests.  If you have more devices, add another batch.

#Each test is done 1000 times, the results are stored in gpu_results.x.txt and cpu_result.x.txt where x is the vector size

#these variables set which platform and device the cpu is

cpu_platform=1
cpu_device=0

#these variables set the gpu platform and device.
gpu_platform=0
gpu_device=1

for i in 16 32 64 128 256 1024 2048 4096 8192 16284
do
	./main $gpu_platform $gpu_device $i 1000 > gpu_results.$i.txt
done

for i in 16 32 64 128 256 1024 2048 4096 8192 16284
do
	./main $cpu_platform $cpu_device $i 1000 > cpu_results.$i.txt
done

