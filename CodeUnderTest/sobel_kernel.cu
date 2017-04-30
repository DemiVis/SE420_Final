//*****************************************************************************************//
//  sobel_kernel.cu - CUDA Hough Transform Benchmark
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
//  running Ubuntu 14.04 
//	
//	Please use at your own risk. We are sharing so that other researchers and developers can 
//	recreate our results and make suggestions to improve and extend the benchmarks over time.
//
//*****************************************************************************************//

#include <stdio.h>
#include <assert.h>

#define MAXRGB	 	255

//***************************************************************//
// Sobel transform using CUDA hardware
//***************************************************************//
__global__ void CUDA_transform(unsigned char *img_out, unsigned char *img_in, unsigned int width, unsigned int height){
	int x,y;
	unsigned char LUp,LCnt,LDw,RUp,RCnt,RDw;
	int pixel;
	
	x=blockDim.x*blockIdx.x+threadIdx.x;
	y=blockDim.y*blockIdx.y+threadIdx.y;
	
	if( x<width && y<height )
	{
		LUp = (x-1>=0 && y-1>=0) ? img_in[(x-1)+(y-1)*width] : 0;
		LCnt= (x-1>=0)           ? img_in[(x-1)+y*width]:0;
		LDw = (x-1>=0 && y+1<height) ? img_in[(x-1)+(y+1)*width] : 0;
		RUp = (x+1<width && y-1>=0)  ? img_in[(x+1)+(y-1)*width] : 0;
		RCnt= (x+1<width)            ? img_in[(x+1)+y*width] : 0;
		RDw = (x+1<width && y+1<height) ? img_in[(x+1)+(y+1)*width] : 0;
		pixel = -1*LUp  + 1*RUp +
		-2*LCnt + 2*RCnt +
		-1*LDw  + 1*RDw;
		pixel = (pixel<0) ? 0 : pixel;
		pixel = (pixel>MAXRGB) ? MAXRGB : pixel;
		img_out[x+y*width] = pixel;
	}
}

//***************************************************************//
// Sobel transform using the CPU
//***************************************************************//
void CPU_transform(unsigned char *img_out, unsigned char *img_in, unsigned int width, unsigned int height) {
	unsigned char LUp,LCnt,LDw,RUp,RCnt,RDw;
	int pixel;
	for(int y=0; y<height; y++)
	{
		for(int x=0; x<width; x++)
		{
			#ifdef DEBUG
				printf("Pixel X:%d Y:%d\n",x,y);
			#endif
			assert(x+(y*width)<width*height);
			LUp = (x-1>=0 && y-1>=0)? img_in[(x-1)+(y-1)*width]:0;
			LCnt= (x-1>=0)? img_in[(x-1)+y*width]:0;
			LDw = (x-1>=0 && y+1<height)? img_in[(x-1)+(y+1)*width]:0;
			RUp = (x+1<width && y-1>=0)? img_in[(x+1)+(y-1)*width]:0;
			RCnt= (x+1<width)? img_in[(x+1)+y*width]:0;
			RDw = (x+1<width && y+1<height)? img_in[(x+1)+(y+1)*width]:0;
			pixel = -1*LUp  + 1*RUp + -2*LCnt + 2*RCnt + -1*LDw  + 1*RDw;
			pixel=(pixel<0)?0:pixel;
			pixel=(pixel>MAXRGB)?MAXRGB:pixel;
			img_out[x+y*width]=pixel;
			#ifdef DEBUG
				printf("\r%5.2f",100*(float)(y*width+x)/(float)(width*height-1));            
			#endif
		}
	}
#ifdef DEBUG
	printf("\n");
#endif
}
