#include "img.h"
#include <string.h>
cl_platform_id	platforms[16];
cl_uint 	num_platforms;
int 		param_value_buffer[8];
size_t 		param_value_size_ret;

//device infomation
cl_device_type device_type[8];
cl_device_id   device_list[8];

//context and queue information
cl_context context;
cl_command_queue queue;
cl_program program;
cl_image_format format;

cl_kernel RGBGreyscale_kernel;
cl_kernel convolution_kernel;
cl_kernel RGBtoHSV_kernel;
cl_kernel HSVtoRGB_kernel;
cl_kernel RGBInvert_kernel;
cl_kernel binOp_kernel;
cl_kernel resize_kernel;
cl_kernel crop_kernel;
cl_kernel overlay_kernel;
cl_kernel colorize_kernel;

//testing kernels
cl_kernel bestCase_kernel;
cl_kernel avgCase_kernel;
cl_kernel complicated_kernel;
cl_kernel worstCase_kernel;

size_t getLocalSize(int global)
{
	for(size_t i = 256; i > 0; i--)
	{
		if(global % i == 0) 
		{
			if(DEBUG) printf("%d for %d\n",i,global);
			return i;
		}
	}
	return 1;
}
void init(int platform, int device)
{
	unsigned int i; 
	int result;
	//platform information
 	//perror("getting basic openCL data: platform info and number of devices");
	
	check(clGetPlatformIDs(8,platforms, &num_platforms));
	//printf("num platforms: %d\n",num_platforms);
	//get info for first platform
	unsigned int num_devices = 0;
	for(i = 0; i < num_platforms; i++)
	{
		
		printf("Platform %d\n", i);
		check(clGetPlatformInfo(platforms[i], CL_PLATFORM_PROFILE, (size_t) 32, (void*)	param_value_buffer, &param_value_size_ret));
 		printf("\tProfile: %s\n",(char*) param_value_buffer);	
		check(clGetPlatformInfo(platforms[i], CL_PLATFORM_VERSION , (size_t) 32, (void*)	param_value_buffer, &param_value_size_ret));
 		printf("\tVersion: %s\n", (char*) param_value_buffer);	
		check(clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, (size_t) 32, (void*)	param_value_buffer, &param_value_size_ret));
 		printf("\tName: %s\n", (char*) param_value_buffer);	
		check(clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, (size_t) 32, (void*)	param_value_buffer, &param_value_size_ret));
 		printf("\tVendor: %s\n",(char*)  param_value_buffer);	
		check(clGetPlatformInfo(platforms[i], CL_PLATFORM_EXTENSIONS, (size_t) 32,(void*) param_value_buffer, &param_value_size_ret));
 		printf("\tExtensions: %s\n",(char*)  param_value_buffer);	
		check(clGetPlatformInfo(platforms[i], CL_PLATFORM_PROFILE, (size_t) 32, (void*)	param_value_buffer, &param_value_size_ret));	
	
		//devices information
		
		check(clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 8, device_list, &num_devices));
	
		char p1[512];
		p1[511] = '\0';
		printf("\tnum devices: %d\n\n", num_devices);
		for(i = 0; i < num_devices; i++)
		{
			cl_ulong size;
			check(clGetDeviceInfo(device_list[i], CL_DEVICE_NAME, 512, (void*) p1, NULL));
			printf("\tdevice %d: %s\n", i, p1);
			check(clGetDeviceInfo(device_list[i], CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, sizeof(cl_ulong), (void*) &size, NULL));
			printf("\t\tGlobal Cache Size: %u\n", size);
			check(clGetDeviceInfo(device_list[i], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), (void*) &size, NULL));
			printf("\t\tGlobal Mem Size: %u\n", size);
			check(clGetDeviceInfo(device_list[i], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), (void*) &size, NULL));
			printf("\t\tlocal mem size: %u\n",size);
			check(clGetDeviceInfo(device_list[i], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), (void*) &size, NULL));		
			printf("\t\tmemory available: %d\n",p1[0]);
		}	
	}
	printf("running on platform %d Device %d\n",platform,device);
	check(clGetDeviceIDs(platforms[platform], CL_DEVICE_TYPE_ALL, 8, device_list, &num_devices));
	//create compute context over all available devices
	context = clCreateContext(NULL,1,&device_list[device],NULL,NULL,&result);	
	check(result);	

	//get devices available to context
	size_t nContextDescriptorSize = 0;
	
	clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, 0, &nContextDescriptorSize);
	printf("got %d context\n",nContextDescriptorSize);		
	cl_device_id * aDevices = (cl_device_id*) malloc(nContextDescriptorSize);
	size_t contextSize;
	clGetContextInfo(context, CL_CONTEXT_DEVICES, nContextDescriptorSize, aDevices, &contextSize);
	printf("got %d devices\n",contextSize);	

	//make cpu queue
	queue = clCreateCommandQueue(context,aDevices[0], CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE , &result);
	check(result);

	const char* program_source = (char*) readin("kernel-out.cl");
	
	program = clCreateProgramWithSource(context,1, &program_source,NULL,&result);
	if (result != CL_SUCCESS)
  	{
  		check(result);
        	size_t len;
        	char buffer[5000];
        	clGetProgramBuildInfo(program, aDevices[0], CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
       		printf("%s\n", buffer);
        	exit(1);

    	}
    	perror("program loaded");

	//build for gpu
	result = clBuildProgram(program,1,aDevices,NULL,NULL,NULL);
	if (result != CL_SUCCESS)
  	{
  		check(result);
  		printf("build fail\n");
        	size_t len;
        	char buffer[50000];
        	for(i = 0; i < num_devices; i++)
        	{
        		memset(buffer,0,sizeof(buffer));
      		  	clGetProgramBuildInfo(program, device_list[i], CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        		printf("%s\n", buffer);
		}
        	exit(1);

    	}
    	
    	perror("program built");

	format.image_channel_order = CL_RGBA;
	format.image_channel_data_type = CL_FLOAT;
	
	RGBGreyscale_kernel = clCreateKernel(program,"RGBGreyscale",&result);
	HSVtoRGB_kernel  = clCreateKernel(program,"HSVtoRGB",&result);
	RGBtoHSV_kernel  = clCreateKernel(program,"RGBtoHSV",&result);
	RGBInvert_kernel = clCreateKernel(program,"RGBInvert",&result);
	convolution_kernel = clCreateKernel(program,"convolution",&result);
	binOp_kernel 	= clCreateKernel(program,"binOp",&result);
	
	resize_kernel	= clCreateKernel(program,"resize",&result);;
	crop_kernel 	= clCreateKernel(program,"crop",&result);;
	overlay_kernel	= clCreateKernel(program,"overlay",&result);	
	
	colorize_kernel = clCreateKernel(program,"colorize",&result);
	bestCase_kernel = clCreateKernel(program,"bestCase",&result);
	avgCase_kernel = clCreateKernel(program,"avgCase",&result);
	complicated_kernel = clCreateKernel(program,"complicated",&result);
	worstCase_kernel = clCreateKernel(program,"worstCase",&result);
	perror("compute kernels ready");	
		
}

Image * new_ImageFromFile(char * file)
{
	Image * tmp = (Image*) malloc(sizeof(Image));
	unsigned char * data = NULL;
	LodePNG_decode32_file(&data, &tmp->width, &tmp->height, file);
	tmp->CPU_Image = (TYPE*) malloc(sizeof(TYPE)*tmp->width*tmp->height*DEPTH);
	for(unsigned int i = 0; i < tmp->height; i++)
	{
		for(unsigned int j = 0; j < tmp->width; j++)
		{
			for(unsigned int k = 0; k < DEPTH; k++)
			{
				 tmp->CPU_Image[(i*tmp->width + j)*DEPTH + k] = (data[(i*tmp->width + j)*DEPTH + k]);	
			}			
		}
	}	
	free(data);
	
	int result;
	tmp->GPU_Image = clCreateImage2D(context,CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, &format, tmp->width, tmp->height, 0,NULL,&result);
	check(result);
	tmp->locality = 0;
	tmp->type = RGBTYPE;
	return tmp;
}

void del_Image(Image * I)
{
	free(I->CPU_Image);
	clReleaseMemObject(I->GPU_Image);
	free(I);
}

Image * new_ImageWidthHeight(int w, int h)
{
	Image * tmp = (Image*) malloc(sizeof(Image));
	cl_int result;
	tmp->width = w;
	tmp->height = h;
	tmp->CPU_Image = (TYPE*) malloc(sizeof(TYPE)*tmp->width*tmp->height*DEPTH);
	tmp->GPU_Image = clCreateImage2D(context,CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, &format, tmp->width, tmp->height, 0,NULL,&result);
	check(result);
	tmp->locality = 0;
	tmp->type = RGBTYPE;
}

void clear(Image * I)
{
	for(unsigned int i = 0; i < I->height; i++)
	{
		for(unsigned int j = 0; j < I->width; j++)
		{
			for(unsigned int k = 0; k < DEPTH; k++)
			{
				 I->CPU_Image[(i*I->width + j)*DEPTH + k] = 0;
			}
		}
	}
				 
}

void greyscale(Image * I)
{
	transfer(I,1);
	cl_int result;
	
	size_t gsize[2];
	gsize[0] = I->width;
	gsize[1] = I->height;
	
	size_t lsize[2];
	lsize[0] = getLocalSize(I->width);
	lsize[1] = getLocalSize(I->height);
	
	cl_mem tmp = clCreateImage2D(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, &format, I->width, I->height, 0,NULL,&result);
	check(result);
	check(clSetKernelArg(RGBGreyscale_kernel,0,sizeof(cl_mem),&I->GPU_Image));
	check(clSetKernelArg(RGBGreyscale_kernel,1,sizeof(cl_mem),&tmp));
	check(clEnqueueNDRangeKernel(queue,RGBGreyscale_kernel,2,0,gsize,NULL,0,NULL,NULL));
	clReleaseMemObject(I->GPU_Image);
	I->GPU_Image = tmp;
}

void colorize(Image * I, float color)
{
	transfer(I,1);
	cl_int result;
	
	size_t gsize[2];
	gsize[0] =I-> width;
	gsize[1] = I->height;
	
	size_t lsize[2];
	lsize[0] = getLocalSize(I->width);
	lsize[1] = getLocalSize(I->height);
	
	cl_mem tmp = clCreateImage2D(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, &format, I->width, I->height, 0,NULL,&result);
	check(result);
	check(clSetKernelArg(colorize_kernel,0,sizeof(cl_mem),&I->GPU_Image));
	check(clSetKernelArg(colorize_kernel,1,sizeof(cl_mem),&tmp));
	check(clSetKernelArg(colorize_kernel,2,sizeof(float),&color));
	check(clEnqueueNDRangeKernel(queue,colorize_kernel,2,0,gsize,NULL,0,NULL,NULL));
	clReleaseMemObject(I->GPU_Image);
	I->GPU_Image = tmp;
}

void bestCase(Image * I, int width_,int height_,float color)
{
	
	transfer(I,1);
	cl_int result;
	
	size_t gsize[2];
	gsize[0] = width_;
	gsize[1] = height_;
	I->width = width_;
	I->height = height_;
	
	size_t lsize[2];
	lsize[0] = getLocalSize(I->width);
	lsize[1] = getLocalSize(I->height);
	
	cl_mem tmp = clCreateImage2D(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, &format, I->width, I->height, 0, NULL, &result);
	
	check(result);
	check(clSetKernelArg(bestCase_kernel,0,sizeof(cl_mem),&I->GPU_Image));
	check(clSetKernelArg(bestCase_kernel,1,sizeof(cl_mem),&tmp));
	check(clSetKernelArg(bestCase_kernel,2,sizeof(float),&color));
	check(clEnqueueNDRangeKernel(queue, bestCase_kernel , 2, 0, gsize, NULL, 0, NULL, NULL));

	clReleaseMemObject(I->GPU_Image);
	free(I->CPU_Image);	
	I->CPU_Image = NULL;
	I->CPU_Image = (TYPE*) malloc(sizeof(TYPE)*I->width*I->height*DEPTH);
	I->GPU_Image = tmp;
	
}


void avgCase(Image * I, Image * other, convMatrix * matrix)
{
	
	matrix_transfer(matrix,1);
	transfer(I,1);
	transfer(other,1);
	cl_int result;
	
	size_t gsize[2];
	gsize[0] = other->width;
	gsize[1] = other->height;
	I->width = other->width;
	I->height = other->height;
	
	size_t lsize[2];
	lsize[0] = getLocalSize(I->width);
	lsize[1] = getLocalSize(I->height);
	
	cl_mem tmp = clCreateImage2D(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, &format, I->width, I->height, 0, NULL, &result);
	check(result);

	check(clSetKernelArg(avgCase_kernel,0,sizeof(cl_mem),&I->GPU_Image));
	check(clSetKernelArg(avgCase_kernel,1,sizeof(cl_mem),&other->GPU_Image));
	check(clSetKernelArg(avgCase_kernel,2,sizeof(cl_mem),&tmp));
	check(clSetKernelArg(avgCase_kernel,3,sizeof(cl_mem),&matrix->GPU_Matrix));
	check(clSetKernelArg(avgCase_kernel,4,sizeof(int),&matrix->dim));
	check(clSetKernelArg(avgCase_kernel,5,sizeof(float),&matrix->div));
	check(clEnqueueNDRangeKernel(queue, avgCase_kernel , 2, 0, gsize, NULL, 0, NULL, NULL));
	clReleaseMemObject(I->GPU_Image);
	free(I->CPU_Image);
	
	I->CPU_Image = NULL;
	I->CPU_Image = (TYPE*) malloc(sizeof(TYPE)*I->width*I->height*DEPTH);
	I->GPU_Image = tmp;
}

void worstcase(Image * I, Image * other, convMatrix * blur,convMatrix * blur2)
{
	
	matrix_transfer(blur,1);
	matrix_transfer(blur2,1);
	transfer(I,1);
	transfer(other,1);
	cl_int result;
	
	size_t gsize[2];
	gsize[0] = other->width;
	gsize[1] = other->height;
	I->width = other->width;
	I->height = other->height;
	
	size_t lsize[2];
	lsize[0] = getLocalSize(I->width);
	lsize[1] = getLocalSize(I->height);
	
	cl_mem tmp = clCreateImage2D(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, &format, I->width, I->height, 0, NULL, &result);
	check(result);
//worstCase(__read_only image2d_t  input,__read_only image2d_t  input2, __write_only image2d_t output,__global TYPE * matrix,__global TYPE * matrix2, const int dim, const float div)

	check(clSetKernelArg(worstCase_kernel,0,sizeof(cl_mem),&I->GPU_Image));
	check(clSetKernelArg(worstCase_kernel,1,sizeof(cl_mem),&other->GPU_Image));
	check(clSetKernelArg(worstCase_kernel,2,sizeof(cl_mem),&tmp));
	check(clSetKernelArg(worstCase_kernel,3,sizeof(cl_mem),&blur->GPU_Matrix));
	check(clSetKernelArg(worstCase_kernel,4,sizeof(cl_mem),&blur2->GPU_Matrix));
	check(clSetKernelArg(worstCase_kernel,5,sizeof(int),&blur->dim));
	check(clSetKernelArg(worstCase_kernel,6,sizeof(float),&blur->div));
	check(clEnqueueNDRangeKernel(queue, worstCase_kernel , 2, 0, gsize, NULL, 0, NULL, NULL));
	clReleaseMemObject(I->GPU_Image);
	free(I->CPU_Image);
	
	I->CPU_Image = NULL;
	I->CPU_Image = (TYPE*) malloc(sizeof(TYPE)*I->width*I->height*DEPTH);
	I->GPU_Image = tmp;
}


void binOp(Image * I, Image * other, int op)
{
	transfer(I,1);
	transfer(other,1);
	cl_int result;
	size_t gsize[2];
	gsize[0] = I->width;
	gsize[1] = I->height;
	
	cl_mem tmp = clCreateImage2D(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, &format, I->width, I->height, 0, NULL, &result);
	check(result);
	check(clSetKernelArg(binOp_kernel,0,sizeof(cl_mem),&I->GPU_Image));
	check(clSetKernelArg(binOp_kernel,1,sizeof(cl_mem),&other->GPU_Image));
	check(clSetKernelArg(binOp_kernel,2,sizeof(cl_mem),&tmp));
	check(clSetKernelArg(binOp_kernel,3,sizeof(int),&op));
	check(clEnqueueNDRangeKernel(queue,binOp_kernel,2,0,gsize,NULL,0,NULL,NULL));
	clReleaseMemObject(I->GPU_Image);
	I->GPU_Image = tmp;
}


//(__read_only image2d_t  input, __read_only image2d_t  input2,__write_only image2d_t output, const int x1, const int x2, const int y1, const int y2, const int op)
void overlay(Image * I, Image * other, int x1, int y1, int op)
{
	setFormat(I,1);
	setFormat(other,1);
	cl_int result;
	size_t gsize[2];
	gsize[0] = I->width;
	gsize[1] = I->height;
	cl_mem tmp = clCreateImage2D(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, &format, I->width, I->height, 0,NULL,&result);
	
	size_t lsize[2];
	lsize[0] = getLocalSize(I->width);
	lsize[1] = getLocalSize(I->height);	
	
	check(result);
	check(clSetKernelArg(overlay_kernel,0,sizeof(cl_mem),&I->GPU_Image));
	check(clSetKernelArg(overlay_kernel,1,sizeof(cl_mem),&other->GPU_Image));
	check(clSetKernelArg(overlay_kernel,2,sizeof(cl_mem),&tmp));
	check(clSetKernelArg(overlay_kernel,3,sizeof(int),&x1));
	check(clSetKernelArg(overlay_kernel,4,sizeof(int),&other->width));
	check(clSetKernelArg(overlay_kernel,5,sizeof(int),&y1));
	check(clSetKernelArg(overlay_kernel,6,sizeof(int),&other->height));
	check(clSetKernelArg(overlay_kernel,7,sizeof(int),&op));
	check(clEnqueueNDRangeKernel(queue, overlay_kernel, 2, 0, gsize, lsize, 0 ,NULL ,NULL));
	
	clReleaseMemObject(I->GPU_Image);
	I->GPU_Image = tmp;
}


void RGBinvert(Image * I)
{
	transfer(I,1);
	//printf("inverting %d %d\n",I->width,I->height);
	cl_int result;
	size_t gsize[2];
	gsize[0] = I->width;
	gsize[1] = I->height;
	cl_mem tmp = clCreateImage2D(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, &format, I->width, I->height, 0,NULL,&result);
	check(result);
	check(clSetKernelArg(RGBInvert_kernel,0,sizeof(cl_mem),&I->GPU_Image));
	check(clSetKernelArg(RGBInvert_kernel,1,sizeof(cl_mem),&tmp));
	check(clEnqueueNDRangeKernel(queue,RGBInvert_kernel,2,0,gsize,NULL,0,NULL,NULL));
	clReleaseMemObject(I->GPU_Image);
	I->GPU_Image = tmp;
}

void resize(Image * I, int width, int height)
{
	transfer(I,1);
	cl_int result;
	size_t gsize[2];
	I->width = width;
	I->height = height;
	gsize[0] = I->width;
	gsize[1] = I->height;
	cl_mem tmp = clCreateImage2D(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, &format, I->width, I->height, 0,NULL,&result);	

	check(result);
	check(clSetKernelArg(resize_kernel,0,sizeof(cl_mem),&I->GPU_Image));
	check(clSetKernelArg(resize_kernel,1,sizeof(cl_mem),&tmp));
	check(clEnqueueNDRangeKernel(queue, resize_kernel, 2, 0, gsize, NULL, 0, NULL, NULL));
	clReleaseMemObject(I->GPU_Image);
	
	free(I->CPU_Image);
	I->CPU_Image = (TYPE*) malloc(sizeof(TYPE)*I->width*I->height*DEPTH);
	I->GPU_Image = tmp;
}

void crop(Image * I, int x1, int x2, int y1, int y2)
{
	transfer(I,1);
	cl_int result;
	size_t gsize[2];
	gsize[0] = x2 - x1;
	gsize[1] = y2 - y1;
	I->width = x2 - x1;
	I->height = y2 - y1;
	cl_mem tmp = clCreateImage2D(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, &format, I->width, I->height, 0,NULL,&result);
	check(result);

	size_t lsize[2];
	lsize[0] = getLocalSize(I->width);
	lsize[1] = getLocalSize(I->height);	
	
	check(clSetKernelArg(crop_kernel, 0, sizeof(cl_mem), &I->GPU_Image));
	check(clSetKernelArg(crop_kernel, 1, sizeof(cl_mem), &tmp));
	check(clSetKernelArg(crop_kernel, 2, sizeof(int), &x1));
	check(clSetKernelArg(crop_kernel, 3, sizeof(int), &y1));
	check(clEnqueueNDRangeKernel(queue,resize_kernel,2,0,gsize,lsize,0,NULL,NULL));
	clReleaseMemObject(I->GPU_Image);
	I->GPU_Image = tmp;
}

void setFormat(Image * I, int type_)
{
	transfer(I,1);
	if(I->type == type_) return;
	else if(type_ == HSVTYPE) toHSV(I);
	else if(type_ == RGBTYPE) toRGB(I);
	I->type = type_;
}

void toHSV(Image * I)
{

	transfer(I,1);
	I->type = 1;
	cl_int result;
	size_t gsize[2];
	gsize[0] = I->width;
	gsize[1] = I->height;
	cl_mem tmp = clCreateImage2D(context,CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, &format, I->width, I->height, 0, NULL, &result);
	check(result);
	
	check(clSetKernelArg(RGBtoHSV_kernel,0,sizeof(cl_mem),&I->GPU_Image));
	check(clSetKernelArg(RGBtoHSV_kernel,1,sizeof(cl_mem),&tmp));
	check(clEnqueueNDRangeKernel(queue, RGBtoHSV_kernel, 2, 0, gsize, NULL, 0, NULL, NULL));
	
	clReleaseMemObject(I->GPU_Image);
	I->GPU_Image = tmp;
	
}

void toRGB(Image * I)
{
	I->type = 0; 
	transfer(I,1);
	cl_int result;
	size_t gsize[2];
	gsize[0] = I->width;
	gsize[1] = I->height;
	size_t lsize[2];
	lsize[0] = getLocalSize(I->width);
	lsize[1] = getLocalSize(I->height);	
	cl_mem tmp = clCreateImage2D(context,CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, &format, I->width, I->height, 0,NULL,&result);
	check(result);
	check(clSetKernelArg(HSVtoRGB_kernel,0,sizeof(cl_mem),&I->GPU_Image));
	check(clSetKernelArg(HSVtoRGB_kernel,1,sizeof(cl_mem),&tmp));
	check(clEnqueueNDRangeKernel(queue,HSVtoRGB_kernel,2,0,gsize,NULL,0,NULL,NULL));
	clReleaseMemObject(I->GPU_Image);
	I->GPU_Image = tmp;
}

void complicated(Image * I, Image * other,convMatrix *  blur, convMatrix * sharpen)
{

	cl_int eastwood;
	transfer(other,1);
	transfer(I,1);
	matrix_transfer(blur,1);
	matrix_transfer(sharpen,1);
	size_t gsize[2];
	gsize[0] = I->width;
	gsize[1] = I->height;	
	cl_mem tmp = clCreateImage2D(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, &format, I->width, I->height, 0,NULL,&eastwood);
	check(eastwood);

//complicated(__read_only image2d_t  input, __read_only image2d_t  input2, __write_only image2d_t output, __global TYPE * blur, const int dim, const float div, __global TYPE * sharpen)
	check(clSetKernelArg(complicated_kernel,0,sizeof(cl_mem),&I->GPU_Image));
	check(clSetKernelArg(complicated_kernel,1,sizeof(cl_mem),&other->GPU_Image));
	check(clSetKernelArg(complicated_kernel,2,sizeof(cl_mem),&tmp));
	check(clSetKernelArg(complicated_kernel,3,sizeof(cl_mem),&blur->GPU_Matrix));
	check(clSetKernelArg(complicated_kernel,4,sizeof(int),&blur->dim));
	check(clSetKernelArg(complicated_kernel,5,sizeof(float),&blur->div));
	check(clSetKernelArg(complicated_kernel,6,sizeof(cl_mem),&sharpen->GPU_Matrix));
	check(clSetKernelArg(complicated_kernel,7,sizeof(float),&sharpen->div));
	check(clEnqueueNDRangeKernel(queue,complicated_kernel,2,0,gsize,NULL,0,NULL,NULL));
	clReleaseMemObject(I->GPU_Image);
	I->GPU_Image = tmp;	
}

void save(Image * I, char * file)
{
	setFormat(I, 0);
	transfer(I, 0);
	unsigned char * data = (unsigned char *) malloc(sizeof(unsigned char)*I->width*I->height*DEPTH);
	assert(data);
	for(unsigned int i = 0; i < I->height; i++)
	{
		for(unsigned int j = 0; j < I->width; j++)
		{
			for(unsigned int k = 0; k < DEPTH; k++)
			{
				 data[(i*I->width + j)*DEPTH + k] = (unsigned char) I->CPU_Image[(i*I->width + j)*DEPTH + k];	
			}			
		}
		
	}

	LodePNG_encode32_file(file,data, I->width, I->height);
	free(data);	
	//perror("done!");
}

void transfer(Image * I, int dest)
{
	if(I->locality != dest)

	{
		size_t origin[3];
		origin[0] = 0;
		origin[1] = 0;
		origin[2] = 0;
		size_t region[3];
		region[0] = I->width;
		region[1] = I->height;
		region[2] = 1;		
		if(dest == 1)
		{
			//if(DEBUG) printf("moving to gpu\n");
			check(clEnqueueWriteImage(queue,I->GPU_Image,CL_FALSE,origin, region,0,0,I->CPU_Image,0,NULL,NULL));	
			I->locality = 1;
			//if(DEBUG) printf("done\n");
		}
		else
		{
			//if(DEBUG) printf("moving to cpu\n");
			check(clEnqueueReadImage(queue,I->GPU_Image,CL_FALSE,origin, region, 0,0,I->CPU_Image,0,NULL,NULL));		
			I->locality = 0;
		}
	}
}

void convolve(Image * I, ConvMatrix * matrix)
{
	matrix_transfer(matrix,1);
	transfer(I,1);
	cl_int result;
	size_t gsize[2];
	gsize[0] = I->width;
	gsize[1] = I->height;
	
	size_t lsize[2];
	lsize[0] = getLocalSize(I->width);
	lsize[1] = 1;
		
	cl_mem tmp = clCreateImage2D(context,CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, &format, I->width, I->height, 0,NULL,&result);
	
	check(clSetKernelArg(convolution_kernel,0,sizeof(cl_mem),&I->GPU_Image));
	check(clSetKernelArg(convolution_kernel,1,sizeof(cl_mem),&tmp));	
	check(clSetKernelArg(convolution_kernel,2,sizeof(cl_mem),&matrix->GPU_Matrix));	
	check(clSetKernelArg(convolution_kernel,3,sizeof(int),&matrix->dim));
	check(clSetKernelArg(convolution_kernel,4,sizeof(float),&matrix->div));
	check(clEnqueueNDRangeKernel(queue, convolution_kernel, 2, 0, gsize, NULL, 0, NULL, NULL));
	
	clReleaseMemObject(I->GPU_Image);
	I->GPU_Image = tmp;	
}

void blur(Image * I, int factor)
{
	convMatrix * tmp = new_conv(factor);
	for(int i = 0; i <tmp->size; i++)
	{
		tmp->CPU_Matrix[i] = 1.0;
	}
	convolve(I,tmp);
	del_conv(tmp);
}

ConvMatrix * new_conv(int dim_)
{
	ConvMatrix * tmp = (ConvMatrix*) malloc(sizeof(ConvMatrix));
	cl_int result;
	tmp->dim = dim_;
	tmp->size = (tmp->dim*2+1)*(tmp->dim*2+1);
	tmp->div = tmp->size;
	tmp->CPU_Matrix = (TYPE*) malloc(sizeof(TYPE)*tmp->size*tmp->size);
	tmp->GPU_Matrix = clCreateBuffer(context,CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, sizeof(TYPE)*tmp->size,NULL,&result);
	check(result);
	tmp->locality = 0;
}

ConvMatrix * merg_conv(ConvMatrix * one, ConvMatrix * two)
{
	cl_int result;
	ConvMatrix * tmp = (ConvMatrix*) malloc(sizeof(ConvMatrix));
	matrix_transfer(one,0);
	matrix_transfer(two,0);
	tmp->dim = one->dim + two->dim;
	tmp->div = (one->div * two->div);
	tmp->size = (tmp->dim*2+1)*(tmp->dim*2+1);

	tmp->CPU_Matrix = (TYPE*) malloc(sizeof(TYPE)*tmp->size);	
	tmp->GPU_Matrix = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, sizeof(TYPE)*tmp->size, NULL, &result);	
	check(result);

	for(int i = 0; i < tmp->size; i++)
	{
		tmp->CPU_Matrix[i] = 0;
	}	
	
	for(int i = -two->dim; i <= two->dim; i++)
	{
		for(int j = -two->dim; j <= two->dim; j++)
		{
			for(int k = -one->dim; k <= one->dim; k++)
			{
				for(int l = -one->dim; l <= one->dim; l++)
				{
					
					//printf("%d %d = %f * %f\n",(dim + i + k),(dim + j + l),two->CPU_Matrix[ (two->dim*2 + 1) * (two->dim + i) + (two->dim + j)], one->CPU_Matrix[ (one->dim*2 + 1) * (one->dim + k) + (one->dim + l)]);
					tmp->CPU_Matrix[(tmp->dim*2 + 1) * (tmp->dim + i + k) + (tmp->dim + j + l)] += 
						 two->CPU_Matrix[ (two->dim*2 + 1) * (two->dim + i) + (two->dim + j)] 
					       * one->CPU_Matrix[ (one->dim*2 + 1) * (one->dim + k) + (one->dim + l)];
								
				}
			}
		}
	}
	tmp->locality = 0;
}

