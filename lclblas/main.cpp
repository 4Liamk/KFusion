#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <CL/cl.h>
#include <string.h>
#include <assert.h>
#include "sparse.h"
#include "reader.h"
#include "mreader.h"
#include "type.h"
#include "sys/time.h"

char * MATRIX;
int LSIZE = 128;

/*
 This is the linear algebra case study.  It tests 3 fairly simple sets of equatiations and compares the fused vs unfused execution times

 If operating correctly, it will only ouput execution times, the format of which can be inferred in the main method.  It measures both transfer time + execution time as well as just execution time

 if incorrect, it will print out and error message with the expected and incorrect value.  These tests are inheriently self testing in way.

 More information on these tests can be found here: https://github.com/4Liamk/KFusion/wiki/Linear-algebra
*/

//flags to ensure correctness
#define DEBUG 0 //debug flag will make execution verbose if 1
#define CHECK 1 //check flag will check all results to ensure they are correct.  If incorrect, it will print out the incorrect matching pair.  Enabled by default currently

//Simple helper function for getting the time as a double
double gettime()
{
	struct timeval start;
	gettimeofday(&start,NULL);
	return (double)start.tv_sec + start.tv_usec/1000000.0;
}

/*
  Low Dependency Case Fused
  This calculates the pythagoran theorem.
  The other test cases are very similar in design and follow similiar logic as well as setup and teardown requirements
*/
int trianglefused(int size)
{
	if(DEBUG) perror("Triangle");
	//initial setup: create three vectors
	Vector * a = newVector(size,LSIZE);
	Vector * b = newVector(size,LSIZE);
	Vector * c = newVector(size,LSIZE);

	//set the vectors to a known value
	for(int i = 0; i < size; i++)
	{
		a->cpu_vals[i] = i;
		b->cpu_vals[i] = i;
	}

	//begin timing of execution and transfer to GPU
	double outerStart = gettime();
	vector_transfer(a,1);
	vector_transfer(b,1);
		
		//finish transfer
		clFinish(queue);
		double innerStart = gettime();
		
		//execute operation, in this case they are fused by the addition of the #pragmas
		#pragma startfuse
		vector_gpu_mult(a,1,a,1,a);
		vector_gpu_mult(b,1,b,1,b);
		vector_gpu_add (c,1,a,1,b);
		vector_gpu_sqrt(c,c);
		#pragma endfuse	
		
		//wait for completion in order to accurately time it
		clFinish(queue);	
		double innerEnd = gettime();
		
		//transfer back results
		vector_transfer(c,0);
		clFinish(queue);
	double outerEnd = gettime();
	
	//print execution time
	printf("%f\t%f\t",outerEnd - outerStart, innerEnd - innerStart);
	
	//if check is enabled, self check results:
	if(CHECK)
	{
		printf("\n");
		for(int i = 0; i < size; i++)
		{
			TYPE result = sqrt(i*i + i*i);
			if(result != c->cpu_vals[i])
			{
				printf("failed on %d: %f != %f\n",i,result, c->cpu_vals[i]);
				return 1;
			}
		}
	}
	deleteVector(a);
	deleteVector(b);
	deleteVector(c);	
	return 0;		
}

