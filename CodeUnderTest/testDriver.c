// SE420 - SQA - test driver for final project
// Matthew Demi Vis
//
// Requirements under test:
/// Function and feature requirements:	///////////////////////////////////////////////////
// R001 - 	The software shall be capable of transforming any input ppm image utilizing a 
// 			standard Sobel transform.
//			SOBEL_OUTPUT test
// R002 - 	The software shall be capable of transforming any input ppm image utilizing a 
//			standard Hough transform. 
//			HOUGH_OUTPUT test
// R003 - 	The software shall be capable of transforming any input ppm image utilizing a 
//			standard Pyramidal down- and up-conversion transform.
//			PYRUP_OUTPUT and PYRDWN_OUTPUT test
// R004 - 	The software shall be capable of transforming any command-line specified ppm 
//			image of the aspect ratio 16:9 within the bounds of R009.
//			
// R005 - 	The software shall be capable of transforming the input image using any one of 
//			the included transforms as specified by a command line argument.
//			
/// Performance Requirements	////////////////////////////////////////////////////////////
// R006 - 	The software shall be capable of performing a Sobel transform on the equivalent
//			of 100M pixels per second (e.g. transform 48.225 1920x1080, 2.07M px, images per
//			second) in continuous mode. 
//			SOBEL_PERF test
// R007 - 	The software shall be capable of performing a Pyramidal transform (up and then 
//			back down conversion) on the equivalent of 50M pixels per second (e.g. transform 
//			24.113 1920x1080, 2.07M px, images per second) in continuous mode.
//			PYR_PERF test
// R008 - 	The software shall be capable of performing a Hough Transform on the equivalent 
//			of 2M pixels per second (e.g. transform 2.170 1280x720, 921k px, images per 
//			second) in continuous mode.
//			HOUGH_PERF test
// R009 - 	The software shall be capable of transforming images of greater than 100k px 
//			(240p low-def) and less than 8.29M px (4K ultra-high-def).
//			LARGE_IMG and SMALL_IMG tests
// R010 - 	The software shall not cause the use of more storage than that which is required 
//			to store the input image(s) and a single output image(s).
//			
/// Error Handling, Recovery and/or Ease of Use Requirements	////////////////////////////
// R011 - 	The software shall be capable of receiving any input arguments without stopping
//			proper execution.
//			
// R012 - 	The software shall notify the user if an input image cannot be found and stop 
//			execution safely.
//			
// R013 - 	The software shall notify the user if an input image is an improperly formatted 
//			file and stop execution safely.
//			
// R014 - 	The software shall not begin running continuously without notifying the user of 
//			a method of stopping execution when desired.
//			
// R015 - 	The software shall not modify the input image for any reason.
//			ORIG_IMG test
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

#define TOO_LARGE_IMG			"mount_xl.pgm" // TODO: check if this is actually too large of an image
#define TOO_SMALL_IMG			"mount_xs.pgm" // TODO: check if this is actually too small of an image

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

#define US_PER_SEC				1000000.0

#define SOBEL_TIMING_FILE	"sobel_timing.txt"
#define HOUGH_TIMING_FILE	"hough_timing.txt"
#define PYR_TIMING_FILE		"pyr_timing.txt"

// Transform Return Codes
#define EXIT_SUCCESS				0
#define EXIT_UNKOWN_ERROR			-1
#define EXIT_IMG_SZ					1
#define EXIT_IN_IMG_NOT_FOUND		2
#define EXIT_IN_IMG_FORMATTING		3

#define DEBUG

enum tests_t {	SOBEL_OUTPUT,
				HOUGH_OUTPUT,
				PYRUP_OUTPUT,
				PYRDWN_OUTPUT,
				SOBEL_PERF,
				PYR_PERF,
				HOUGH_PERF,
			  	LARGE_IMG,
			  	SMALL_IMG,
				ORIG_IMG,
				NUM_TESTS	};
