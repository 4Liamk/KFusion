#include "sparse.h"
#include "mreader.h"
#include "check.h"
#include "reader.h"

cl_platform_id	platforms[16];
cl_uint 	num_platforms;
int 		param_value_buffer[8];
size_t 	param_value_size_ret;

//device infomation
cl_device_type device_type[8];
cl_device_id   device_list[8];

//context and queue information
cl_context context;
cl_command_queue queue;

cl_kernel dotproduct_kernel;
cl_kernel denseMult_kernel;
cl_kernel vmult_kernel;
cl_kernel add_kernel;
cl_kernel sum_kernel;
cl_kernel vsqrt_kernel;
cl_kernel mult_kernel;
cl_program program;
#define DEBUG 0

void init(int platform, int device)
{
	if(DEBUG) printf("----------------------------------------------\n");
	if(DEBUG) printf("------INITIATING OPENCL PLEASE STAND BY-------\n");
	if(DEBUG) printf("----------------------------------------------\n");

	unsigned int i; 
	int result;
	//platform information
 	if(DEBUG) perror("getting basic openCL data: platform info and number of devices");
	
	check(clGetPlatformIDs(8,platforms, &num_platforms));
	printf("num platforms: %d\n",num_platforms);
	//get info for first platform
	unsigned int num_devices = 0;
	for(i = 0; i < num_platforms; i++)
	{
		if(DEBUG) printf("Platform %d\n", i);
		check(clGetPlatformInfo(platforms[i], CL_PLATFORM_PROFILE, (size_t) 32, (void*)	param_value_buffer, &param_value_size_ret));
 		if(DEBUG) printf("\tProfile: %s\n",(char*) param_value_buffer);	
		check(clGetPlatformInfo(platforms[i], CL_PLATFORM_VERSION , (size_t) 32, (void*)	param_value_buffer, &param_value_size_ret));
 		if(DEBUG) printf("\tVersion: %s\n", (char*) param_value_buffer);	
		check(clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, (size_t) 32, (void*)	param_value_buffer, &param_value_size_ret));
 		if(DEBUG) printf("\tName: %s\n", (char*) param_value_buffer);	
		check(clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, (size_t) 32, (void*)	param_value_buffer, &param_value_size_ret));
 		if(DEBUG) printf("\tVendor: %s\n",(char*)  param_value_buffer);	
		//check(clGetPlatformInfo(platforms[i], CL_PLATFORM_EXTENSIONS, (size_t) 32,(void*) param_value_buffer, &param_value_size_ret));
 		if(DEBUG) printf("\tExtensions: %s\n",(char*)  param_value_buffer);	
		check(clGetPlatformInfo(platforms[i], CL_PLATFORM_PROFILE, (size_t) 32, (void*)	param_value_buffer, &param_value_size_ret));	
		check(clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 8, device_list, &num_devices));
	
		char p1[512];
		p1[511] = '\0';
		if(DEBUG) printf("\tnum devices: %d\n\n", num_devices);
		for(i = 0; i < num_devices; i++)
		{
			cl_ulong size;
			check(clGetDeviceInfo(device_list[i], CL_DEVICE_NAME, 512, (void*) p1, NULL));
			if(DEBUG) printf("\tdevice %d: %s\n", i, p1);
			check(clGetDeviceInfo(device_list[i], CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, sizeof(cl_ulong), (void*) &size, NULL));
			if(DEBUG) printf("\t\tGlobal Cache Size: %d\n", i, size);
			check(clGetDeviceInfo(device_list[i], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), (void*) &size, NULL));
			if(DEBUG) printf("\t\tGlobal Mem Size: %d\n", i, size);
			check(clGetDeviceInfo(device_list[i], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), (void*) &size, NULL));
			if(DEBUG) printf("\t\tlocal mem size: %d\n",size);
			check(clGetDeviceInfo(device_list[i], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), (void*) &size, NULL));		
			if(DEBUG) printf("\t\tmemory available: %d\n", i, p1[0]);
		}	
	}
	check(clGetDeviceIDs(platforms[platform], CL_DEVICE_TYPE_ALL, 8, device_list, &num_devices));
	//create compute context over all available devices
	context = clCreateContext(NULL,1,&device_list[device],NULL,NULL,&result);	
	//check(result);	

	//get devices available to context
	size_t nContextDescriptorSize = 0;
	
	clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, 0, &nContextDescriptorSize);
	
	cl_device_id * aDevices = (cl_device_id*) malloc(nContextDescriptorSize);
	
	clGetContextInfo(context, CL_CONTEXT_DEVICES, nContextDescriptorSize, aDevices, 0);	

	//make cpu queue
	queue = clCreateCommandQueue(context,aDevices[0],COMMAND_QUEUE_ARGS,&result);
	//check(result);

	const char* program_source = (char*) readin("kernel.cl");
	
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
    	if(DEBUG) perror("program loaded");

	//build for cpu
	result = clBuildProgram(program,1,aDevices,NULL,NULL,NULL);
	if (result != CL_SUCCESS)
  	{
  		check(result);
  		if(DEBUG) printf("build fail\n");
        	size_t len;
        	char buffer[5000];
        	for(i = 0; i < num_devices; i++)
        	{
        		memset(buffer,0,sizeof(buffer));
      		  	clGetProgramBuildInfo(program, device_list[i], CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        		printf("%s\n", buffer);
		}
        	exit(1);

    	}
    	
    	if(DEBUG) perror("program built");
	//build compute kernel
	dotproduct_kernel = clCreateKernel(program,"dotproduct",&result);
	sum_kernel = clCreateKernel(program,"sum",&result);
	mult_kernel = clCreateKernel(program,"mult",&result);
	vmult_kernel = clCreateKernel(program,"vmult",&result);
	vsqrt_kernel = clCreateKernel(program,"vsqrt",&result);		
	add_kernel = clCreateKernel(program,"add",&result);
	denseMult_kernel = clCreateKernel(program,"denseMult",&result);
	if(DEBUG) perror("compute kernels ready");	
		
}