int triangle(int size)
{
	if(DEBUG) perror("Triangle Fused");
	//setup
	Vector * a = newVector(size,LSIZE);
	vector * b = newVector(size,LSIZE);
	vector * c = newVector(size,LSIZE);
	for(int i = 0; i < size; i++)
	{
		a->cpu_vals[i] = i;
		b->cpu_vals[i] = i;
	}
	
	//data transfer
	double outerStart = gettime();
	vector_transfer(a,1);
	vector_transfer(b,1);
		clFinish(queue);
		
		//operations - unfused
		double innerStart = gettime();
		vector_gpu_mult(a,1,a,1,a);
		vector_gpu_mult(b,1,b,1,b);
		vector_gpu_add (c,1,a,1,b);
		vector_gpu_sqrt(c,c);	
		clFinish(queue);
		//time and transfer back	
		double innerEnd = gettime();
		vector_transfer(c,0);
		clFinish(queue);
	double outerEnd = gettime();
	printf("%f\t%f\t",outerEnd - outerStart, innerEnd - innerStart);

	//self check
	if(CHECK)
	{
		printf("\n");
		for(int i = 0; i < size; i++)
		{
			TYPE result = sqrt(i*i + i*i);
			if(result != c->cpu_vals[i])
			{
				printf("failed on %d/%d: %f %f %f != %f\n",i,size,a->cpu_vals[i],b->cpu_vals[i],result, c->cpu_vals[i]);
				return 1;
			}
		}
	}
	deleteVector(a);
	deleteVector(b);
	deleteVector(c);
	return trianglefused(size);
}


int distance2(int size)
{
	if(DEBUG) perror("Distance Fused");
	
	//setup 
	Vector * x1 = newVector(size,LSIZE);
	vector * x2 = newVector(size,LSIZE);
	Vector * y1 = newVector(size,LSIZE);
	vector * y2 = newVector(size,LSIZE);
	vector * c = newVector(size,LSIZE);
	
	//assigment
	for(int i = 0; i < size; i++)
	{
		x1->cpu_vals[i] = i;
		x2->cpu_vals[i] = i*2;
		y1->cpu_vals[i] = i;
		y2->cpu_vals[i] = i*2;		
	}	
	

	double outerStart = gettime();	
	//transfer
	vector_transfer(x1,1);
	vector_transfer(x2,1);
	vector_transfer(y1,1);
	vector_transfer(y2,1);	
		clFinish(queue);

		//operations: fused
		double innerStart = gettime();	
		#pragma startfuse
		vector_gpu_add(x1,1,x1,-1,x2);
		vector_gpu_add(y1,1,y1,-1,y2);
		vector_gpu_mult(x1,1,x1,1,x1);
		vector_gpu_mult(y1,1,y1,1,y1);	
		vector_gpu_add(c,1,x1,1,y1);	
		vector_gpu_sqrt(c,c);
		#pragma endfuse		
		clFinish(queue);
		double innerEnd = gettime();			
		//transfer back to CPU
		vector_transfer(c,0);
		vector_transfer(y1,0);
		vector_transfer(x1,0);
		clFinish(queue);
	double outerEnd = gettime();
	printf("%f\t%f\t",outerEnd - outerStart, innerEnd - innerStart);	
	if(CHECK)
	{
		printf("\n");
		for(int i = 0; i < size; i++)
		{
			TYPE result = sqrt( (i-2*i)*(i-2*i) + (i-2*i)*(i-2*i));
			if(result != c->cpu_vals[i])
			{
				printf("failed on %d: %f %f %f != %f\n",i,x1->cpu_vals[i],y1->cpu_vals[i],c->cpu_vals[i],result);
				return 1;
			}
		}
	}	
	deleteVector(c);
	deleteVector(x1);
	deleteVector(x2);
	deleteVector(y1);
	deleteVector(y2);		
	return 0;
}

