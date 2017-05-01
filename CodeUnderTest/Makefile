CXX=g++
SDK_ROOT=/home/ubuntu/NVIDIA_GPU_Computing_SDK/
CUDA_ROOT=/usr/local/cuda/
CUDA_FLAGS=-Wno-deprecated-gpu-targets -I${SDK_ROOT}/C/common/inc/ -L${CUDA_ROOT} -lcudart -L${SDK_ROOT}/C/lib/
PRODUCT= test_benchmarks

all: options.o hough pyramid sobel test_benchmarks
	
options.o:
	sudo ${CXX} -c -lpthread options.cpp ppm.cpp -I./

hough: hough.cpp hough_kernel.cu 
	### BUILDING HOUGH BENCHMARK ###
	nvcc $@.cpp hough_kernel.cu -o $@ options.o ppm.o -O0 ${CUDA_FLAGS} -Xcompiler -ftest-coverage -Xcompiler -fprofile-arcs -Xcompiler --coverage
	
pyramid: pyramid.cpp pyramid_kernel.cu
	### BUILDING PYRAMIDAL BENCHMARK ###
	nvcc $@.cpp pyramid_kernel.cu -o $@ options.o ppm.o -O0 ${CUDA_FLAGS} -Xcompiler  -ftest-coverage -Xcompiler -fprofile-arcs  -Xcompiler --coverage
	
sobel: sobel.cpp sobel_kernel.cu
	### BUILDING SOBEL BENCHMARK ###
	nvcc $@.cpp sobel_kernel.cu -o $@ options.o ppm.o -O0 ${CUDA_FLAGS} -Xcompiler -ftest-coverage -Xcompiler -fprofile-arcs  -Xcompiler --coverage

test_benchmarks: testDriver.o
	### BUILDING TEST DRIVER ###
	g++ testDriver.c ppm.o -o $@ -O0 -Wall -ftest-coverage -fprofile-arcs --coverage
	
test:
	### FORCING REBUILD OF TEST DRIVER ###
	g++ testDriver.c -o test_benchmarks ppm.o -O0 -Wall
	
%.o: %.c
	${CXX} -c $< -o $@
	
clean:
	rm -f *.o *~
	rm -f hough hough.pgm houghOut.pgm sobelOut.pgm 
	rm -f pyramid pyrdown.pgm pyrup.pgm
	rm -f sobel sobel_out.pgm
	rm -f test_benchmarks sobel_diff.pgm hough_diff.pgm
	rm -f *.gcda *.gcno