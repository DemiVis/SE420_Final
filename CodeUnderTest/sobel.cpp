//*****************************************************************************************//
//  Sobel_kernal.cu - CUDA Sobel Edge detection benchmark
//
//  Authors: Ramnarayan Krishnamurthy, University of Colorado (Shreyas.Ramnarayan@gmail.com)
//	         Matthew Demi Vis, Embry-Riddle Aeronautical University (MatthewVis@gmail.com)
//			 
//	This code was used to obtain results documented in the SPIE Sensor and Technologies paper: 
//	S. Siewert, V. Angoth, R. Krishnamurthy, K. Mani, K. Mock, S. B. Singh, S. Srivistava, 
//	C. Wagner, R. Claus, M. Demi Vis, “Software Defined Multi-Spectral Imaging for Arctic 
//	Sensor Networks”, SPIE Algorithms and Technologies for Multipectral, Hyperspectral, and 
//	Ultraspectral Imagery XXII, Baltimore, Maryland, April 2016. 
//
//	This code was developed for, tested and run on a Jetson TK1 development kit by NVIDIA
//  running Ubuntu 14.04. 
//	
//	Please use at your own risk. We are sharing so that other researchers and developers can 
//	recreate our results and make suggestions to improve and extend the benchmarks over time.
//
//*****************************************************************************************//

// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <sys/io.h>
#include <iostream>

#include <time.h>
#include <pthread.h>
#include <sched.h>

// Project Includes
#include <cuda_runtime.h>
#include "ppm.h"
#include "options.h"

// Project Specific Defines
#define BLOCK_SIZE 	8
#define DEFAULT_IMAGE "beach.pgm"
#define MAX_IMG_SZ		8290000 // px
#define MIN_IMG_SZ		100000	// px

// Return Codes
#define EXIT_SUCCESS				0
#define EXIT_UNKOWN_ERROR			-1
#define EXIT_IMG_SZ					1
#define EXIT_IN_IMG_NOT_FOUND		2
#define EXIT_IN_IMG_FORMATTING		3
#define EXIT_CUDA_ERR				4

//#define DEBUG

#define TIMING_FILE	"sobel_timing.txt"
#define NS_PER_SEC	1000000000
#define MS_PER_SEC	1000000

// Kernels (in sobel_kernel.cu)
extern void sobel_transform_wrapper(unsigned char *img_out, unsigned char *img_in, unsigned int width, unsigned int height, dim3 grid, dim3 threads);
extern void CPU_transform(unsigned char *img_out, unsigned char *img_in, unsigned int width, unsigned int height);

// Global variables for RT threads
pthread_attr_t rt_sched_attr;
struct sched_param rt_param;
pid_t mainpid;
pthread_t rt_thread;
int rt_max_prio;

// Global Variables for Transform
unsigned char *h_img_out_array, *h_img_in_array;
struct timespec run_time = {0, 0};
unsigned int img_width, img_height, img_chan;
bool run_once = false;
int freq = 0;
std::string imageFilename = DEFAULT_IMAGE;
double elap_time_d;

//***************************************************************//
// Initialize CUDA hardware
//***************************************************************//
#if __DEVICE_EMULATION__
bool InitCUDA(void)
{
	fprintf(stderr, "There is no device.\n");
	return true;
}
#else
bool InitCUDA(void)
{
	int count = 0;
	int i = 0;

	cudaGetDeviceCount(&count);
	if(count == 0) {
		fprintf(stderr, "There is no device.\n");
		return false;
	}

	for(i = 0; i < count; i++) 
	{
		cudaDeviceProp prop;
		if(cudaGetDeviceProperties(&prop, i) == cudaSuccess) 
		{
			if(prop.major >= 1)
			break;
		}
	}

	if(i == count) 
	{
		fprintf(stderr, "There is no device supporting CUDA.\n");
		return false;
	}

	cudaSetDevice(i);

	printf("CUDA initialized.\n");
	return true;
}
#endif

//***************************************************************//
// Take the difference of two timespec structures
//***************************************************************//
void timespec_diff(struct timespec *start, struct timespec *stop,
struct timespec *result, bool check_neg)
{        
	result->tv_sec = stop->tv_sec - start->tv_sec;
	result->tv_nsec = stop->tv_nsec - start->tv_nsec;
	