SparseMatrix * loadSparseMatrix(char * filename, int pad)
{
	char input[128];
	strcpy(input,filename);
	strcat(input,".bin");
	printf("loading binary: %s\n", input);
	SparseMatrix * m = bmpaddedread(input,pad);
	if(m == NULL)
	{
		strcpy(input,filename);
		strcat(input,".txt");
		printf("binary not found, loading text and then saving as binary for next time: %s\n",input);
		m = mread(input);
		strcphttps://mail.google.com/mail/u/0/?ui=2&shva=1#inboxy(input,filename);
		strcat(input,".bin");
		bmwrite(input,m);
	}
	assert(m);
	int i;
	for(i = 0; i < 10; i++)
	{
		printf("%d, %e, %d\n",m->cpu_cols[i],m->cpu_vals[i],m->cpu_index[i]);
	}
	int result;
	//make the gpu side data structure
	m->gpu_cols = clCreateBuffer(context,CL_MEM_READ_WRITE, sizeof(int)*m->colsLength,NULL,&result);
	m->gpu_vals = clCreateBuffer(context,CL_MEM_READ_WRITE, sizeof(TYPE)*m->colsLength,NULL,&result);
	m->gpu_index = clCreateBuffer(context,CL_MEM_READ_WRITE, sizeof(int)*m->indexLength,NULL,&result);
	m->locality = 0;
	return m;
}

void sparse_transfer(SparseMatrix * m, int dest)
{
	if(m->locality == dest) return;
	else
	{
		//transfer to CPU
		if(dest == 0)
		{
			clEnqueueReadBuffer(queue,m->gpu_cols,CL_FALSE,0,sizeof(int)*m->colsLength,m->cpu_cols,0,NULL,NULL);
			clEnqueueReadBuffer(queue,m->gpu_vals,CL_FALSE,0,sizeof(TYPE)*m->colsLength,m->cpu_vals,0,NULL,NULL);
			clEnqueueReadBuffer(queue,m->gpu_index,CL_FALSE,0,sizeof(int)*m->indexLength,m->cpu_index,0,NULL,NULL);
			m->locality = 0;
		}
		else
		{
			clEnqueueWriteBuffer(queue,m->gpu_cols,CL_FALSE,0,sizeof(int)*m->colsLength,m->cpu_cols,0,NULL,NULL);
			clEnqueueWriteBuffer(queue,m->gpu_vals,CL_FALSE,0,sizeof(TYPE)*m->colsLength,m->cpu_vals,0,NULL,NULL);
			clEnqueueWriteBuffer(queue,m->gpu_index,CL_FALSE,0,sizeof(int)*m->indexLength,m->cpu_index, 0,NULL, NULL);
			m->locality = 1;
		}
	}
}

void vector_transfer(Vector * v, int dest)
{
	if(v->locality == dest) return;
	else
	{
		//transfer to CPU
		if(dest == 0)
		{
			if(DEBUG) printf("Moving to CPU\n");
			clEnqueueReadBuffer(queue,v->gpu_vals,CL_TRUE,0,sizeof(TYPE)*v->length,v->cpu_vals,0,NULL,NULL);
			v->locality = 0;
		}
		else
		{
			if(DEBUG) printf("Moving to GPU\n");
			clEnqueueWriteBuffer(queue,v->gpu_vals,CL_TRUE,0,sizeof(TYPE)*v->length,v->cpu_vals,0,NULL,NULL);
			v->locality = 1; 		
		}
	}
}

