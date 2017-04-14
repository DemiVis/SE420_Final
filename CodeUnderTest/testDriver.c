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

#include <errno.h>
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

#define INPUT_IMG_FOLDER		"../Test_images/"

#define TOO_LARGE_IMG			/*Some huge ppm file*/
#define TOO_SMALL_IMG			/*Some tiny ppm file*/

#define MED_INPUT_IMG			"mount_m.pgm"
#define MED_INPUT_IMG_HT		(480)
#define MED_INPUT_IMG_WT		(640)
#define MED_INPUT_IMG_SZ		(MED_INPUT_IMG_HT * MED_INPUT_IMG_WT)
#define MED_INPUT_IMG_SOBEL		"sobel_expected.pgm"
#define MED_INPUT_IMG_PYRUP		"pyrup_expected.pgm"
#define MED_INPUT_IMG_PYRDWN	"pyrdwn_expected.pgm"
#define MED_INPUT_IMG_HOUGH		"hough_expected.pgm"

#define MAX_PPM_HR_LEN			( 50)

#define SOBEL_CMD				"./sobel"
#define SOBEL_OUT				"sobel_out.pgm"
#define PYR_CMD					"./pyramid"
#define PYRUP_OUT				"pyrup.pgm"
#define PYRDWN_OUT				"pyrdown.pgm"
#define HOUGH_CMD				"./hough"
#define HOUGH_OUT				"hough.pgm"
#define NO_CMD_OUTPUT			">/dev/null 2>&1"
#define OUTPUT_TO_FILE			">>transformOutputs.txt 2>>transformOutputs.txt"
#define NO_WAIT					" -nowait"
#define USE_CUDA				" -cuda"

#define NUM_TESTS				10

#define US_PER_SEC				1000000.0

#define SOBEL_TIMING_FILE	"sobel_timing.txt"
#define HOUGH_TIMING_FILE	"hough_timing.txt"
#define PYR_TIMING_FILE		"pyr_timing.txt"

#define DEBUG
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
	system("rm -f "SOBEL_OUT PYRUP_OUT PYRDWN_OUT HOUGH_OUT);
}
		   
// compare ppm pixel by pixel and create diff image
// Only works for medium image sized pgm images
bool compare_ppm(const char * img1_filename, const char * img2_filename, const char *diff_filename)
{
	unsigned char img1[MED_INPUT_IMG_SZ], img2[MED_INPUT_IMG_SZ], diff[MED_INPUT_IMG_SZ];
	char temp[512];
	int tempInt;
	unsigned tempUnsigned;
	bool rv = true;
	
	// Read input files
	if( readppm(img1, &tempInt, temp, &tempInt, &tempUnsigned, &tempUnsigned, &tempUnsigned,
             (char *)img1_filename) != true)
	{
#ifdef DEBUG
		printf("error reading ppm 1 for comparison\n");
#endif
		return false;
	}
	if( readppm(img2, &tempInt, temp, &tempInt, &tempUnsigned, &tempUnsigned, &tempUnsigned,
             (char *)img2_filename) != true)
	{
#ifdef DEBUG
		printf("error reading ppm 2 for comparison\n");
#endif
		return false;
	}
	
	// Do the comparison
	for(int i = 0; i < MED_INPUT_IMG_SZ; i++)
	{
		if(img1[i] != img2[i])
		{
			diff[i] = 255;
			rv = false;
		}
		else
			diff[i] = 0;
	}
	
	// Dump diff pgm if desired
	if(diff_filename != NULL)
		dump_ppm_data(diff_filename, MED_INPUT_IMG_WT, MED_INPUT_IMG_HT, 1, diff);
	
	return rv;
	//return true; // stub until can be fixed
}

void copy_ppm(const char *filename, const char *copy_filename)
{
	unsigned char temp[MED_INPUT_IMG_SZ];
	int tempInt;
	unsigned tempUnsigned;
	char tempStr[100];
	
	if( readppm(temp, &tempInt, tempStr, &tempInt, &tempUnsigned, &tempUnsigned, &tempUnsigned,
             (char *)filename) != true)
		printf("Problems w copy_ppm\n");
	dump_ppm_data(copy_filename, MED_INPUT_IMG_WT, MED_INPUT_IMG_HT, 1, temp);
}

long int get_transform_timing(const char *filename)
{
	char tempChar;
	long int rv;
	FILE *fp = fopen(filename, "r");
	
	// Get passed the '**" at the beginning
	tempChar = fgetc(fp);
	if(tempChar != '*') return -1;
	tempChar = fgetc(fp);
	if(tempChar != '*') return -1;
	
	// Get the number
	fscanf(fp,"%ld",&rv);
	
	// Most basic check for correctness
	if(rv < 0) return -1;
	
	return rv;
}