	if ( check_neg && result->tv_nsec < 0) {
		result->tv_sec = result->tv_sec - 1;
		result->tv_nsec = result->tv_nsec + NS_PER_SEC;
		
	}
}

//***************************************************************//
// Convert timespec to double containing time in ms
//***************************************************************//
double timespec2double( struct timespec time_in)
{
	double rv;
	rv = (((double)time_in.tv_sec)*1000)+(((double)time_in.tv_nsec)/MS_PER_SEC);
	return rv;
}

//***************************************************************//
// Transform thread
//***************************************************************//
void *CUDA_transform_thread(void * threadp)
{
	// CUDA transform local variables
	unsigned char *d_img_out_array=NULL, *d_img_in_array=NULL;
	struct timespec start_time, end_time, elap_time, diff_time;
	int errVal;
	double start_time_d, end_time_d, diff_time_d;
	dim3 threads(BLOCK_SIZE, BLOCK_SIZE);
	dim3 grid(img_width / threads.x, img_height / threads.y);

	// Allocate CPU memory
	h_img_out_array = (unsigned char *)malloc(img_width * img_height);
	
	// Allocate CUDA memory for in and out image
	cudaMalloc((void**) &d_img_in_array, sizeof(unsigned char)*img_width*img_height);
	cudaMalloc((void**) &d_img_out_array, sizeof(unsigned char)*img_width*img_height);
	
	// Infinite loop to allow for power measurement
	do
	{
		// Get start of runtime timing
		if(clock_gettime(CLOCK_REALTIME, &start_time) )
		{
			printf("clock_gettime() - start - error.. exiting.\n");
			break;
		}
		start_time_d = timespec2double(start_time);
		
		// Copy the image into CUDA memory 
		cudaMemcpy(d_img_in_array, h_img_in_array, sizeof(unsigned char)*img_width*img_height, cudaMemcpyHostToDevice);
		
		// Complete the Sobel Transform
		sobel_transform_wrapper(d_img_out_array, d_img_in_array, img_width, img_height, grid, threads);
		cudaThreadSynchronize();
		
		// Copy the transformed image back from the CUDA memory 
		cudaMemcpy(h_img_out_array, d_img_out_array, sizeof(unsigned char)*img_width*img_height, cudaMemcpyDeviceToHost);
		
		// Get end of transform time timing
		if(clock_gettime(CLOCK_REALTIME, &end_time) )
		{
			printf("clock_gettime() - end - error.. exiting.\n");
			break;
		}
		
		if(run_time.tv_nsec != 0)
		{
			// Calculate the timing for nanosleep
			timespec_diff(&start_time, &end_time, &elap_time, true);
			timespec_diff(&elap_time, &run_time, &diff_time, false);
#ifdef DEBUG
			end_time_d = timespec2double(end_time);
			elap_time_d = end_time_d - start_time_d;
			diff_time_d = timespec2double(diff_time);
			printf("DEBUG: Transform runtime: %fms\n       Sleep time:       %fms\n",  elap_time_d, diff_time_d);
#endif	
			if(diff_time.tv_sec < 0 || diff_time.tv_nsec < 0)
			{
				diff_time_d = timespec2double(diff_time);
				printf("TIME OVERRUN by %fms---------\n", -diff_time_d);  
			} else
			{
				// Sleep for time needed to allow for running at known frequency 
				errVal = nanosleep(&diff_time, &end_time);			
				if(errVal == -1)
				{
					printf("\nFreq delay interrupted. Exiting..\n");
					printf("**%d - %s**\n", errno, strerror(errno));
					break;
				}
			}
		}
		
		// Get and calculate end of runtime time
		if(clock_gettime(CLOCK_REALTIME, &end_time) )
		{
			printf("clock_gettime() - end - error.. exiting.\n");
			break;
		}
		end_time_d = timespec2double(end_time);
		elap_time_d = end_time_d - start_time_d;
		printf("     Freq: %f Hz\n", 1000.0/elap_time_d);
	} while(!run_once);
	
	cudaFree(d_img_in_array);
	cudaFree(d_img_out_array);
	
	return NULL;
}

