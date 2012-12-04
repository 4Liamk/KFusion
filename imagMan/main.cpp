#include "lodepng.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <omp.h>
#include <math.h>
#include "img-out.h"
#include <assert.h>
#include <sys/time.h>
#include <CL/cl.h>
#include <Magick++.h>
#define OUTPUT 1
#define DEPTH 4 

//using namespace Magick;
double gettime()
{
	struct timeval start;
	gettimeofday(&start,NULL);
	return (double) start.tv_sec + start.tv_usec/1000000.0;
}

void horizontal(char * image)
{
	{
		Image * dash = new_ImageFromFile(image);
		Image * dash2 = new_ImageFromFile(image);
		clFinish(queue);
		double start = gettime(); 
			transfer(dash,1);
			transfer(dash2,1);
			clFinish(queue);
			double startinner = gettime();	
				RGBinvert(dash);
				RGBinvert(dash2);
				clFinish(queue);
			double endinner = gettime();	
			transfer(dash,0);
			transfer(dash2,0);
			clFinish(queue);
		double end = gettime();
		printf("%f\t%f\t",end - start, endinner- startinner);
	
		if(OUTPUT)
		{
			save(dash,"hfuse1.png");
			save(dash2,"hfuse2.png");
		}
	
		del_Image(dash);
		del_Image(dash2);
	}
	
	{
		Image * dash = new_ImageFromFile(image);
		Image * dash2 = new_ImageFromFile(image);
		clFinish(queue);
		double start = gettime(); 
			transfer(dash,1);
			transfer(dash2,1);
			clFinish(queue);
			double startinner = gettime();	
				#pragma starthfuse
				RGBinvert(dash);
				RGBinvert(dash2);
				#pragma endfuse
				clFinish(queue);
			double endinner = gettime();	
			transfer(dash,0);
			transfer(dash2,0);
			clFinish(queue);
		double end = gettime();
		printf("%f\t%f\t",end - start, endinner- startinner);
		
		if(OUTPUT)
		{
			save(dash,"hfuse3.png");
			save(dash2,"hfuse4.png");
		}
		del_Image(dash);
		del_Image(dash2);
	}
}

void bestCase3(char * image)
{
	Image * dash3 = new_ImageFromFile(image);	
	clFinish(queue);
	double start = gettime();
		transfer(dash3,1);
		clFinish(queue);
		double startinner = gettime();
			bestCase(dash3, dash3->width*1.1,dash3->height*1.1,.20);
			clFinish(queue);
		double endinner = gettime();
		transfer(dash3,0);
		clFinish(queue);
	double end = gettime();
	printf("%f\t%f\t",end - start, endinner- startinner);
	clFinish(queue);
	del_Image(dash3);
}

void bestCase2(char * image)
{

	Image * dash2 = new_ImageFromFile(image);
	clFinish(queue);
	double start = gettime();
		transfer(dash2,1);
		clFinish(queue);
		double startinner = gettime();
			#pragma startfuse
			resize(dash2 , dash2->width*1.1 , dash2->height*1.1);
			toHSV(dash2);
			colorize(dash2,.20);
			toRGB(dash2);
			RGBinvert(dash2);
			#pragma endfuse
			clFinish(queue);
		double endinner = gettime();
		transfer(dash2,0);
		clFinish(queue);
	double end = gettime();
	printf("%f\t%f\t",end - start, endinner- startinner);
	del_Image(dash2);
	bestCase3(image);
	
}


void bestcase(char * image)
{
	//perror("Best Case");
	Image * dash = new_ImageFromFile(image);

	clFinish(queue);
	float color = .20;
	double start = gettime();
		transfer(dash,1);
		clFinish(queue);
		double startinner = gettime();
			resize(dash , dash->width*1.1 , dash->height*1.1);
			toHSV(dash);
			colorize(dash,color);
			toRGB(dash);
			RGBinvert(dash);	
			clFinish(queue);
		double endinner = gettime();
		transfer(dash,0);
		clFinish(queue);
	double end = gettime();
	printf("%f\t%f\t",end - start, endinner- startinner);

	del_Image(dash);
	bestCase2(image);
	
	//delete dash3;		
}