char test_t_str[][27] = {  "Sobel Output Test",
						   "Hough Output Test",
						   "Pyramidal Up Output Test",
						   "Pyramidal Down Output Test",
						   "Sobel Performance Test",
						   "Pyramidal Performance Test",
						   "Hough Performance Test",
						   "Large Image Test",
						   "Small Image Test",
						   "Original Image Test",
						   ""};

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
int compare_ppm(const char * img1_filename, const char * img2_filename, const char *diff_filename)
{
	unsigned char img1[MED_INPUT_IMG_SZ], img2[MED_INPUT_IMG_SZ], diff[MED_INPUT_IMG_SZ];
	char temp[512];
	int tempInt;
	unsigned tempUnsigned;
	int rv = 1;
	
	// Read input files
	if( readppm(img1, &tempInt, temp, &tempInt, &tempUnsigned, &tempUnsigned, &tempUnsigned,
             (char *)img1_filename) != true)
	{
#ifdef DEBUG
		printf("error reading ppm 1 for comparison\n");
#endif
		return -1;
	}
	if( readppm(img2, &tempInt, temp, &tempInt, &tempUnsigned, &tempUnsigned, &tempUnsigned,
             (char *)img2_filename) != true)
	{
#ifdef DEBUG
		printf("error reading ppm 2 for comparison\n");
#endif
		return -2;
	}
	
	// Do the comparison
	for(int i = 0; i < MED_INPUT_IMG_SZ; i++)
	{
		if(img1[i] != img2[i])
		{
			diff[i] = 255;
			rv = 0;
		}
		else
			diff[i] = 0;
	}
	
	// Dump diff pgm if desired
	if(diff_filename != NULL)
		dump_ppm_data(diff_filename, MED_INPUT_IMG_WT, MED_INPUT_IMG_HT, 1, diff);
	
	return rv;
}

// create a copy of a ppm
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