Vector * newVector(int len, int lsize_)
{
	Vector * tmp = (Vector *)malloc(sizeof(Vector));
	tmp->length = len;
	tmp->lsize = lsize_;
	tmp->cpu_vals = (TYPE*) malloc(sizeof(TYPE)*len);
	int result;
	tmp->gpu_vals = clCreateBuffer(context,CL_MEM_READ_WRITE, sizeof(TYPE)*tmp->length, NULL, &result);
	check(result);
	tmp->locality = 0;
	return tmp;
}

void deleteVector(Vector * v)
{
	free(v->cpu_vals);
	clReleaseMemObject(v->gpu_vals);
	free(v);
}

void deleteMatrix(Matrix * m)
{
	free(m->cpu_vals);
	clReleaseMemObject(m->gpu_vals);
	free(m);
}

void vector_gpu_add(Vector * c,double alpha, Vector * a, double beta, Vector * b)
{
	//ensure all data is on the GPU and as we are writing to this, our locality becomes the gpu
	vector_transfer(a,1);
	vector_transfer(b,1);
	c->locality = 1;
	//set kernel args
	check(clSetKernelArg(add_kernel,0,sizeof(cl_mem),&c->gpu_vals));
	check(clSetKernelArg(add_kernel,1,sizeof(TYPE),&alpha));
	check(clSetKernelArg(add_kernel,2,sizeof(cl_mem),&a->gpu_vals));
	check(clSetKernelArg(add_kernel,3,sizeof(TYPE),&beta));
	check(clSetKernelArg(add_kernel,4,sizeof(cl_mem),&b->gpu_vals));
	check(clEnqueueNDRangeKernel(queue,add_kernel,1,0,&c->length,&c->lsize,0,NULL,NULL));	

}

void vector_gpu_mult(Vector * c,double alpha, Vector * a, double beta, Vector * b)
{
	//ensure all data is on the GPU and as we are writing to this, our locality becomes the gpu
	vector_transfer(a,1);
	vector_transfer(b,1);
	c->locality = 1;
	//printf("mult for size %d %d\n",c->length,c->lsize);
	//set kernel args
	check(clSetKernelArg(vmult_kernel,0,sizeof(cl_mem),&c->gpu_vals));
	check(clSetKernelArg(vmult_kernel,1,sizeof(TYPE),&alpha));
	check(clSetKernelArg(vmult_kernel,2,sizeof(cl_mem),&a->gpu_vals));
	check(clSetKernelArg(vmult_kernel,3,sizeof(TYPE),&beta));
	check(clSetKernelArg(vmult_kernel,4,sizeof(cl_mem),&b->gpu_vals));
	check(clEnqueueNDRangeKernel(queue,vmult_kernel,1,0,&c->length,&c->lsize,0,NULL,NULL));	

}

void vector_gpu_sqrt(Vector * c, Vector * a)
{
	//ensure all data is on the GPU and as we are writing to this, our locality becomes the gpu
	vector_transfer(a,1);
	c->locality = 1;
	
	//set kernel args
	check(clSetKernelArg(vsqrt_kernel,0,sizeof(cl_mem),&c->gpu_vals));
	check(clSetKernelArg(vsqrt_kernel,1,sizeof(cl_mem),&a->gpu_vals));
	check(clEnqueueNDRangeKernel(queue,vsqrt_kernel,1,0,&c->length,&c->lsize,0,NULL,NULL));	

}

void __sum(cl_mem * tmp, TYPE &val, int call, int reductionlength,size_t woot)
{
	cl_mem result	= clCreateBuffer(context,CL_MEM_READ_WRITE,sizeof(TYPE),NULL,&call);
	check(clSetKernelArg(sum_kernel,0,sizeof(cl_mem),tmp));
	check(clSetKernelArg(sum_kernel,1,sizeof(cl_mem),&result));
	check(clSetKernelArg(sum_kernel,2,sizeof(int),&reductionlength));
	check(clEnqueueNDRangeKernel(queue,sum_kernel,1,0,&woot,&woot,0,NULL,NULL));	
	check(clEnqueueReadBuffer(queue,result,CL_TRUE,0,sizeof(TYPE),&val,0,NULL,NULL));
	clReleaseMemObject(result);	
}

