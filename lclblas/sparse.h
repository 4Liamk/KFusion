#ifndef SPARSE_H
#define SPARSE_H
#include "type.h"
#include <CL/cl.h>
#include <stdio.h>
#define COMMAND_QUEUE_ARGS NULL
//CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE 
typedef struct sparsematrix
{
	int * cpu_cols;
	int * cpu_index;
	TYPE * cpu_vals;
	size_t valsLength;
	size_t colsLength;
	size_t indexLength;
	int locality;
	cl_mem gpu_vals;
	cl_mem gpu_cols;
	cl_mem gpu_index;
}SparseMatrix;

typedef struct matrix
{
	int width;
	int height;
	int length;
	TYPE * cpu_vals;
	cl_mem  gpu_vals;
	int locality;
}Matrix;

typedef struct vector
{
	size_t length;
	size_t lsize;
	TYPE * cpu_vals;
	cl_mem gpu_vals;
	int locality;
}Vector;

typedef struct diag
{
	TYPE * vals;
	int width;
	size_t size;
	int locality;
}Diag;
	
void init(int platform, int device);

//sparse matrix functions
SparseMatrix * loadSparseMatrix(char * inputfile, int pad);
void sparse_transfer(SparseMatrix *s,int dest);

//vector functions
Vector * newVector(int len, int lsize_);
void deleteVector(Vector * v);
void vector_gpu_dot(Vector * a, Vector * other, TYPE &val);
void vector_gpu_add(Vector * c, double alpha, Vector * a, double beta, Vector * b);
void vector_gpu_mult(Vector * c, double alpha, Vector * a, double beta, Vector * b);
void vector_gpu_sqrt(Vector *c, Vector * a);
void vector_transfer(Vector * v,int dest);
void vector_set(Vector * v,double val);
void vector_copy(Vector * dest, Vector * src);
void vector_print(vector * v, char * pref);
void vector_print2(vector * v, char * pref, int total);

//matrix operations
void sparse_mult(Vector * c,SparseMatrix * a, Vector * b);
void sparse_solve(Vector * v, SparseMatrix * a, Vector * b, int itr);
void denseMult(vector * c, Matrix * m, Vector * x);
void solve(Vector * v, Matrix * m, Vector * b, int itr);
Matrix * newMatrix(int x, int y);
void deleteMatrix(Matrix * m);
void matrix_transfer(Matrix * v, int dest);


//OpenCL Info
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

extern cl_kernel dot_kernel;
extern cl_kernel mult_kernel;
extern cl_kernel vmult_kernel;
extern cl_kernel vsqrt_kernel;
extern cl_kernel add_kernel;
extern cl_kernel sum_kernel;
extern cl_kernel denseMult_kernel;
extern cl_program program;
#endif
