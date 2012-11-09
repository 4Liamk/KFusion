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
int LSIZE = 16;
#define DEBUG 0
#define CHECK 0

double gettime()
{
	struct timeval start;
	gettimeofday(&start,NULL);
	return (double)start.tv_sec + start.tv_usec/1000000.0;
}

int trianglefused(int size)
{
	if(DEBUG) perror("Triangle");
	Vector * a = newVector(size,16);
	Vector * b = newVector(size,16);
	Vector * c = newVector(size,16);
	for(int i = 0; i < size; i++)
	{
		a->cpu_vals[i] = i;
		b->cpu_vals[i] = i;
	}
	double outerStart = gettime();
	vector_transfer(a,1);
	vector_transfer(b,1);
		clFinish(queue);
		double innerStart = gettime();
		#pragma startfuse
		vector_gpu_mult(a,1,a,1,a);
		vector_gpu_mult(b,1,b,1,b);
		vector_gpu_add (c,1,a,1,b);
		vector_gpu_sqrt(c,c);
		#pragma endfuse	
		clFinish(queue);	
		double innerEnd = gettime();
		vector_transfer(c,0);
		clFinish(queue);
	double outerEnd = gettime();
	printf("%f\t%f\t",outerEnd - outerStart, innerEnd - innerStart);
	if(CHECK)
	{
		printf("\n");
		for(int i = 0; i < size; i++)
		{
			TYPE result = sqrt(i*i + i*i);
			if(result != c->cpu_vals[i])
			{
				printf("failed on %d: %f != %f\n",i,result, c->cpu_vals[i]);
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
	Vector * a = newVector(size,16);
	vector * b = newVector(size,16);
	vector * c = newVector(size,16);
	for(int i = 0; i < size; i++)
	{
		a->cpu_vals[i] = i;
		b->cpu_vals[i] = i;
	}
	double outerStart = gettime();
	vector_transfer(a,1);
	vector_transfer(b,1);
		clFinish(queue);
		double innerStart = gettime();
		vector_gpu_mult(a,1,a,1,a);
		vector_gpu_mult(b,1,b,1,b);
		vector_gpu_add (c,1,a,1,b);
		vector_gpu_sqrt(c,c);	
		clFinish(queue);	
		double innerEnd = gettime();
		vector_transfer(c,0);
		clFinish(queue);
	double outerEnd = gettime();
	printf("%f\t%f\t",outerEnd - outerStart, innerEnd - innerStart);

	
	if(CHECK)
	{
		printf("\n");
		for(int i = 0; i < size; i++)
		{
			TYPE result = sqrt(i*i + i*i);
			if(result != c->cpu_vals[i])
			{
				printf("failed on %d/%d: %f %f %f != %f\n",i,size,a->cpu_vals[i],b->cpu_vals[i],result, c->cpu_vals[i]);
			}
		}
	}
	deleteVector(a);
	deleteVector(b);
	deleteVector(c);
	trianglefused(size);	
	return 0;
}


int distance2(int size)
{
	if(DEBUG) perror("Distance Fused");
	Vector * x1 = newVector(size,16);
	vector * x2 = newVector(size,16);
	Vector * y1 = newVector(size,16);
	vector * y2 = newVector(size,16);
	vector * c = newVector(size,16);
	for(int i = 0; i < size; i++)
	{
		x1->cpu_vals[i] = i;
		x2->cpu_vals[i] = i*2;
		y1->cpu_vals[i] = i;
		y2->cpu_vals[i] = i*2;		
	}	
	double outerStart = gettime();
	vector_transfer(x1,1);
	vector_transfer(x2,1);
	vector_transfer(y1,1);
	vector_transfer(y2,1);	
		clFinish(queue);
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
			}
		}
	}	
	deleteVector(c);
	deleteVector(x1);
	deleteVector(x2);
	deleteVector(y1);
	deleteVector(y2);		
}

int distance(int size)
{
	if(DEBUG) perror("Distance");
	Vector * x1 = newVector(size,16);
	vector * x2 = newVector(size,16);
	Vector * y1 = newVector(size,16);
	vector * y2 = newVector(size,16);
	vector * c = newVector(size,16);
	for(int i = 0; i < size; i++)
	{
		x1->cpu_vals[i] = i;
		x2->cpu_vals[i] = i*2;
		y1->cpu_vals[i] = i;
		y2->cpu_vals[i] = i*2;		
	}	
	double outerStart = gettime();
	vector_transfer(x1,1);
	vector_transfer(x2,1);
	vector_transfer(y1,1);
	vector_transfer(y2,1);	
		clFinish(queue);
		double innerStart = gettime();	
		vector_gpu_add(x1,1,x1,-1,x2);
		vector_gpu_add(y1,1,y1,-1,y2);
		vector_gpu_mult(x1,1,x1,1,x1);
		vector_gpu_mult(y1,1,y1,1,y1);	
		vector_gpu_add(c,1,x1,1,y1);	
		vector_gpu_sqrt(c,c);
		clFinish(queue);
		double innerEnd = gettime();			
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
				printf("failed on %d: %f %f %f != %f\n",i,result,x1->cpu_vals[i],y1->cpu_vals[i], c->cpu_vals[i]);
			}
		}
	}
	distance2(size);
	deleteVector(c);
	deleteVector(x1);
	deleteVector(x2);
	deleteVector(y1);
	deleteVector(y2);						
}


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
				printf("%d: %f != %f\n",i,r->cpu_vals[i], result->cpu_vals[i]);
		}
	}	
	deleteVector(x);
	deleteVector(b);
	deleteVector(r);
	deleteVector(b2);
	deleteVector(p);	
	deleteMatrix(A);
}

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
	cgStartFused(size,r);
	deleteVector(r);
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

int other(int size)
{
	Vector * a = newVector(size,LSIZE);
	Vector * b = newVector(size,LSIZE);
	Vector * c = newVector(size,LSIZE);
	Vector * c2 = newVector(size,LSIZE);
	Vector * c3 = newVector(size,LSIZE);
}

int main(int argc, char** argv)
{
	int iterations;
	int platform = atoi(argv[1]);
	int device = atoi(argv[2]);
	int size = atoi(argv[3]);
	int tests = atoi(argv[4]);
	init(platform, device);
	for(int i = 0; i < tests; i++)
	{
		triangle(size);
		distance(size);
		cgStart(size);
		//solve(size, 2);	
		printf("\n");
	}
	return 0;
}