#pragma synchronize out
void vector_gpu_dot(Vector * a, Vector * other, TYPE &val)
{
	vector_transfer(a,1);
	vector_transfer(other,1);
	
	int call;
	size_t woot = a->lsize;
	int reductionlength = a->length/a->lsize;
	size_t woot2 = a->lsize/2;
	
	cl_mem tmp 	= clCreateBuffer(context,CL_MEM_READ_WRITE,sizeof(TYPE)*woot,NULL,&call);
	cl_mem result	= clCreateBuffer(context,CL_MEM_READ_WRITE,sizeof(TYPE),NULL,&call);
	
	check(clSetKernelArg(dotproduct_kernel,0,sizeof(cl_mem),&a->gpu_vals));
	check(clSetKernelArg(dotproduct_kernel,1,sizeof(cl_mem),&other->gpu_vals));
	check(clSetKernelArg(dotproduct_kernel,2,sizeof(cl_mem),&tmp));
	check(clEnqueueNDRangeKernel(queue,dotproduct_kernel,1,0,&a->length,&a->lsize,0,NULL,NULL));	
	
	__sum(&tmp, val,call,reductionlength,woot);
	clReleaseMemObject(tmp);
} 
void vector_set(Vector * v, double val)
{
	v->locality = 0;
	for(int i = 0; i < v->length; i++)
	{
		v->cpu_vals[i] = val;
	}
}
#pragma synchronize in
void vector_copy(Vector * src, Vector * dest)
{
	/*
	cl_int clEnqueueCopyBuffer ( 	cl_command_queue command_queue,
	  	cl_mem src_buffer,
	  	cl_mem dst_buffer,
	  	size_t src_offset,
	  	size_t dst_offset,
	  	size_t cb,
	  	cl_uint num_events_in_wait_list,
	  	const cl_event *event_wait_list,
	  	cl_event *event)*/
	clEnqueueCopyBuffer(queue,src->gpu_vals,dest->gpu_vals,0,0,sizeof(TYPE)*src->length,0,NULL,NULL);
	src->locality = 1;
}

void sparse_mult(Vector * b, SparseMatrix * A, Vector * x)
{
	vector_transfer(b,1);
	vector_transfer(x,1);
	sparse_transfer(A,1);
	b->locality = 1;
	//__kernel void mult(__global const int * cols, __global const TYPE * vals,__global const int * index, __global const TYPE * x, __global TYPE * b)
	check(clSetKernelArg(mult_kernel,0,sizeof(cl_mem),&(A->gpu_cols)));
	check(clSetKernelArg(mult_kernel,1,sizeof(cl_mem),&(A->gpu_vals)));
	check(clSetKernelArg(mult_kernel,2,sizeof(cl_mem),&(A->gpu_index)));
	check(clSetKernelArg(mult_kernel,3,sizeof(cl_mem),&(x->gpu_vals)));
	check(clSetKernelArg(mult_kernel,4,sizeof(cl_mem),&b->gpu_vals));
	check(clEnqueueNDRangeKernel(queue,mult_kernel,1,0,&b->length,&b->lsize,0,NULL,NULL));	
}

void vector_print(Vector * v,char * prefix)
{
	vector_transfer(v,0);
	for(int i = 0; i < v->length; i++)
	{
		printf("%s%e\n",prefix, v->cpu_vals[i]);
	}
}

void vector_print2(Vector * v,char * prefix, int total)
{
	vector_transfer(v,0);
	for(int i = 0; i < total; i++)
	{
		printf("%s%e\n",prefix, v->cpu_vals[i]);
	}
}

Matrix * newMatrix(int x, int y)
{
	Matrix * a = (Matrix*) malloc(sizeof(Matrix));
	a->width = x;
	a->height = y;
	a->cpu_vals = (TYPE*) malloc(sizeof(TYPE*)*x*y);
	cl_int result;
	a->gpu_vals = clCreateBuffer(context,CL_MEM_READ_WRITE, sizeof(TYPE)*x*y, NULL, &result); 
	a->locality = 0;
	check(result);
	return a;
}

void matrix_transfer(Matrix * A, int dest)
{
	if(A->locality == dest)
	{
		return;
	}
	else if(dest == 0)
	{
		check(clEnqueueReadBuffer(queue,A->gpu_vals,CL_FALSE,0,sizeof(TYPE)*A->width*A->height,A->cpu_vals,0,NULL,NULL));
	}
	else if(dest == 1)
	{
		check(clEnqueueWriteBuffer(queue,A->gpu_vals,CL_FALSE,0,sizeof(TYPE)*A->width*A->height,A->cpu_vals,0,NULL,NULL));		
	}
	A->locality = dest;
}
#pragma synchronize in
void denseMult(Vector * b, Matrix * A, Vector * x)
{
	vector_transfer(x,1);
	vector_transfer(b,1);
	matrix_transfer(A,1);
	check(clSetKernelArg(denseMult_kernel,0,sizeof(cl_mem),&b->gpu_vals));	
	check(clSetKernelArg(denseMult_kernel,1,sizeof(cl_mem),&A->gpu_vals));
	check(clSetKernelArg(denseMult_kernel,2,sizeof(cl_mem),&x->gpu_vals));	
	check(clSetKernelArg(denseMult_kernel,3,sizeof(int),&A->width));
	check(clEnqueueNDRangeKernel(queue,denseMult_kernel,1,0,&b->length,&b->lsize,0,NULL,NULL));
}
