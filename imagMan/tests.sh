for i in 1024 2048 4096 8192
do
	#./magickTest luke$i.png luke$i.png > magick_cpu_results.$i.txt
	#./imageTest 0 1 luke$i.png luke$i.png > fixed_gpu_results.$i.txt
	./imageTest 1 0  luke$i.png luke$i.png > fixed_cpu_results.$i.txt
done


do
	#./magickTest luke$i.png luke$i.png > magick_cpu_results.$i.txt
	#./imageTest 0 1 luke$i.png luke$i.png > fixed_gpu_results.$i.txt
	#./imageTest 1 0  luke$i.png luke$i.png > fixed_cpu_results.$i.txt
done
