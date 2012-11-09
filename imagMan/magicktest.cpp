#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <omp.h>
#include <math.h>
#include <assert.h>
#include <sys/time.h>
#include <CL/cl.h>
#include <Magick++.h>
#define OUTPUT 0
#define DEPTH 4 

using namespace Magick;
double gettime()
{
	struct timeval start;
	gettimeofday(&start,NULL);
	return (double)start.tv_sec + start.tv_usec/1000000.0;
}


int bestCase(char * i)
{
	char buffer[64];
	sprintf(buffer,"110%%x110%%/!");
	Image * dash4 = new Image(i);
	Color c(ColorHSL(.20,1,1));
	//Geometry g("110%%x110%%");
	double start = gettime();
		double startinner = gettime();
			dash4->resize("1238x1238!");
			dash4->colorize(1,1,1,c);
			dash4->negate();
		double endinner = gettime();
	double end = gettime();
	printf("%f\t%f\t",end - start, endinner- startinner);
	//dash4->write("best4.png");
	delete dash4;
}	
	
	
int avgcase(char * i)
{
	char buffer[64];
	sprintf(buffer,"110%%x110%%/!");
	Image * dash = new Image(i);
	Image * second = new Image(i);	
	Color c(ColorHSL(.20,1,1));
	double b2[9];
	for(int i = 0; i < 9; i++)
	{
		b2[i] = 1;
	}

	double start = gettime();
		double startinner = gettime();
			dash->convolve(3,b2);
			dash->composite(*second,0,0,MinusCompositeOp);
		double endinner = gettime();
	double end = gettime();
	printf("%f\t%f\t",end - start, endinner- startinner);
	//dash->write("avg4.png");
	delete dash;
	delete second;
}	

int worst(char * i)
{
	char buffer[64];
	sprintf(buffer,"110%%x110%%/!");
	Image * dash = new Image(i);
	Image * second = new Image(i);	
	Color c(ColorHSL(.20,1,1));
	double b2[9];
	for(int i = 0; i < 9; i++)
	{
		b2[i] = 1;
	}

	double start = gettime();
		double startinner = gettime();
			dash->convolve(3,b2);
			second->convolve(3,b2);
			dash->composite(*second,0,0,MinusCompositeOp);
		double endinner = gettime();
	double end = gettime();
	printf("%f\t%f\t",end - start, endinner- startinner);
	//dash->write("worst4.png");
	delete dash;
	delete second;
}
		
int complicated(char * i)
{
	char buffer[64];
	sprintf(buffer,"110%%x110%%/!");
	Image * dash = new Image(i);
	Image * second = new Image(i);	
	Color c(ColorHSL(.20,1,1));
	double b2[9];
	for(int i = 0; i < 9; i++)
	{
		b2[i] = 1;
	}

	double start = gettime();
		double startinner = gettime();
			dash->convolve(3,b2);
			second->convolve(3,b2);
			dash->type(GrayscaleType);
			second->type(GrayscaleType);
			dash->composite(*second,0,0,MinusCompositeOp);
		double endinner = gettime();
	double end = gettime();
	printf("%f\t%f\t",end - start, endinner- startinner);
	//dash->write("comp4.png");
	delete dash;
	delete second;
}
	
int main(int argc, char ** argv)
{

	for(int i = 0; i < 1000; i++)
	{
		bestCase(argv[1]); printf("\t");
		avgcase(argv[1]); printf("\t");	
		worst(argv[1]); printf("\t");	
		complicated(argv[1]); printf("\t");
		printf("\n");
	}

}
