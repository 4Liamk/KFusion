#ifndef SPARSE_H
#define SPARSE_H
#include <CL/cl.h>
#include <stdio.h>
#include "lodepng.h"
#include "reader.h"
#include <vector>
#include <stdlib.h>
#include "check.h"
#define COMMAND_QUEUE_ARGS NULL
#define DEPTH 4
#define TYPE float
#define RGBTYPE  0
#define HSVTYPE  1
#define DEBUG 0
#define AND 0
#define OR 1
#define XOR 2
#define ADD 3
#define SUB 4
#define MUL 5
#define DIV 6
#define AVG 7
#include <assert.h>

//CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE 
void init(int platform, int device);
size_t getLocalSize(int global);

/*
 type: ConvMatrix
 a convolution matrix type used
*/
typedef struct convMatrix
{
		cl_mem GPU_Matrix;
		TYPE * CPU_Matrix;
		int dim;
		float div;
		int size;

		int locality;
}ConvMatrix;

/*

*/
typedef struct image
{
		cl_mem GPU_Image;
		TYPE * CPU_Image;
		unsigned int width;
		unsigned int height;	
		int locality;
		int type;
}Image;

//conv Functions
ConvMatrix * new_conv(int dim_);
ConvMatrix * merg_conv(ConvMatrix * one, ConvMatrix * two);
void del_conv(ConvMatrix * tokill);

void matrix_transfer(ConvMatrix * m, int dest);
void matrix_print(char * prefix);

void encode(char * message, char * image);

char * decode(char * outfile, char * image);

Image * new_ImageFromFile(char * file);
Image * new_ImageWidthHeight(int w, int h);
Image * copy_Image(Image * copy);
void del_Image(Image * I);

//data movement to/from GPU
void transfer(Image * I, int dest);			
void save(Image * I, char * file);

//image manipulation functions
void greyscale(Image * I);
void blur(Image * I, int level);
void convolve(Image * I, convMatrix * input);
void colorize(Image * I, float color);

//testing specific functions
void bestCase(Image * I,int width,int height,float color);
void avgCase(Image * I,Image * other, convMatrix * matrix);
void complicated(Image * I,Image * other,convMatrix *  blur, convMatrix * sharpen);

//image conversion stuff
void toHSV(Image * I);
void toRGB(Image * I);
void setFormat(Image * I,int type_);
void clear(Image * I);
void binOp(Image * I,Image * other, int op);
void binOp(Image * I,Image * a,Image * b,int op);
void resize(Image * I,int width, int height);
void crop(int x1, int x2, int y1, int y2);
void overlay(Image * I,Image * other, int x1, int y1, int op);
void mosiac(Image * I,char * file, int size);
void RGBinvert(Image * I);
void edge(Image * I,int factor);	
void worstcase(Image * I, Image * d, ConvMatrix * a, convMatrix * b);

//platform information
extern cl_platform_id	platforms[16];
extern cl_uint 	num_platforms;
extern int 	param_value_buffer[8];
extern size_t 	param_value_size_ret;

//device infomation
extern cl_device_type device_type[8];
extern cl_device_id   device_list[8];

//context and queue information
extern cl_context     	context;
extern cl_command_queue queue;
extern cl_program 	program;
extern cl_image_format 	format;

//image operation kernels
extern cl_kernel RGBtoHSV_kernel;
extern cl_kernel binOp_kernel;
extern cl_kernel HSVtoRGB_kernel;
extern cl_kernel RGBInvert_kernel;
extern cl_kernel RGBGreyscale_kernel;
extern cl_kernel convolution_kernel;
extern cl_kernel resize_kernel;
extern cl_kernel crop_kernel;
extern cl_kernel overlay_kernel;
extern cl_kernel colorize_kernel;

//testing kernels
extern cl_kernel bestCase_kernel;
extern cl_kernel avgCase_kernel;
extern cl_kernel complicated_kernel;
extern cl_kernel worstCase_kernel;
#endif