int main()
{
	//struct timespec start_time, end_time;
	long int elap_time, pxPerSec;
	int passCount = 0, failCount = 0;
	bool tempbool;
	char copy_file[64], tempStr[100];
	
	// Save off input image original
	sprintf(copy_file, "%s", INPUT_IMG_FOLDER MED_INPUT_IMG);
	strncpy(tempStr, &copy_file[strnlen(copy_file, 25)-4], 4); // copy just file extension
	copy_file[strnlen(copy_file, 64)-4] = 0; // Terminate string without extension
	strncat(copy_file, "_orignal", 8); // add original to end of filename
	strncat(copy_file, tempStr, 4); // put extension back on
	copy_ppm(INPUT_IMG_FOLDER MED_INPUT_IMG, copy_file);

	//// Test that the Transforms actually transform correctly
	printf("Feature Testing-------------------------------------\n");
	
	// Run Sobel Transform - R001
	printf("-----Sobel Transform-----\n");
	system(SOBEL_CMD" -img="INPUT_IMG_FOLDER MED_INPUT_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	printf("Sobel complete\n");
	if( compare_ppm(SOBEL_OUT, MED_INPUT_IMG_SOBEL, "sobel_diff.pgm") )
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
	system(HOUGH_CMD" -img="INPUT_IMG_FOLDER MED_INPUT_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	if( compare_ppm(HOUGH_OUT, MED_INPUT_IMG_HOUGH, "hough_diff.pgm") )
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
	system(PYR_CMD" -img="INPUT_IMG_FOLDER MED_INPUT_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	if( compare_ppm(PYRUP_OUT, MED_INPUT_IMG_PYRUP, "pyrup_diff.pgm") )
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
	if( compare_ppm(PYRDWN_OUT, MED_INPUT_IMG_PYRDWN, "prydwn_diff.pgm") )
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
	system(SOBEL_CMD" -img="INPUT_IMG_FOLDER MED_INPUT_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	elap_time = get_transform_timing(SOBEL_TIMING_FILE);
	printf("Sobel Elapsed Time:     %ld us\n", elap_time);
	pxPerSec = MED_INPUT_IMG_SZ*US_PER_SEC/elap_time;
	printf("                       (%ld px/s)", pxPerSec);
	if(pxPerSec < SOBEL_MIN_PX_PER_SEC)
	{
		printf(" R006 FAILED\n");
		failCount++;
	}
	else
	{
		printf(" R006 PASSED\n");
		passCount++;
		
		// Remove the timing file if passed since don't need for cause analysis
		system("rm -f "SOBEL_TIMING_FILE);
	}
	
	// R007, R003 - Pyramidal Performance Testing
	elap_time = (float) system(PYR_CMD" -img="INPUT_IMG_FOLDER  MED_INPUT_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	elap_time = get_transform_timing(PYR_TIMING_FILE);
	printf("Pyramidal Elapsed Time: %ld us\n",elap_time);
	pxPerSec = MED_INPUT_IMG_SZ*US_PER_SEC/elap_time;
	printf("                       (%ld px/s)", pxPerSec);
	if(pxPerSec < PYR_MIN_PX_PER_SEC)
	{
		printf(" R007 FAILED\n");
		failCount++;
	}
	else
	{
		printf(" R007 PASSED\n");
		passCount++;
		
		// Remove the timing file if passed since don't need for cause analysis
		system("rm -f "PYR_TIMING_FILE);
	}
	
	// R008, R002 - Hough Performance Testing
	system(HOUGH_CMD" -img="INPUT_IMG_FOLDER MED_INPUT_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	elap_time = get_transform_timing(HOUGH_TIMING_FILE);
	printf("Hough Elapsed Time:     %ld us\n", elap_time);
	pxPerSec = MED_INPUT_IMG_SZ*US_PER_SEC/elap_time;
	printf("                       (%ld px/s)", pxPerSec);
	if(pxPerSec < HOUGH_MIN_PX_PER_SEC)
	{
		printf(" R008 FAILED\n");
		failCount++;
	}
	else
	{
		printf(" R008 PASSED\n");
		passCount++;
		
		// Remove the timing file if passed since don't need for cause analysis
		system("rm -f "HOUGH_TIMING_FILE);
	}
	
	// Test error handling capabilities
	printf("Error Handling Testing------------------------------\n");
	
	printf("Original image check..\n");
	if(compare_ppm(INPUT_IMG_FOLDER MED_INPUT_IMG, copy_file, NULL))
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
	