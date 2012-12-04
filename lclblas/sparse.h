#ifndef SPARSE_H
#define SPARSE_H
#include "type.h"
#include <CL/cl.h>
#include <stdio.h>
#define COMMAND_QUEUE_ARGS NULL
//CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE 

/*Sparse Matrix data type*/
typedef struct sparsematrix
{
	//Matrix is stored in CSR format
	//These are the lengths of each field
	size_t valsLength;
	size_t colsLength;
	size_t indexLength;

	//These are the collection of values for the cpu
	int * cpu_cols;
	int * cpu_index;
	TYPE * cpu_vals;

	//the locality flag determines which device has the most up to date copy
	int locality;

	//these are pointers of the target devices memory buffers
	cl_mem gpu_vals;
	cl_mem gpu_cols;
	cl_mem gpu_index;
	
}SparseMatrix;

//Dense matrix data type
typedef struct matrix
{
	int width;
	int height;

	//number of values in matrix
	int length;

	//cpu data
	TYPE * cpu_vals;

	//gpu/opencl data 
	cl_mem  gpu_vals;

	//locality
	int locality;
}Matrix;

//vector data type
typedef struct vector
{
	//length of vector
	size_t length;

	//local work group size used for OpenCL
	size_t lsize;

	//cpu data
	TYPE * cpu_vals;

	//gpu/opencl data
	cl_mem gpu_vals;

	//locality flag
	int locality;
}Vector;

//diagonal banded matrix type, not used
typedef struct diag
{
	TYPE * vals;
	int width;
	size_t size;
	int locality;
}Diag;

/*
	Main initialization function.  It takes in two inputs:
		int platform: which OpenCL platform this will attempt to find a device on.  O is typically first GPU vendor
		int device: which device within the platform.  0 is typically your first GPU
	On a machine with 2 gpus and 1 CPU: 0 0 is first GPU, 0 1 is second GPU, 1 0 is CPU
	
	This initializes OpenCL on a target device, sets all the required global variables and loads all the required kernels.

	KFusion will create a new init function: fused_init, which will call this init function.  As such, init is required.
*/
void init(int platform, int device);

//sparse matrix functions
SparseMatrix * loadSparseMatrix(char * inputfile, int pad);
void sparse_transfer(SparseMatrix *s,int dest);

//vector functions
/*	NewVector
	Create a newVector of length len and local work group size lsize_.
	Local work group size is used be OpenCL when allocating work to the GPU
	
	Vector must be deleted using deleteVector(Vector * v)
*/
Vector * newVector(int len, int lsize_);

/* Deletes a vector*/
void deleteVector(Vector * v);

/* vector_gpu_dot
   Computes a dot product between two vectors and stores the result in cal
   This will be carried out on the gpu.  Init must be executed first

*/
void vector_gpu_dot(Vector * a, Vector * other, TYPE &val);

/*
   Add two vectors. The result is stored in c.  c[i] = alpha*a[i] + beta*b[i]
   This will be carried out on the gpu.  Init must be executed first
   */
void vector_gpu_add(Vector * c, double alpha, Vector * a, double beta, Vector * b);

/*
   vector_gpu_mult
   Multiply the elements of two vectors. The result is stored in c
   c[i] = alpha*a[i]*beta*b[i] 
   This will be carried out on the gpu.  Init must be executed first
   */
void vector_gpu_mult(Vector * c, double alpha, Vector * a, double beta, Vector * b);

/*
   compute the square root for each element in the vectors
   This will be carried out on the gpu.  Init must be executed first
   */
void vector_gpu_sqrt(Vector *c, Vector * a);

/*
   Transfer a vector to or from the GPU
   1 is GPU
   0 is CPU
*/
void vector_transfer(Vector * v,int dest);

/*
	Set all the elements in a vector to a value: val
*/
void vector_set(Vector * v,double val);


/*
	Copy the values in one vector to another
*/
void vector_copy(Vector * dest, Vector * src);

/*
   Print out the values in a vector with a given prefix as specified by pref
*/
void vector_print(vector * v, char * pref);

/*
   Print out the value in a vector with a given prefix specified by pref.  Only "total" values will be printed.  To print the first 10 values of a vector total = 10.
*/
void vector_print2(vector * v, char * pref, int total);

//matrix operations

//create a new dense matrix x by y
Matrix * newMatrix(int x, int y);

//delete the matrix
void deleteMatrix(Matrix * m);

void matrix_transfer(Matrix * v, int dest);

//multiply a sparse matrix A by a vector b storing the result in c
void sparse_mult(Vector * c,SparseMatrix * A, Vector * b);

//solve Ax = b for a sprars matrix using the conjugate gradient method and itr iterations
void sparse_solve(Vector * x, SparseMatrix * A, Vector * b, int itr);

//multiply a dense matrix M by x storing the result in c.
void denseMult(vector * c, Matrix * m, Vector * x);

//Solve Ax=b for a dense matrix using the conjugate gradient method and itr iterations
void solve(Vector * x, Matrix * A, Vector * b, int itr);


//OpenCL Info
/* this information is a series of globa variables which are 
   used to keep track of the OpenCL state as well as load a 
   series of kernels
   
   These will all be altered by the init function
*/
//platform information
extern cl_platform_id	platforms[16];
extern cl_uint 	num_platforms;
extern int 		param_value_buffer[8];
extern size_t 		param_value_size_ret;

//device infomation
extern cl_device_type device_type[8];
extern cl_device_id   device_list[8];

//context and queue information
extern cl_context     	 context;
extern cl_command_queue queue;

//kernel information
extern cl_kernel dot_kernel;
extern cl_kernel mult_kernel;
extern cl_kernel vmult_kernel;
extern cl_kernel vsqrt_kernel;
extern cl_kernel add_kernel;
extern cl_kernel sum_kernel;
extern cl_kernel denseMult_kernel;
extern cl_program program;
#endif
