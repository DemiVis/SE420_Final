// SE420 - SQA - test driver for final project
// Matthew Demi Vis
//
// Requirements under test:
/// Function and feature requirements:	///////////////////////////////////////////////////
// R001 - 	The software shall be capable of transforming any input ppm image utilizing a 
// 			standard Sobel transform.
// R002 - 	The software shall be capable of transforming any input ppm image utilizing a 
//			standard Hough transform. 
// R003 - 	The software shall be capable of transforming any input ppm image utilizing a 
//			standard Pyramidal down- and up-conversion transform.
// R004 - 	The software shall be capable of transforming any command-line specified ppm 
//			image of the aspect ratio 16:9 within the bounds of R009.
// R005 - 	The software shall be capable of transforming the input image using any one of 
//			the included transforms as specified by a command line argument.
/// Performance Requirements	////////////////////////////////////////////////////////////
// R006 - 	The software shall be capable of performing a Sobel transform on the equivalent
//			of 100M pixels per second (e.g. transform 48.225 1920x1080, 2.07M px, images per
//			second) in continuous mode. 
// R007 - 	The software shall be capable of performing a Pyramidal transform (up and then 
//			back down conversion) on the equivalent of 50M pixels per second (e.g. transform 
//			24.113 1920x1080, 2.07M px, images per second) in continuous mode.
// R008 - 	The software shall be capable of performing a Hough Transform on the equivalent 
//			of 2M pixels per second (e.g. transform 2.170 1280x720, 921k px, images per 
//			second) in continuous mode.
// R009 - 	The software shall be capable of transforming images of greater than 100k px 
//			(240p low-def) and less than 8.29M px (4K ultra-high-def).
// R010 - 	The software shall not cause the use of more storage than that which is required 
//			to store the input image(s) and a single output image(s).
/// Error Handling, Recovery and/or Ease of Use Requirements	////////////////////////////
// R011 - 	The software shall be capable of receiving any input arguments without stopping
//			proper execution.
// R012 - 	The software shall notify the user if an input image cannot be found and stop 
//			execution safely.
// R013 - 	The software shall notify the user if an input image is an improperly formatted 
//			file and stop execution safely.
// R014 - 	The software shall not begin running continuously without notifying the user of 
//			a method of stopping execution when desired.
// R015 - 	The software shall not modify the input image for any reason.
//
// Because there is not that many requirements and the code is generally quite small this test
// driver is the only test driver for this project. As such regression testing should include
// running this test and this test only.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ppm.h"

#define	SOBEL_MIN_PX_PER_SEC	(100e6)		// R006
#define PYR_MIN_PX_PER_SEC		( 50e6)		// R007
#define HOUGH_MIN_PX_PER_SEC	(  2e6)		// R008
#define MIN_IMG_PX				(100e3)		// R009
#define MAX_IMG_PX				(826e4)		// R009

#define TOO_LARGE_IMG			/*Some huge ppm file*/
#define TOO_SMALL_IMG			/*Some tiny ppm file*/

#define MED_INPUT_IMG			"beach_160x120.pgm"
#define MED_INPUT_IMG_HT		(120)
#define MED_INPUT_IMG_WT		(160)
#define MED_INPUT_IMG_SZ		(MED_INPUT_IMG_HT * MED_INPUT_IMG_WT)
#define MED_INPUT_IMG_SOBEL		"beach_160x120_sobel.pgm"
#define MED_INPUT_IMG_PYRUP		"beach_160x120_pyrup.pgm"
#define MED_INPUT_IMG_PYRDWN	"beach_160x120_pyrdwn.pgm"
#define MED_INPUT_IMG_HOUGH		"beach_160x120_hough.pgm"

#define MAX_PPM_HR_LEN			( 50)

#define SOBEL_CMD				"./sobel"
#define SOBEL_OUT				"sobel_out.ppm "
#define PYR_CMD					"./pyramid"
#define PYRUP_OUT				"pyrup.ppm "
#define PYRDWN_OUT				"pyrdown.ppm "
#define HOUGH_CMD				"./hough"
#define HOUGH_OUT				"hough.ppm "
#define NO_CMD_OUTPUT			">/dev/null 2>&1"
#define OUTPUT_TO_FILE			">>transformOutputs.txt 2>>transformOutputs.txt"
#define NO_WAIT					" -nowait"
#define USE_CUDA				" -cuda"

#define NUM_TESTS				10

//***************************************************************//
// Convert timespec to double containing time in ms
//***************************************************************//
double timespec2double( struct timespec time_in)
{
	double rv;
	rv = (((double)time_in.tv_sec)*1000)+(((double)time_in.tv_nsec)/1000000);
	return rv;
} 

// Clear Out transform results images
void clear_outputs(void)
{
	system("rm -f "SOBEL_OUT PYRUP_OUT PYRDWN_OUT "pyrdiff.ppm " HOUGH_OUT);
}
		   
// Stubbed out functions
bool compare_ppm(const char * img1_filename, const char * img2_filename)
{
	// Stub
	return true;
}

void copy_ppm(const char *filename, const char *copy_filename)
{
	//stub
}



