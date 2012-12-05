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
 a convolution matrix type used when performing an image convolution
*/
typedef struct convMatrix
{
		//gpu data
		cl_mem GPU_Matrix;
		
		//cpu data
		TYPE * CPU_Matrix;
		
		//dimension of the matrix
		int dim;
		
		//divisor for the end result typically 9 or 25 depending on size
		float div;
		
		//size of the matrix typicaly 9 or 25
		int size;
		//locality flag
		int locality;
}ConvMatrix;

/*

*/
typedef struct image
{
		cl_mem GPU_Image; //gpu data
		TYPE * CPU_Image; //cpu data
		unsigned int width; //widt of image
		unsigned int height; //height of image
		int locality; //locality flag, host or device.  0 for host 1 for device
		int type; //type of image RGB or HSV
}Image;

//conv Functions

//create a convolution, must be deleted later with del_conv
ConvMatrix * new_conv(int dim_);

//combine two convolution functions to produce a third one approximately the combined width and height of the child matrices
//must be deleted using del_conv
ConvMatrix * merg_conv(ConvMatrix * one, ConvMatrix * two);

//delete a convolution matrix
void del_conv(ConvMatrix * tokill);

/*
  transfer a matrix to and from device. 
  0 is for host. 
  1 is for device
*/
void matrix_transfer(ConvMatrix * m, int dest);

/*Print a matrix.  DO NOT USE!*/
//void matrix_print(ConvMatrix *m,char * prefix);

//Image creation functions
/*
  new_ImageFromFile(char * file)
  load a png from file
  returns an Image struct
*/
Image * new_ImageFromFile(char * file);

/*
  new_ImageFromWidthHeight(int w. int h)
  creates a new image of width w and height h
  image should be deleted using del_Image
*/
Image * new_ImageWidthHeight(int w, int h);

/*
   Copy and Image from another one
   Must be deleted using del_Image
*/
Image * copy_Image(Image * copy);

/*
	delete an image
*/
void del_Image(Image * I);

//data movement to/from GPU
/*
  Transfer an image to and from the target device.  0 is the CPU, device is 1
  Uses lazy copying and as such, will not copy of locality is the same as destination
*/
void transfer(Image * I, int dest);			

/*
  save an Image I to file as a png.  This uses basic PNG settings and seems to work well
*/
void save(Image * I, char * file);

//image manipulation functions
void greyscale(Image * I);
void blur(Image * I, int level);
void convolve(Image * I, convMatrix * input);
void colorize(Image * I, float color);

//testing specific functions
//Each one of these functions is a manual implementation of the automatic fusion produced by KFusion
void bestCase(Image * I,int width,int height,float color);
void avgCase(Image * I,Image * other, convMatrix * matrix);
void complicated(Image * I,Image * other,convMatrix *  blur, convMatrix * sharpen);

//image conversion operations
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