//***************************************************************//
// Transform thread
//***************************************************************//
void *CPU_transform_thread(void * threadp)
{
	// CPU transform local variables
	struct timespec start_time, end_time, elap_time, diff_time;
	int errVal;
	double start_time_d, end_time_d, diff_time_d;

	// Allocate memory
	h_img_out_array = (unsigned char *)malloc(img_width * img_height);
	
	// Infinite loop to allow for power measurement
	do
	{
		// Get start of runtime timing
		if(clock_gettime(CLOCK_REALTIME, &start_time) )
		{
			printf("clock_gettime() - start - error.. exiting.\n");
			break;
		}
		start_time_d = timespec2double(start_time);
        
		CPU_transform(h_img_out_array, h_img_in_array, img_width, img_height);
		
		// Get end of transform time timing
		if(clock_gettime(CLOCK_REALTIME, &end_time) )
		{
			printf("clock_gettime() - end - error.. exiting.\n");
			break;
		}
		
		if(run_time.tv_nsec != 0)
		{
			// Calculate the timing for nanosleep
			timespec_diff(&start_time, &end_time, &elap_time, true);
			timespec_diff(&elap_time, &run_time, &diff_time, false);
#ifdef DEBUG
			end_time_d = timespec2double(end_time);
			elap_time_d = end_time_d - start_time_d;
			diff_time_d = timespec2double(diff_time);
			printf("DEBUG: Transform runtime: %fms\n       Sleep time:       %fms\n",  elap_time_d, diff_time_d);
#endif	
			if(diff_time.tv_sec < 0 || diff_time.tv_nsec < 0)
			{
				diff_time_d = timespec2double(diff_time);
				printf("TIME OVERRUN by %fms---------\n", -diff_time_d);  
			} else
			{
				// Sleep for time needed to allow for running at known frequency 
				errVal = nanosleep(&diff_time, &end_time);			
				if(errVal == -1)
				{
					printf("\nFreq delay interrupted. Exiting..\n");
					printf("**%d - %s**\n", errno, strerror(errno));
					break;
				}
			}
		}
		
		// Get and calculate end of runtime time
		if(clock_gettime(CLOCK_REALTIME, &end_time) )
		{
			printf("clock_gettime() - end - error.. exiting.\n");
			break;
		}
		end_time_d = timespec2double(end_time);
		elap_time_d = end_time_d - start_time_d;
		printf("     Freq: %f Hz\n", 1000.0/elap_time_d);
	} while(!run_once);
	return NULL;
}