void avgcase3(char * image, char * image2)
{
	Image * dash = new_ImageFromFile(image);
	Image * second = new_ImageFromFile(image2);
	ConvMatrix * blur = new_conv(1);
	double b2[9];
	for(int i = 0; i < blur->size; i++)
	{
		b2[i] = 1;
		blur->CPU_Matrix[i] = 1;
	}	
	char geom[32];
	//resize(second,(int)second->width,(int)second->height);
	clFinish(queue);
	double start = gettime();
		transfer(second,1);
		transfer(dash,1);
		matrix_transfer(blur,1);
		clFinish(queue);
		double startinner = gettime();
			avgCase(dash,second,blur);
			clFinish(queue);
		double endinner = gettime();
		transfer(dash,0);
		clFinish(queue);
		
	double end = gettime(); 
	printf("%f\t%f\t",end - start, endinner- startinner);	
	if(OUTPUT)
	{
		save(dash,"avg3.png");
	}
 	
	del_Image(dash);	
	del_conv(blur);
	del_Image(second);
}
void avgcase2(char * image, char * image2)
{

	Image * dash = new_ImageFromFile(image);
	Image * second = new_ImageFromFile(image2);
	ConvMatrix * blur = new_conv(1);
	double b2[9];
	for(int i = 0; i < blur->size; i++)
	{
		b2[i] = 1;
		blur->CPU_Matrix[i] = 1;
	}	
	char geom[32];
	//resize(second,(int)second->width,(int)second->height);
	clFinish(queue);
	double start = gettime();
		transfer(second,1);
		transfer(dash,1);
		matrix_transfer(blur,1);
		clFinish(queue);
		double startinner = gettime();
			#pragma startfuse
			convolve(dash,blur);
			//resize(dash,second->width, second->height);
			toHSV(dash);
			toHSV(second);
			binOp(dash,second,SUB);		
			toRGB(dash);
			#pragma endfuse
			clFinish(queue);
		double endinner = gettime();
		transfer(dash,0);
		clFinish(queue);
		
	double end = gettime(); 
	printf("%f\t%f\t",end - start, endinner- startinner);	
	if(OUTPUT)
	{
		save(dash,"avg2.png");
	}
 	
	del_Image(dash);	
	del_conv(blur);
	del_Image(second);
	

}

void avgcase(char * image, char* image2)
{
	//perror("Average Case");
	Image * dash = new_ImageFromFile(image);
	Image * second = new_ImageFromFile(image2);
	ConvMatrix * blur = new_conv(1);
	double b2[9];
	for(int i = 0; i < blur->size; i++)
	{
		b2[i] = 1;
		blur->CPU_Matrix[i] = 1;
	}	
	char geom[32];
	//resize(second,(int)second->width,(int)second->height);
	clFinish(queue);
	double start = gettime();
		transfer(second,1);
		transfer(dash,1);
		matrix_transfer(blur,1);
		clFinish(queue);
		double startinner = gettime();
			convolve(dash,blur);
			//resize(dash,second->width, second->height);
			toHSV(dash);
			toHSV(second);
			binOp(dash,second,SUB);		
			toRGB(dash);
			clFinish(queue);
		double endinner = gettime();
		transfer(dash,0);
		clFinish(queue);	
	double end = gettime(); 
	
	printf("%f\t%f\t",end - start, endinner- startinner);	

	if(OUTPUT)
	{
		save(dash,"avg1.png");
	}
	del_Image(dash);	
	del_conv(blur);
	del_Image(second);
	avgcase2(image,image2);	
	avgcase3(image,image2);	
}

void worst3(char * image)
{
	Image * dash = new_ImageFromFile(image);
	Image * dash2 = new_ImageFromFile(image);
	ConvMatrix * blur = new_conv(1);
	ConvMatrix * blur2 = new_conv(1);
	double b2[9];
	double b3[9];
	for(int i = 0; i < blur->size; i++)
	{
		blur->CPU_Matrix[i] = 1;
		blur2->CPU_Matrix[i] = 1;
		b2[i] = 1;
		b3[i] = 1;
	}	
	ConvMatrix * combo = merg_conv(blur,blur2);
	clFinish(queue);
	
	double start = gettime();
		transfer(dash,1);
		matrix_transfer(blur,1);
		matrix_transfer(blur2,1);
		clFinish(queue);
		double startinner = gettime();
			worstcase(dash,dash2,blur,blur2);
			clFinish(queue);
		double endinner = gettime();
		transfer(dash,0);
	clFinish(queue);
	double end = gettime(); printf("%f\t%f\t",end - start, endinner- startinner);
	
	if(OUTPUT)
	{
		save(dash,"worst3.png");
	}
	del_Image(dash);
	del_Image(dash2);
	del_conv(blur);
	del_conv(blur2);	
	del_conv(combo);
		
}