//distance calculates the distance between series of 2 x,y coordinates
int distance(int size)
{
	if(DEBUG) perror("Distance");
	//setup
	Vector * x1 = newVector(size,16);
	vector * x2 = newVector(size,16);
	Vector * y1 = newVector(size,16);
	vector * y2 = newVector(size,16);
	vector * c = newVector(size,16);

	//assigment
	for(int i = 0; i < size; i++)
	{
		x1->cpu_vals[i] = i;
		x2->cpu_vals[i] = i*2;
		y1->cpu_vals[i] = i;
		y2->cpu_vals[i] = i*2;		
	}	

	//transfer
	double outerStart = gettime();
	vector_transfer(x1,1);
	vector_transfer(x2,1);
	vector_transfer(y1,1);
	vector_transfer(y2,1);	
		clFinish(queue);

		//do the operations
		double innerStart = gettime();	
		vector_gpu_add(x1,1,x1,-1,x2);
		vector_gpu_add(y1,1,y1,-1,y2);
		vector_gpu_mult(x1,1,x1,1,x1);
		vector_gpu_mult(y1,1,y1,1,y1);	
		vector_gpu_add(c,1,x1,1,y1);	
		vector_gpu_sqrt(c,c);
		clFinish(queue);

		//transfer
		double innerEnd = gettime();			
		vector_transfer(c,0);
		vector_transfer(y1,0);
		vector_transfer(x1,0);
		clFinish(queue);
	double outerEnd = gettime();
	printf("%f\t%f\t",outerEnd - outerStart, innerEnd - innerStart);	

	//self check results
	if(CHECK)
	{
		printf("\n");
		for(int i = 0; i < size; i++)
		{
			TYPE result = sqrt( (i-2*i)*(i-2*i) + (i-2*i)*(i-2*i));
			if(result != c->cpu_vals[i])
			{
				printf("failed on %d: %f %f %f != %f\n",i,result,x1->cpu_vals[i],y1->cpu_vals[i], c->cpu_vals[i]);
				return 1;
			}
		}
	}

	deleteVector(c);
	deleteVector(x1);
	deleteVector(x2);
	deleteVector(y1);
	deleteVector(y2);
	return distance2(size);		
	return 0;			
}

//first few operations in the conjugate gradient method -- fused
int cgStartFused(int size,Vector * result)
{
	if(DEBUG) perror("Start Fused");
	Vector * x = newVector(size,32);
	Vector * b = newVector(size,16);
	Vector * b2= newVector(size,16);
	Vector * r = newVector(size,16);
	Vector * p = newVector(size,16);
	Matrix * A = newMatrix(size,size);
	for(int i = 0; i < size; i++)
	{
		x->cpu_vals[i] = i;
		b->cpu_vals[i] = i;
		b2->cpu_vals[i] = i;
		r->cpu_vals[i] = i;
	}	
	for(int i = 0; i < size*size; i++)
	{
		A->cpu_vals[i] = 1;
	}
	double outerStart = gettime();
	vector_transfer(x,1);
	vector_transfer(b,1);
	vector_transfer(b2,1);
	vector_transfer(r,1);
	matrix_transfer(A,1);
		clFinish(queue);
		double innerStart = gettime();
		#pragma startfuse
		denseMult(b,A,x);
		vector_gpu_add(r,1,b2,-1,b);
		vector_gpu_add(p,1,r,0,r);
		vector_gpu_mult(r,1,p,1,p);
		#pragma endfuse
		clFinish(queue);	
		double innerEnd = gettime();
		vector_transfer(r,0);
		clFinish(queue);
	double outerEnd = gettime();
	printf("%f\t%f\t",outerEnd - outerStart, innerEnd - innerStart);	
	if(CHECK)
	{
		printf("\n");
		for(int i = 0; i < size; i++)
		{
			if(r->cpu_vals[i] != result->cpu_vals[i])
			{
				printf("%d: %f != %f\n",i,r->cpu_vals[i], result->cpu_vals[i]);
				return 1;
			}
		}
	}	
	deleteVector(x);
	deleteVector(b);
	deleteVector(r);
	deleteVector(b2);
	deleteVector(p);	
	deleteMatrix(A);
	deleteVector(result);
	return 0;
}

