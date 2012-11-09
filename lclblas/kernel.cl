#define TYPE double
#define TYPE4 double4
#define TYPE2 double
#define VLENGTH 4
#define LOCK __local int
#pragma OPENCL EXTENSION cl_khr_fp64: enable
#define MAX_LOCAL 256
void reduce(__local TYPE * comp)
{
	for(unsigned int i= get_local_size(0)/2 ; i > 0; i /= 2)
	{
		barrier(CLK_LOCAL_MEM_FENCE);
		if(get_local_id(0) < i)	
			comp[get_local_id(0)] += comp[get_local_id(0)+i];
		
	}
}

void reduce2(__local TYPE * comp, int len)
{
	for(unsigned int i= len/2 ; i > 0; i /= 2)
	{
		barrier(CLK_LOCAL_MEM_FENCE);
		if(get_local_id(0) < i)	
			comp[get_local_id(0)] += comp[get_local_id(0)+i];
		
	}
}

__kernel void sum(__global TYPE * input, __global TYPE * result, const int len)
{
	__local TYPE comp[MAX_LOCAL];
	comp[get_local_id(0)] = (get_local_id(0) < len) ? input[get_local_id(0)] : 0;
	reduce(comp);
	if(get_local_id(0) == 0) result[0] = comp[0];
}

__kernel void vmult(__global TYPE * c, const TYPE alpha, __global TYPE * a, const TYPE beta, __global TYPE * b)
{
	#pragma load
	int gid = get_global_id(0);
	
	#pragma load
	TYPE aVal = a[gid];
	
	#pragma load
	TYPE bVal = b[gid];
	#pragma load
	TYPE cVal;
	
	cVal = alpha*aVal*beta*bVal;
	
	#pragma store
	c[gid] = cVal;
}

__kernel void vsqrt(__global TYPE * c, __global TYPE * a)
{
	#pragma load
	int gid = get_global_id(0);
	
	#pragma load
	TYPE aVal = a[gid];
	
	#pragma load
	TYPE cVal;
	
	cVal = sqrt(aVal);
	
	#pragma store
	c[gid] = cVal;
}

__kernel void add(__global TYPE * c, const TYPE alpha, __global TYPE * a, const TYPE beta, __global TYPE * b)
{
	#pragma load
	int gid = get_global_id(0);
	
	#pragma load
	TYPE aVal = a[gid];

	#pragma load
	TYPE bVal = b[gid];

	#pragma load
	TYPE cVal;

	cVal = alpha*aVal + beta*bVal;
	
	#pragma store
	c[gid] = cVal;

}

__kernel void denseMult(__global TYPE * b, __global TYPE * A, __global TYPE * x, const int cols)
{
	#pragma load
	__local TYPE val;
		
	#pragma load
	int gid = get_global_id(0);

	int global_size = get_global_size(0);

	#pragma load
	TYPE sum = x[0]*A[gid];
	
	for(int i = 1; i < cols; i++)
	{
		val = x[i];
		sum += val*A[i*global_size + gid];
	}
	
	#pragma store
	b[gid] = sum;
}
__kernel void dotproduct(__global TYPE * A, __global TYPE * B, __global TYPE * results)
{
	#pragma load
	__local TYPE comp[MAX_LOCAL];
	#pragma load
	int gid = get_global_id(0);
	#pragma load
	int lid = get_local_id(0);
	#pragma load
	int god = get_group_id(0);
	#pragma load
	TYPE a = A[gid];
	#pragma load
	TYPE b = B[gid];
	#pragma load
	comp[lid] = a*b;
	
	reduce(comp);
	barrier(CLK_LOCAL_MEM_FENCE);
	
	#pragma store
	if(lid == 0) results[god] = comp[0];
}

__kernel void mult(__global int * cols, __global TYPE * vals,__global int * index, __global TYPE * x, __global TYPE * b)
{
	__local int local_index[MAX_LOCAL+1];
	
	event_t copy = async_work_group_copy(local_index, &index[get_group_id(0)*get_local_size(0)], get_local_size(0)+1, 0);
	
	int gid = get_global_id(0);
	int lid = get_local_id(0);
		
	TYPE accum = 0;
	wait_group_events(1,&copy);

	for(int i = local_index[lid]; i < local_index[lid+1]; i++)
	{
		accum += vals[i]*x[cols[i]];
	}
	b[gid] = accum;
}