// get the timing out of a timing file created by a transform
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
	bool temp_bool;
	int temp_int;
	char copy_file[64], tempStr[64];
	char fail_string[NUM_TESTS][128];
	bool test_passed[NUM_TESTS];
	
	// Save off input image original
	sprintf(copy_file, "%s", INPUT_IMG_FOLDER MED_INPUT_IMG);
	strncpy(tempStr, &copy_file[strnlen(copy_file, 25)-3], 4); // copy just file extension
	copy_file[strnlen(copy_file, 64)-3] = 0; // Terminate string without extension
	strncat(copy_file, "_orignal", 8); // add original to end of filename
	strncat(copy_file, tempStr, 4); // put extension back on
	copy_ppm(INPUT_IMG_FOLDER MED_INPUT_IMG, copy_file);

	//// Test that the Transforms actually transform correctly
	printf("Feature Testing-------------------------------------\n");
	
	// Run Sobel Transform - R001
	printf("-----Sobel Transform-----\n");
	system(SOBEL_CMD" -img="INPUT_IMG_FOLDER MED_INPUT_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	printf("Sobel complete\n");
	if( (temp_int = compare_ppm(SOBEL_OUT, MED_INPUT_IMG_SOBEL, "sobel_diff.pgm") ) > 0 )
	{
		printf("Sobel transform output matches expected output.\n");
		printf("                                     R001 PASSED\n");
		test_passed[SOBEL_OUTPUT] = true;
	}
	else
	{
		printf("Sobel transform does not match expected output.\n");
		printf("                                     R001 FAILED\n");
		test_passed[SOBEL_OUTPUT] = false;
		if(temp_int == 0)
			sprintf(fail_string[SOBEL_OUTPUT],"benchmark output does not match expected output");
		else if(temp_int == -1)
			sprintf(fail_string[SOBEL_OUTPUT],"could not open the transform output");
		else if(temp_int == -2)
			sprintf(fail_string[SOBEL_OUTPUT],"could not open expected output");
	}
	
	// Run Hough Transform - R002
	printf("-----Hough Transform-----\n");
	system(HOUGH_CMD" -img="INPUT_IMG_FOLDER MED_INPUT_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	if( (temp_int = compare_ppm(HOUGH_OUT, MED_INPUT_IMG_HOUGH, "hough_diff.pgm") ) > 0 )
	{
		printf("Hough transform output matches expected output.\n");
		printf("                                     R002 PASSED\n");
		test_passed[HOUGH_OUTPUT] = true;
	}
	else
	{
		printf("Hough transform does not match expected output.\n");
		printf("                                     R002 FAILED\n");
		test_passed[HOUGH_OUTPUT] = false;
		if(temp_int == 0)
			sprintf(fail_string[HOUGH_OUTPUT],"benchmark output does not match expected output");
		else if(temp_int == -1)
			sprintf(fail_string[HOUGH_OUTPUT],"could not open the transform output");
		else if(temp_int == -2)
			sprintf(fail_string[HOUGH_OUTPUT],"could not open expected output");
	}
	
	// Run Pyramidal Transform - R003
	printf("-----Pyramidal Transforms-----\n");
	system(PYR_CMD" -img="INPUT_IMG_FOLDER MED_INPUT_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	if( (temp_int = compare_ppm(PYRUP_OUT, MED_INPUT_IMG_PYRUP, "pyrup_diff.pgm") ) > 0)
	{
		printf("Pyramidal Up transform output matches expected output.\n");
		temp_bool = true;
		test_passed[PYRUP_OUTPUT] = true;
	}
	else
	{
		printf("Pyramidal Up transform does not match expected output.\n");
		temp_bool = false;
		test_passed[PYRUP_OUTPUT] = false;
		if(temp_int == 0)
			sprintf(fail_string[PYRUP_OUTPUT],"benchmark output does not match expected output");
		else if(temp_int == -1)
			sprintf(fail_string[PYRUP_OUTPUT],"could not open the transform output");
		else if(temp_int == -2)
			sprintf(fail_string[PYRUP_OUTPUT],"could not open expected output");
	}
	if( (temp_int = compare_ppm(PYRDWN_OUT, MED_INPUT_IMG_PYRDWN, "prydwn_diff.pgm") ) > 0)
	{
		printf("Pyramidal Down transform output matches expected output.\n");
		if(temp_bool) printf("                                     R003 PASSED\n");
		else 	      printf("                                     R003 FAILED\n");
		test_passed[PYRDWN_OUTPUT] = true;
	}
	else
	{
		printf("Pyramidal Down transform does not match expected output.\n");
		printf("                                     R003 FAILED\n");
		test_passed[PYRDWN_OUTPUT] = false;
		if(temp_int == 0)
			sprintf(fail_string[PYRDWN_OUTPUT],"benchmark output does not match expected output");
		else if(temp_int == -1)
			sprintf(fail_string[PYRDWN_OUTPUT],"could not open the transform output");
		else if(temp_int == -2)
			sprintf(fail_string[PYRDWN_OUTPUT],"could not open expected output");
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
		test_passed[SOBEL_PERF] = false;
		sprintf(fail_string[SOBEL_PERF],"actual pixels per sec %.0f too low",SOBEL_MIN_PX_PER_SEC - pxPerSec);
	}
	else
	{
		printf(" R006 PASSED\n");
		test_passed[SOBEL_PERF] = true;
		
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
		test_passed[PYR_PERF] = false;
		sprintf(fail_string[PYR_PERF],"actual pixels per sec %.0f too low",SOBEL_MIN_PX_PER_SEC - pxPerSec);
	}
	else
	{
		printf(" R007 PASSED\n");
		test_passed[PYR_PERF] = true;
		
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
		test_passed[HOUGH_PERF] = false;
		sprintf(fail_string[HOUGH_PERF],"actual pixels per sec %.0f too low",SOBEL_MIN_PX_PER_SEC - pxPerSec);
	}
	else
	{
		printf(" R008 PASSED\n");
		test_passed[HOUGH_PERF] = true;
		
		// Remove the timing file if passed since don't need for cause analysis
		system("rm -f "HOUGH_TIMING_FILE);
	}
	
	// Test error handling capabilities
	printf("Error Handling Testing------------------------------\n");
	// Check when input is an image that's too large
	printf("Large Image Check..\n");
	temp_int = system(SOBEL_CMD" -img="INPUT_IMG_FOLDER TOO_LARGE_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	if(WEXITSTATUS(temp_int) == EXIT_IMG_SZ)
	{
		printf("  Sobel Passed!\n");
		fail_string[LARGE_IMG][0] = 0; // Explicity set fail string to empty
		temp_bool = true;
	}
	else
	{
		printf("  Sobel Failed!\n");
		test_passed[LARGE_IMG] = false;
		sprintf(fail_string[LARGE_IMG], "Sobel returned code %d. ", temp_int);
	}
	temp_int = system(PYR_CMD" -img="INPUT_IMG_FOLDER TOO_LARGE_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	if(WEXITSTATUS(temp_int) == EXIT_IMG_SZ)
	{
		printf("  Pyramidal Passed!\n");
		temp_bool &= true;
	}
	else
	{
		printf("  Pyramidal Failed!\n");
		test_passed[LARGE_IMG] = false;
		sprintf(&fail_string[LARGE_IMG][strnlen(fail_string[LARGE_IMG],23)], "Pyramidal returned code %d. ", temp_int);
	}
	temp_int = system(HOUGH_CMD" -img="INPUT_IMG_FOLDER TOO_LARGE_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	if(WEXITSTATUS(temp_int) == EXIT_IMG_SZ)
	{
		printf("  Hough Passed!");
		temp_bool &= true;
	}
	else
	{
		printf("  Hough Failed!");
		test_passed[LARGE_IMG] = false;
		sprintf(&fail_string[LARGE_IMG][strnlen(fail_string[LARGE_IMG],50)], "Hough returned code %d. ", temp_int);
	}
	
	// Check when input is an image that's too small
	printf("Small Image Check..\n");
	temp_int = system(SOBEL_CMD" -img="INPUT_IMG_FOLDER TOO_SMALL_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	
	if(WEXITSTATUS(temp_int) == EXIT_IMG_SZ)
	{
		printf("  Sobel Passed!\n");
		fail_string[SMALL_IMG][0] = 0; // Explicity set fail string to empty
		temp_bool &= true;
	}
	else
	{
		printf("  Sobel Failed!\n");
		test_passed[SMALL_IMG] = false;
		sprintf(fail_string[SMALL_IMG], "Sobel returned code %d. ", temp_int);
	}
	temp_int = system(PYR_CMD" -img="INPUT_IMG_FOLDER TOO_SMALL_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	if(WEXITSTATUS(temp_int) == EXIT_IMG_SZ)
	{
		printf("  Pyramidal Passed!\n");
		temp_bool &= true;
	}
	else
	{
		printf("  Pyramidal Failed!\n");
		test_passed[SMALL_IMG] = false;
		sprintf(&fail_string[SMALL_IMG][strnlen(fail_string[SMALL_IMG],25)], "Pyramidal returned code %d. ", temp_int);
	}
	temp_int = system(HOUGH_CMD" -img="INPUT_IMG_FOLDER TOO_SMALL_IMG NO_WAIT USE_CUDA OUTPUT_TO_FILE);
	if(WEXITSTATUS(temp_int) == EXIT_IMG_SZ)
	{
		printf("  Hough Passed!");
		temp_bool &= true;
	}
	else
	{
		printf("  Hough Failed!");
		test_passed[SMALL_IMG] = false;
		sprintf(&fail_string[SMALL_IMG][strnlen(fail_string[SMALL_IMG],54)], "Hough returned code %d. ", temp_int);
	}
	if(temp_bool)
		printf("\t\tR009 PASSED\n");
	else
		printf("\t\tR009 FAILED\n");
	
	printf("Original image check..\n");
	if((temp_int = compare_ppm(INPUT_IMG_FOLDER MED_INPUT_IMG, copy_file, NULL) ) > 0)
	{
		printf(" ..Input image has not changed\n");
		printf("                                     R015 PASSED\n");
		test_passed[ORIG_IMG] = true;
	}
	else
	{
		
		printf(" ..Input image has changed\n");
		printf("                                   R015 FAILED\n");
		test_passed[ORIG_IMG] = false;
		if(temp_int == 0)
			sprintf(fail_string[ORIG_IMG],"input no longer matches original image");
		else if(temp_int == -1)
			sprintf(fail_string[ORIG_IMG],"could not open the input image");
		else if(temp_int == -2)
			sprintf(fail_string[ORIG_IMG],"could not open the original copy");
	}
	
	// Wrap it up
	printf("TEST COMPLETE---------------------------------------\n");
	printf("\nREGRESSION TEST REPORT-----------------------------\n");
	temp_bool = false; // any test failed
	for(int i = 0; i < NUM_TESTS; i++)
	{
		if(test_passed[i])
		{
			printf("  %s passed.\n",test_t_str[i]);
		}
		else
		{
			printf("  %s failed because \"%s\".\n",test_t_str[i],fail_string[i]);
			temp_bool = true;
		}
	}
	if(temp_bool) // if any test failed
		printf("Some tests failed. Full regression test FAILED.\n");
	else
		printf("All tests passed. Full regression test PASSED.\n");
}
	