//***************************************************************//
// Main function
//***************************************************************//
int main(int argc, char* argv[])
{
	// Local variables
	Options options(argc, argv);
	int errVal = 0;
	bool use_cuda = 0;
	struct sched_param main_param;
	int tempInt, rv;
	char tempChar[50];
	bool wait = true;
	
	// Check input
	if(options.has("help") || options.has("h") || options.has("?")) 
	{
		std::cout << "Usage: " << argv[0] << " [-continuous [-fps=FPS]] [-img=imageFilename]  [-cuda]" << std::endl;
		exit(EXIT_SUCCESS);
	}
	
	printf("\nBEGIN SOBEL TRANSFORM BENCHMARK\n");
	
	if(options.has("img")) 
	{
		imageFilename = options.get<std::string>("img");
	}
	std::cout << "Img set to " << imageFilename << std::endl;
	
	if(options.has("continuous")) 
	{
		run_once = false;
		std::cout << "Continuous mode" << std::endl;
		if(options.has("fps")) 
		{
			freq = options.get<unsigned int>("fps");
			std::cout << "FPS Limiter set at " << freq << std::endl;
		} else 
		{
			freq = 0;
			std::cout << "FPS Unlimited" << std::endl;
		}
	} else 
	{
		run_once = true;
		std::cout << "Single shot mode" << std::endl;
	}
	
	if (options.has("cuda")) 
	{
		std::cout << "Program will use CUDA for transform." << std::endl;
		use_cuda = true;
	} else 
	{
		use_cuda = false;
		std::cout << "Program will use CPU for transform." << std::endl;
	}
	
	if(options.has("nowait"))
   	{
		wait = false;
		std::cout << "Program will not wait for acknowledgement" << std::endl;
	}
	
#ifdef DEBUG
	printf("DEBUG: Begin Program. \n");
#endif
	
	// Initialize CUDA
	if(!InitCUDA()) {
		exit(EXIT_CUDA_ERR);
	};
	
	// Read Input image
	printf("Reading input image...");
	rv = parse_ppm_header((const char *) imageFilename.c_str(), &img_width, &img_height, &img_chan);
	if(!rv) 
	{
		printf("error reading file.\n"); 
		exit(EXIT_IN_IMG_FORMATTING); 
	}
		
	// Check image size
	if(img_width * img_height > MAX_IMG_SZ || img_width * img_height < MIN_IMG_SZ)
	{
		printf("Input image is too large or too small.\n");
		exit(EXIT_IMG_SZ);
	}
	
	h_img_in_array = (unsigned char *)malloc(sizeof(unsigned int) * img_width * img_height);
	readppm(h_img_in_array, &tempInt, 
	tempChar, &tempInt,
	&img_height, &img_width, &img_chan,
	(char *)imageFilename.c_str());
	
	printf("\nWidth:%d  Height:%d\n",img_width, img_height);
	printf("[done]\n");
	
	// Pre-setup for Real-time threads
	mainpid = getpid();
	rt_max_prio = sched_get_priority_max(SCHED_FIFO);
	sched_getparam(mainpid, &main_param);
	main_param.sched_priority = rt_max_prio;
	errVal = sched_setscheduler(mainpid, SCHED_FIFO, &main_param);
	if(errVal < 0) 
	perror("main_param error");
	
	// Setup real-time thread
	pthread_attr_init(&rt_sched_attr);
	pthread_attr_setinheritsched(&rt_sched_attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&rt_sched_attr, SCHED_FIFO);
	rt_param.sched_priority=rt_max_prio-1;
	pthread_attr_setschedparam(&rt_sched_attr, &rt_param);
	
	if (freq) {
		run_time.tv_nsec = (NS_PER_SEC/freq);
		} else {
		run_time.tv_nsec = 0;
	}
	
	// If non-continuous inform user that infinite loop will be entered to allow for power measurement
	if (run_once)
		printf("Program will now transform image once\n");
	else 
		printf("Program will enter an infinite loop, use Ctrl+C to exit program when done.\n");      

	if(wait)
    {
		printf("Press enter to proceed...");
		std::cin.ignore(); // Pause to allow user to read
	}
	   
	// Start Transform
	if (use_cuda)
	{
		// Start real-time thread
		pthread_create( &rt_thread,  // pointer to thread descriptor
		NULL,     				// use default attributes
		CUDA_transform_thread,		// thread function entry point
		&rt_param);				// parameters to pass in
		} else {
		// Start real-time thread
		pthread_create( &rt_thread,  // pointer to thread descriptor
		NULL,     				// use default attributes
		CPU_transform_thread,		// thread function entry point
		&rt_param);				// parameters to pass in
	}
	
	// Let transform thread run //
	
	// Wait for thread to exit
	pthread_join(rt_thread, NULL);
	
	// Write back result
	dump_ppm_data("sobel_out.pgm", img_width, img_height, img_chan, h_img_out_array);
	
	// Free up memory
#ifdef DEBUG
	printf("Cleaning up memory..\n");
#endif
	free(h_img_out_array);
	free(h_img_in_array);

	// Final cleanup and whatnot
	if( run_once && !wait ) // usually only in testing, so output speed of transform to file
	{
		FILE *fp;
		
		fp = fopen(TIMING_FILE, "w");
		
		// Write the elapsed time in microseconds
		fprintf(fp, "**%d**", (int)(elap_time_d*1000));
		
		fclose(fp);
		
		// Print for debugging for now
		printf("**%d**\n", (int)(elap_time_d*1000));
	}
	
	// End the program
	exit(EXIT_SUCCESS);
}