void worst2(char * image)
{
	Image * dash = new_ImageFromFile(image);
	Image * dash2 = new_ImageFromFile(image);
	ConvMatrix * blur = new_conv(1);
	ConvMatrix * blur2 = new_conv(1);
	double b2[9];
	double b3[9];
	for(int i = 0; i < blur->size; i++)
	{
		blur->CPU_Matrix[i] = 1;
		blur2->CPU_Matrix[i] = 1;
		b2[i] = 1;
		b3[i] = 1;
	}	
	ConvMatrix * combo = merg_conv(blur,blur2);
	clFinish(queue);
	
	double start = gettime();
		transfer(dash,1);
		matrix_transfer(blur,1);
		matrix_transfer(blur2,1);
		clFinish(queue);
		double startinner = gettime();
			#pragma startfuse
			convolve(dash,blur);
			convolve(dash2,blur2);
			binOp(dash,dash2,SUB);
			#pragma endfuse
			clFinish(queue);
		double endinner = gettime();
		transfer(dash,0);
	clFinish(queue);
	double end = gettime(); printf("%f\t%f\t",end - start, endinner- startinner);
	
	if(OUTPUT)
	{
		save(dash,"worst2.png");
	}
	del_Image(dash);
	del_Image(dash2);
	del_conv(blur);
	del_conv(blur2);	
	del_conv(combo);

		
}

void worstcase(char * image)
{
	//perror("Worst Case");
	Image * dash = new_ImageFromFile(image);
	Image * dash2 = new_ImageFromFile(image);
	ConvMatrix * blur = new_conv(1);
	ConvMatrix * blur2 = new_conv(1);
	double b2[9];
	double b3[9];
	for(int i = 0; i < blur->size; i++)
	{
		blur->CPU_Matrix[i] = 1;
		blur2->CPU_Matrix[i] = 1;
		b2[i] = 1;
		b3[i] = 1;
	}	
	ConvMatrix * combo = merg_conv(blur,blur2);
	clFinish(queue);
	
	double start = gettime();
		transfer(dash,1);
		matrix_transfer(blur,1);
		matrix_transfer(blur2,1);
		clFinish(queue);
		double startinner = gettime();
			convolve(dash,blur);
			convolve(dash2,blur2);
			binOp(dash,dash2,SUB);
			clFinish(queue);
		double endinner = gettime();
		transfer(dash,0);
	clFinish(queue);
	double end = gettime(); printf("%f\t%f\t",end - start, endinner- startinner);
	if(OUTPUT)
	{
		save(dash,"worst1.png");
	}
	del_Image(dash);
	del_Image(dash2);
	del_conv(blur);
	del_conv(blur2);	
	del_conv(combo);
	worst2(image);
	worst3(image);	
}