//the first few operations of the conjugate gradient algorithm
int cgStart(int size)
{
	if(DEBUG) perror("Start");
	Vector * x = newVector(size,32);
	Vector * b = newVector(size,16);
	Vector * b2= newVector(size,16);
	Vector * r = newVector(size,16);
	Vector * p = newVector(size,16);
	Matrix * A = newMatrix(size,size);
	for(int i = 0; i < size; i++)
	{
		x->cpu_vals[i] = i;
		b->cpu_vals[i] = i;
		b2->cpu_vals[i] = i;
		r->cpu_vals[i] = i;
	}	
	for(int i = 0; i < size*size; i++)
	{
		A->cpu_vals[i] = 1;
	}
	double outerStart = gettime();
	vector_transfer(x,1);
	vector_transfer(b,1);
	vector_transfer(b2,1);
	vector_transfer(r,1);
	matrix_transfer(A,1);
		clFinish(queue);
		double innerStart = gettime();
		denseMult(b,A,x);
		vector_gpu_add(r,1,b2,-1,b);
		vector_gpu_add(p,1,r,0,r);
		vector_gpu_mult(r,1,p,1,p);
		clFinish(queue);	
		double innerEnd = gettime();
		vector_transfer(r,0);
		clFinish(queue);
	double outerEnd = gettime();
	printf("%f\t%f\t",outerEnd - outerStart, innerEnd - innerStart);	


	if(CHECK)
	{
		printf("\n");
		for(int i = 0; i < size; i++)
		{
			//printf("%d: %f\n",i,r->cpu_vals[i]);
		}
	}	
	deleteVector(x);
	deleteVector(b);
	deleteVector(b2);
	deleteVector(p);	
	deleteMatrix(A);
	return cgStartFused(size,r);
	

}


int solvefused(int size, int itr, Vector * xcomp)
{
	if(DEBUG) perror("Solve Fuse");
	Matrix * A = newMatrix(size,size);
	Vector * r = newVector(size,LSIZE);
	Vector * b = newVector(size,LSIZE);
	Vector * p = newVector(size,LSIZE);
	Vector * tmp = newVector(size,LSIZE);
	Vector * x = newVector(size,LSIZE);
	TYPE rtr;

	for(int i = 0; i < size; i++)
	{
		r->cpu_vals[i] = 0;
		b->cpu_vals[i] = 0;
		p->cpu_vals[i] = 0;
		x->cpu_vals[i] = 0;
		tmp->cpu_vals[i]=0;
		for(int j = 0; j < size; j++)
		{
			A->cpu_vals[i+j*size] = i*j;
			b->cpu_vals[i] += 10*j*i;
		}
	}
	
	double outerStart = gettime();
	vector_transfer(x,1);
	vector_transfer(b,1);
	vector_transfer(tmp,1);
	vector_transfer(r,1);
	vector_transfer(p,1);	
	matrix_transfer(A,1);
		clFinish(queue);
		double innerStart = gettime();
			#pragma startfuse
			denseMult(tmp,A,x);	
			vector_gpu_add(r,1,b,-1,tmp);		
			#pragma endfuse	
			if(DEBUG)
				vector_print2(r,"r: ",10);
			#pragma startfuse
			vector_gpu_add(p,1,r,0,r);
			vector_gpu_dot(r,r,rtr);					
			#pragma endfuse
			if(DEBUG)
			{
				printf("rtr %f\n",rtr);
				vector_print2(p,"p: ",10);	
			}
			for(int i = 0; i < itr; i++)
			{
				TYPE ptap; 
				TYPE rtr2;
				#pragma startfuse
				denseMult(tmp,A,p);	
				vector_gpu_dot(p,tmp,ptap);
				#pragma endfuse
				if(DEBUG)
				{
					printf("ptAp %f\n",ptap);
					vector_print2(tmp,"tmp2: ",10);	
				}		
				TYPE alpha = rtr/ptap;
		
				#pragma startfuse
				vector_gpu_add(x,1,x,-alpha,p);	
				vector_gpu_add(r,1,r,-alpha,tmp);
				vector_gpu_dot(r,r,rtr2);
				#pragma endfuse
		
				TYPE beta = rtr2/rtr;
				rtr = rtr2;
				vector_gpu_add(p,1,r,beta,p);
		
			}
		double innerEnd = gettime();
		vector_transfer(x,0);
		clFinish(queue);
	double outerEnd = gettime();
	printf("%f\t%f\t",outerEnd - outerStart, innerEnd - innerStart);	
	
	if(CHECK)
	{
		printf("\n");
		for(int i = 0; i < size; i++)
		{
			if(x->cpu_vals[i] != xcomp->cpu_vals[i])
				printf("%d: %f != %f\n",i,xcomp->cpu_vals[i], x->cpu_vals[i]);
		}
	}	
	deleteVector(r);
	deleteVector(p);
	deleteVector(tmp);
	deleteVector(b);
	deleteMatrix(A);
	deleteVector(x);				
}

