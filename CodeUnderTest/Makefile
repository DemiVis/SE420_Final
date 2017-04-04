CXX=g++
SDK_ROOT=/home/ubuntu/NVIDIA_GPU_Computing_SDK/
CUDA_ROOT=/usr/local/cuda/
CUDA_FLAGS=-Wno-deprecated-gpu-targets -I${SDK_ROOT}/C/common/inc/ -L${CUDA_ROOT} -lcudart -L${SDK_ROOT}/C/lib/
PRODUCT= test_benchmarks

all: options.o hough pyramid sobel test_benchmarks
	
options.o:
	sudo ${CXX} -c -lpthread options.cpp ppm.cpp -I./

hough: hough.cu houghKernels.cu 
	### BUILDING HOUGH BENCHMARK ###
	nvcc $@.cu houghKernels.cu -o $@ options.o ppm.o -O0 ${CUDA_FLAGS}
	
pyramid: pyramid.cu pyramid_kernel.cu
	### BUILDING PYRAMIDAL BENCHMARK ###
	nvcc $@.cu pyramid_kernel.cu -o $@ options.o ppm.o -O0 ${CUDA_FLAGS}
	
sobel: sobel.cu
	### BUILDING SOBEL BENCHMARK ###
	nvcc $@.cu -o $@ options.o ppm.o -O0 ${CUDA_FLAGS}

test_benchmarks: testDriver.o
	### BUILDING TEST DRIVER ###
	g++ testDriver.c -o $@ -O0 -Wall
	
test:
	### FORCING REBUILD OF TEST DRIVER ###
	g++ testDriver.c -o test_benchmarks ppm.o -O0 -Wall
	
%.o: %.c
	${CXX} -c $< -o $@
	
clean:
	rm -f *.o *~
	rm -f hough hough.ppm houghOut.pgm sobelOut.pgm 
	rm -f pyramid pyrdown.ppm pyrup.ppm pyrdiff.ppm
	rm -f sobel sobel_out.ppm
	rm -f test_benchmarks