void complicated(char * comp, char * other)
{
	////perror("complicated Case");	
	Image * dash = new_ImageFromFile(comp);
	Image * dash2 = new_ImageFromFile(comp);
	Image * dash3 = new_ImageFromFile(comp);
	Image * second1 = new_ImageFromFile(other);
	Image * second2 = new_ImageFromFile(other);
	Image * second3 = new_ImageFromFile(other);
	
	ConvMatrix * blur = new_conv(1);
	ConvMatrix * sharpen = new_conv(1);
	ConvMatrix * blur2 = new_conv(1);
	ConvMatrix * sharpen2 = new_conv(1);
	ConvMatrix * blur3 = new_conv(1);
	ConvMatrix * sharpen3 = new_conv(1);			
	double b2[9];
	double b3[9];
	for(int i = 0; i < blur->size; i++)
	{
		blur->CPU_Matrix[i] = 1;
		sharpen->CPU_Matrix[i] = 0;
		blur2->CPU_Matrix[i] = 1;
		sharpen2->CPU_Matrix[i] = 0;
		blur3->CPU_Matrix[i] = 1;
		sharpen3->CPU_Matrix[i] = 0;				
		b2[i] = 1;
		b3[i] = 0;
		
	}
	

	b3[7] = 9;
	sharpen->CPU_Matrix[5] = 1;
	sharpen->div = 1;
	sharpen2->CPU_Matrix[5] = 1;
	sharpen2->div = 1;
	sharpen3->CPU_Matrix[5] = 1;
	sharpen3->div = 1;
	
	clFinish(queue);
	double start = gettime();
		transfer(dash,1);
		transfer(second1,1);
		matrix_transfer(blur,1);
		matrix_transfer(sharpen,1);
		clFinish(queue);	
		double startinner = gettime();
			convolve(dash,blur);
			convolve(second1,sharpen);
			greyscale(dash);
			greyscale(second1);
			binOp(dash,second1,SUB);
			clFinish(queue);
		double endinner = gettime();
		transfer(dash,0);
		clFinish(queue);
	double end = gettime(); printf("%f\t%f\t",end - start, endinner- startinner);

	clFinish(queue);
	start = gettime();
		transfer(dash2,1);
		transfer(second2,1);
		matrix_transfer(blur2,1);
		matrix_transfer(sharpen2,1);
		clFinish(queue);		
		startinner = gettime();
			#pragma startfuse
			convolve(dash2,blur2);
			convolve(second2,sharpen2);
			greyscale(dash2);
			greyscale(second2);
			binOp(dash2,second2,SUB);
			#pragma endfuse						
			clFinish(queue);
		endinner = gettime();
		transfer(dash2,0);
		clFinish(queue);
	end = gettime(); printf("%f\t%f\t",end - start, endinner- startinner);

	clFinish(queue);
	start = gettime();
		transfer(dash3,1);
		transfer(second3,1);
		matrix_transfer(blur3,1);
		matrix_transfer(sharpen3,1);
		clFinish(queue);		
		startinner = gettime();
			complicated(dash3, second3, blur3, sharpen3);
			clFinish(queue);
		endinner = gettime();
		transfer(dash3,0);
		clFinish(queue);
	end = gettime(); printf("%f\t%f\t",end - start, endinner- startinner);

	if(OUTPUT)
	{
		save(dash, "comp1.png");
		save(dash2,"comp2.png");
		save(dash3,"comp3.png");
	//	dash3->write("comp3.png");
	}	
	
	del_Image(dash);
	del_Image(dash2);
	del_Image(dash3);
	//delete dash3;
	
	del_Image(second1);
	del_Image(second2);
	del_Image(second3);
	//delete second3;
	
	del_conv(blur);
	del_conv(sharpen);
	del_conv(blur2);
	del_conv(sharpen2);
	del_conv(blur3);
	del_conv(sharpen3);		
}


/*

*/
int main(int argc,  char ** argv)
{
	if(argc < 4) 
	{
		printf("Correct usage is: imageTest <platform> <device> <TestImage1> <Test Image2> <iterations>");
		exit(1);
	}
	init(atoi(argv[1]), atoi(argv[2]));
	int iterations = 1000;	
	if(argc > 4)
		iterations = atoi(argv(5));
	//printf("BestCase.Total\tBestCase.OperationsOnly\tBestCase.Fused.Total\tBestCase.Fused.OperationsOnly\tBestCase Manual All\tBest Case Manual Fusion\t\t");
	//printf("AvgCase.Total\tAvgCase.Functions\tAvgCase.Fused.Total\tAvgCase.Fused.Operations\tManual.Fusion\tManual Fusion Operations Only\t\t");
	//printf("Complicated.Total\tComplicated.Functions\tComplicated.Fused.Total\tComplicated.Fused.Operations\tManual.Fusion\tmanual fusion operations only\t\t");
	//printf("WorstCase.Total\tWorstCase.Functions\tWorstCase.Fused.Total\tWorstCase.Fused.Operations\tManual.Fusion\tmanual fusion operations only\n");
	for(int i = 0; i < 1000; i++)
	{
		bestcase(argv[3]); printf("\t");
		avgcase(argv[3],argv[4]);printf("\t");
		complicated(argv[3],argv[4]);printf("\t");
		worstcase(argv[3]);
		printf("\n");
		clFinish(queue);
		fflush(NULL);
	}
	return 0;
}