int main()
{
	struct timespec start_time, end_time;
	float elap_time, pxPerSec;
	int passCount = 0, failCount = 0;
	bool tempbool;
	char copy_file[50], tempStr[100];
	
	// Save off input image original
	sprintf(copy_file, "%s", MED_INPUT_IMG);
	strncpy(tempStr, &copy_file[strnlen(copy_file, 25)-4], 4); // copy just file extension
	copy_file[strnlen(copy_file, 25)-4] = 0; // Terminate string without extension
	strncat(copy_file, "_orignal", 8); // add original to end of filename
	strncat(copy_file, tempStr, 4); // put extension back on
	copy_ppm(MED_INPUT_IMG, copy_file);

	//// Test that the Transforms actually transform correctly
	printf("Feature Testing-------------------------------------\n");
	
	// Run Sobel Transform - R001
	printf("-----Sobel Transform-----\n");
	system(SOBEL_CMD" -img="MED_INPUT_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	if( compare_ppm(SOBEL_OUT, MED_INPUT_IMG_SOBEL) )
	{
		printf("Sobel transform output matches expected output.\n");
		printf("                                     R001 PASSED\n");
		passCount++;
	}
	else
	{
		printf("Sobel transform does not match expected output.\n");
		printf("                                     R001 FAILED\n");
		failCount++;
	}
	
	// Run Hough Transform - R002
	printf("-----Hough Transform-----\n");
	system(HOUGH_CMD" -img="MED_INPUT_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	if( compare_ppm(HOUGH_OUT, MED_INPUT_IMG_HOUGH) )
	{
		printf("Hough transform output matches expected output.\n");
		printf("                                     R002 PASSED\n");
		passCount++;
	}
	else
	{
		printf("Hough transform does not match expected output.\n");
		printf("                                     R002 FAILED\n");
		failCount++;
	}
	
	// Run Pyramidal Transform - R003
	printf("-----Pyramidal Transforms-----\n");
	system(PYR_CMD" -img="MED_INPUT_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	if( compare_ppm(PYRUP_OUT, MED_INPUT_IMG_PYRUP) )
	{
		printf("Pyramidal Up transform output matches expected output.\n");
		tempbool = true;
		passCount++;
	}
	else
	{
		printf("Pyramidal Up transform does not match expected output.\n");
		tempbool = false;
		failCount++;
	}
	if( compare_ppm(PYRDWN_OUT, MED_INPUT_IMG_PYRDWN) )
	{
		printf("Pyramidal Down transform output matches expected output.\n");
		if(tempbool) printf("                                     R003 PASSED\n");
		else 		 printf("                                     R003 FAILED\n");
		passCount++;
	}
	else
	{
		printf("Pyramidal Down transform does not match expected output.\n");
		printf("                                     R003 FAILED\n");
		failCount++;
	}
	
	//// Performance Testing Section
	printf("Performance Testing---------------------------------\n");
	// Sobel Performance Testing - R006
	// Call ./SOBEL_CMD with normal input image and time it
	clock_gettime(CLOCK_REALTIME, &start_time);
	system(SOBEL_CMD" -img="MED_INPUT_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	clock_gettime(CLOCK_REALTIME, 	&end_time);
	elap_time = timespec2double(end_time) - timespec2double(start_time);
	printf("Sobel Elapsed Time:     %.3f ms\n", elap_time);
	pxPerSec = MED_INPUT_IMG_SZ*1000/elap_time;
	printf("                       (%.0f px/s)", pxPerSec);
	if(pxPerSec < SOBEL_MIN_PX_PER_SEC)
	{
		printf(" R006 FAILED\n");
		failCount++;
	}
	else
	{
		printf(" R006 PASSED\n");
		passCount++;
	}
	
	// R007, R003 - Pyramidal Performance Testing
	clock_gettime(CLOCK_REALTIME, &start_time);
	system(PYR_CMD" -img="MED_INPUT_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	clock_gettime(CLOCK_REALTIME, 	&end_time);
	elap_time = timespec2double(end_time) - timespec2double(start_time);
	printf("Pyramidal Elapsed Time: %.3f ms\n",elap_time);
	pxPerSec = MED_INPUT_IMG_SZ*1000/elap_time;
	printf("                       (%.0f px/s)", pxPerSec);
	if(pxPerSec < PYR_MIN_PX_PER_SEC)
	{
		printf(" R007 FAILED\n");
		failCount++;
	}
	else
	{
		printf(" R007 PASSED\n");
		passCount++;
	}
	
	// R008, R002 - Hough Performance Testing
	clock_gettime(CLOCK_REALTIME, &start_time);
	system(HOUGH_CMD" -img="MED_INPUT_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	clock_gettime(CLOCK_REALTIME, 	&end_time);
	elap_time = timespec2double(end_time) - timespec2double(start_time);
	printf("Hough Elapsed Time:     %.3f ms\n", elap_time);
	pxPerSec = MED_INPUT_IMG_SZ*1000/elap_time;
	printf("                       (%.0f px/s)", pxPerSec);
	if(pxPerSec < HOUGH_MIN_PX_PER_SEC)
	{
		printf(" R008 FAILED\n");
		failCount++;
	}
	else
	{
		printf(" R008 PASSED\n");
		passCount++;
	}
	
	// Test error handling capabilities
	printf("Error Handling Testing------------------------------\n");
	
	printf("Original image check..\n");
	if(compare_ppm(MED_INPUT_IMG, copy_file))
	{
		printf(" ..Input image has not changed\n");
		printf("                                     R015 PASSED\n");
		passCount++;
	}
	else
	{
		
		printf(" ..Input image has changed\n");
		printf("                                   R015 FAILED\n");
		failCount++;
	}
	
	// Wrap it up
	printf("TEST COMPLETE---------------------------------------\n");
	printf("	%d test(s) passed\n", passCount);
	printf("	%d test(s) failed\n", failCount);
}
	