void print(ConvMatrix * tmp,char * prefix)
{
	matrix_transfer(tmp,0);
	printf("%s div: %f dim: %d\n",prefix,tmp->div,tmp->dim);
	for(int i = 0; i < tmp->dim*2 + 1; i++)
	{
		for(int j = 0; j < tmp->dim*2 + 1; j++)
		{
			printf("%f\t",tmp->CPU_Matrix[(tmp->dim*2 +1)*(i) + j]);
		}
		printf("\n");
	}
	tmp->locality = 0;
}

void del_conv(ConvMatrix * tokill)
{
	delete tokill->CPU_Matrix;
	clReleaseMemObject(tokill->GPU_Matrix);
}

void matrix_transfer(ConvMatrix * tmp, int dest)
{
	if(dest == tmp->locality) return;
	else if(dest == 1)
	{
		clEnqueueWriteBuffer(queue,tmp->GPU_Matrix,CL_FALSE,0,tmp->size*sizeof(TYPE),tmp->CPU_Matrix,0,NULL,NULL);
		tmp->locality = 1;
	}
	else 
	{
		clEnqueueReadBuffer(queue,tmp->GPU_Matrix,CL_FALSE,0,tmp->size*sizeof(TYPE),tmp->CPU_Matrix,0,NULL,NULL);
		tmp->locality = 0;
	}
}