int solve(int size, int itr)
{
	if(DEBUG) perror("Solve");
	Matrix * A = newMatrix(size,size);
	Vector * r = newVector(size,LSIZE);
	Vector * b = newVector(size,LSIZE);
	Vector * p = newVector(size,LSIZE);
	Vector * tmp = newVector(size,LSIZE);
	Vector * x = newVector(size,LSIZE);
	TYPE rtr;
	
	for(int i = 0; i < size; i++)
	{
		r->cpu_vals[i] = 0;
		b->cpu_vals[i] = 0;
		p->cpu_vals[i] = 0;
		x->cpu_vals[i] = 0;
		tmp->cpu_vals[i]=0;
		for(int j = 0; j < size; j++)
		{
			A->cpu_vals[i+j*size] = i*j;
			b->cpu_vals[i] += 10*j*i;
		}
	}
	
	double outerStart = gettime();
	vector_transfer(x,1);
	vector_transfer(b,1);
	vector_transfer(tmp,1);
	vector_transfer(r,1);
	vector_transfer(p,1);	
	matrix_transfer(A,1);
		clFinish(queue);
		double innerStart = gettime();
		
			denseMult(tmp,A,x);	
			vector_gpu_add(r,1,b,-1,tmp);
			vector_gpu_dot(r,r,rtr);
			if(DEBUG)
				vector_print2(r,"r: ",10);
			vector_gpu_add(p,1,r,0,r);
			if(DEBUG)
			{
				printf("rtr %f\n",rtr);
				vector_print2(p,"p: ",10);	
			}			
			for(int i = 0; i < itr; i++)
			{
				TYPE ptap; 
				TYPE rtr2;
		
				denseMult(tmp,A,p);	
				vector_gpu_dot(p,tmp,ptap);
				if(DEBUG)
				{
					printf("ptAp %f\n",ptap);
					vector_print2(tmp,"tmp2: ",10);	
				}				
				TYPE alpha = rtr/ptap;

				vector_gpu_add(x,1,x,-alpha,p);	
				vector_gpu_add(r,1,r,-alpha,tmp);
				vector_gpu_dot(r,r,rtr2);

				TYPE beta = rtr2/rtr;
				rtr = rtr2;
				vector_gpu_add(p,1,r,beta,p);
			}
		double innerEnd = gettime();
		vector_transfer(x,0);
		clFinish(queue);
	double outerEnd = gettime();
	printf("%f\t%f\t",outerEnd - outerStart, innerEnd - innerStart);
		
	deleteVector(r);
	deleteVector(p);
	deleteVector(tmp);
	deleteVector(b);
	deleteMatrix(A);
	solvefused(size,itr,x);	
	deleteVector(x);
}

int main(int argc, char** argv)
{
	int iterations;
	int platform = atoi(argv[1]);
	int device = atoi(argv[2]);
	int size = atoi(argv[3]);
	int tests = atoi(argv[4]);

	//initialize OpenCL
	init(platform, device);

	for(int i = 0; i < tests; i++)
	{
		if(triangle(size)) perror("traingle: Low Dependency Test Failed");
	//	if(distance(size)) perror("distance: Medium Dependency test Failed");
	//	if(cgStart(size)) perror("cgStart: High Dependency test Failed");
		//solve(size, 2);	
		printf("\n");
	}
	printf("program functioned Correctly");
	return 0;
}
