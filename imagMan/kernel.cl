#define TYPE float
#define AND 0
#define OR 1
#define XOR 2
#define ADD 3
#define SUB 4
#define MUL 5
#define DIV 6
#define AVG 7

float hueToRGB(float p, float q, float t)
{
	if(t < 0) t += 1;
	if(t > 1) t -= 1;
	if(t < 1/6.0) return p + (q - p ) * 6 * t;
	if(t < 1/2.0) return q;
	if(t < 2/3.0) return p + (q - p) * (2/3.0 - t) * 6;
	return p;

}

__kernel void RGBGreyscale(__read_only image2d_t  input, __write_only image2d_t output)
{
	#pragma load
	const sampler_t smp = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE |CLK_FILTER_LINEAR;
		
	#pragma load
	float2 coord = (float2)((float)get_global_id (0),(float)get_global_id (1));
	#pragma load
	coord.x /= get_global_size(0); 
	#pragma load
	coord.y /= get_global_size(1); 
	#pragma load	
	float4 val = read_imagef(input, smp, coord);
	
	val.x = val.x*.29 + val.y*.60 + val.z*.11;
	val.y = val.x;
	val.z = val.x;
	
	#pragma store
	int2 coord2 = (int2) (get_global_id(0), get_global_id(1));
	#pragma store
	write_imagef(output,coord2,val);
}

__kernel void crop(__read_only image2d_t  input, __write_only image2d_t output, const int x1, const int y1)
{
	#pragma load
	const sampler_t smp = CLK_NORMALIZED_COORDS_FALSE | //Natural coordinates
		                 CLK_ADDRESS_NONE | //Clamp to zeros
		                 CLK_FILTER_NEAREST; //Don't interpolate	
	#pragma load	
	int2 coord = (int2)(x1 + get_global_id(0),y1 + get_global_id(1));
	#pragma load
	float4 val = read_imagef(input, smp, coord);
	
	#pragma store
	write_imagef(output,coord,val);
}

__kernel void resize(__read_only image2d_t  input, __write_only image2d_t output)
{
	#pragma load
	const sampler_t smp = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE |CLK_FILTER_LINEAR;
	#pragma load
	float2 coord = (float2)((float)get_global_id (0),(float)get_global_id (1));
	#pragma load
	coord.x /= get_global_size(0); 
	#pragma load
	coord.y /= get_global_size(1);  
	#pragma load
	float4 val = read_imagef(input, smp, coord);
	
	
	#pragma store
	int2 coord2 = (int2) (get_global_id(0), get_global_id(1));
	#pragma store
	write_imagef(output, coord2, val);
}

__kernel void overlay(__read_only image2d_t  input, __read_only image2d_t  input2,__write_only image2d_t output, const int x1, const int x2, const int y1, const int y2, const int op)
{
	#pragma load
	const sampler_t smp = CLK_NORMALIZED_COORDS_FALSE | //Natural coordinates
		                 CLK_ADDRESS_NONE | //Clamp to zeros
		                 CLK_FILTER_NEAREST; //Don't interpolate	
	#pragma load
	int2 coord = (int2)(get_global_id(0), get_global_id(1));
	#pragma load
	float4 val = read_imagef(input, smp, coord);
	float4 tmp;
	tmp = val;
	if(get_global_id(0) >= x1 && get_global_id(0) < x1 + x2 && get_global_id(1) >= y1 && get_global_id(1) < y1+y2) 
	{
		int2 coord2 = (int2)(get_global_id(0) - x1, get_global_id(1) - y1);
		float4 val2 = read_imagef(input2, smp, coord2);
		/*
		if(op == AND) 		tmp = val & val2;
		else if(op == OR) 	tmp = val | val2;
		else if(op == XOR) 	tmp = val ^ val2;
		else if(op == ADD) 	tmp = val + val2;
		else if(op == SUB) 	tmp = val - val2;
		else if(op == MUL) 	tmp = val * val2;
		else if(op == DIV) 	tmp = val + val2;
		*/
		tmp = val;
		tmp.x = (val.x*.9 + val2.x*.1);
		tmp.y = (val.y*.9 + val2.y*.1);
		//tmp.x = (val.x*.1 + val2.x*.9);
		//tmp.y = (val.y*.1 + val2.y*.9);		
		tmp.z = (val.z*.7 + val2.z*.3);
		tmp.w = fmin(val.w,val2.w);
	}
	write_imagef(output,coord,tmp);
}

__kernel void binOp(__read_only image2d_t  input, __read_only image2d_t  input2,__write_only image2d_t output, const int op)
{
	#pragma load
	const sampler_t smp = CLK_NORMALIZED_COORDS_TRUE | //Natural coordinates
		                 CLK_ADDRESS_CLAMP_TO_EDGE | //Clamp to zeros
		                 CLK_FILTER_LINEAR;	
	#pragma load
	float2 coord = (float2)((float)get_global_id (0),(float)get_global_id (1));
	#pragma load
	coord.x /= get_global_size(0); 
	#pragma load
	coord.y /= get_global_size(1); 
	#pragma load
	float4 val = read_imagef(input, smp, coord);
	#pragma load
	float4 val2 = read_imagef(input2, smp, coord);
	/*
	if(op == AND) tmp = val & val2;
	else if(op == OR) tmp = val | val2;
	else if(op == XOR) tmp = val ^ val2;
	else if(op == ADD) tmp = val + val2;
	else if(op == SUB) tmp = val - val2;
	else if(op == MUL) tmp = val * val2;
	else if(op == DIV) tmp = val / val2;
	else if(op == AVG) 
	*/
	if(op == SUB) 
		val = fabs(val - val2);
	else if(op == AVG) 
		val = (val + val2)/2;
	val.w = 255;
	
	#pragma store
	int2 coord2= (int2)(get_global_id(0),get_global_id(1));
	#pragma store
	write_imagef(output,coord2,val);
}

__kernel void RGBInvert(__read_only image2d_t  input, __write_only image2d_t output)
{
	#pragma load
	const sampler_t smp = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;
	#pragma load
	float2 coord = (float2)((float)get_global_id (0),(float)get_global_id (1));
	#pragma load
	coord.x /= get_global_size(0); 
	#pragma load
	coord.y /= get_global_size(1); 
	
	#pragma load
	float4 val = read_imagef(input, smp, coord);
	
	val.x = 255-val.x;
	val.y = 255-val.y;
	val.z = 255-val.z;
	
	#pragma store
	int2 coord2 = (int2)(get_global_id(0), get_global_id(1));
	#pragma store
	write_imagef(output,coord2,val);
}


float4 HtR(float4 val)
{
	float H = val.x;
	float S = val.y; 
	float L = val.z;
	
	float q = L < 0.5 ? L * (1 + S) : L + S - L * S;
	float p = 2 * L - q; 
	float r = hueToRGB(p,q,H + 1/3.0);
	float g = hueToRGB(p,q,H);
	float b = hueToRGB(p,q,H - 1/3.0);
	
	val.x = r*255;
	val.y = g*255;
	val.z = b*255;	
	return val;
}

float4 RtH(float4 val)
{
	float r = val.x/255.0;
	float g = val.y/255.0;
	float b = val.z/255.0;
	float maxval = fmax(r,fmax(g,b));
	float minval = fmin(r,fmin(g,b));
	float h,s,l;
	l = (maxval + minval)/2;
	if(maxval == minval) h = s = 0; 
	else 
	{
		float d = maxval - minval;
		s = l > .5 ? d / (2 - 2*l) : d / (2*l);
		if(maxval == r) h = (g - b) / d + (g < b ? 6 : 0);
		else if(maxval == g) h = (b - r) / d + 2;
		else if(maxval == b) h = (r - g) / d + 4;
		h /= 6;
	}
	val.x = h;
	val.y = s;
	val.z = l;
	return val;
}

__kernel void HSVtoRGB(__read_only image2d_t  input, __write_only image2d_t output)
{
	#pragma load
	const sampler_t smp = CLK_NORMALIZED_COORDS_TRUE | //Natural coordinates
		                 CLK_ADDRESS_CLAMP_TO_EDGE  | //Clamp to zeros
		                 CLK_FILTER_LINEAR; //Don't interpol	
	#pragma load
	float2 coord = (float2)((float)get_global_id (0),(float)get_global_id (1));
	#pragma load
	coord.x /= get_global_size(0); 
	#pragma load
	coord.y /= get_global_size(1); 
	
	#pragma load
	float4 val = read_imagef(input, smp, coord);
	
	
	val = HtR(val);

	#pragma store
	int2 coord2 = (int2)(get_global_id(0), get_global_id(1));	
	#pragma store
	write_imagef(output,coord2,val);
}


__kernel void RGBtoHSV(__read_only image2d_t  input, __write_only image2d_t output)
{
	#pragma load
	const sampler_t smp = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE |CLK_FILTER_LINEAR;
	#pragma load
	float2 coord = (float2)((float)get_global_id (0),(float)get_global_id (1));
	#pragma load
	coord.x /= get_global_size(0); 
	#pragma load
	coord.y /= get_global_size(1); 
	#pragma load
	float4 val = read_imagef(input, smp, coord);
	
	
	val = RtH(val);
	
	#pragma store
	int2 coord2 = (int2)(get_global_id(0), get_global_id(1));
	#pragma store
	write_imagef(output,coord2,val);
}

__kernel void convolution(__read_only image2d_t  input, __write_only image2d_t output, __global TYPE * matrix, const int dim, const float div)
{
	#pragma load 
	__local float copy[32]; 
	#pragma load
	event_t copyevent = async_work_group_copy(copy,matrix,(2*dim+1)*(2*dim+1),0);
	#pragma load
	const sampler_t smp = CLK_NORMALIZED_COORDS_TRUE | //Natural coordinates
		                 CLK_ADDRESS_CLAMP_TO_EDGE  | //Clamp to zeros
		                 CLK_FILTER_LINEAR; //Don't interpol
	#pragma load	        
	float4 tmp;
	
	tmp.x = 0;tmp.y = 0;tmp.z = 0;tmp.w = 0;
	wait_group_events(1,&copyevent);
	for(int i = -dim; i < dim+1; i++)
	{
		for( int j = -dim; j < dim+1; j++)
		{
			float2 coord = (float2)((float)(get_global_id(0) + i)/get_global_size(0), (float)(get_global_id(1) + j)/get_global_size(1));
			float4 val = read_imagef(input, smp, coord);
			tmp += copy[(i + dim) * ( 2 * dim + 1) + (j + dim)]*val;
		}
	}
	tmp /= div;
	#pragma store
	int2 coord2 = (int2)(get_global_id(0), get_global_id(1));
	#pragma store
	write_imagef(output,coord2,tmp);
}
__kernel void colorize(__read_only image2d_t  input, __write_only image2d_t output, const float color)
{
	#pragma load
	const sampler_t smp = CLK_NORMALIZED_COORDS_TRUE |
		                 CLK_ADDRESS_CLAMP_TO_EDGE |
		                 CLK_FILTER_LINEAR; 
	#pragma load
	float2 coord = (float2)((float)get_global_id (0),(float)get_global_id (1));
	#pragma load
	coord.x /= get_global_size(0); 
	#pragma load
	coord.y /= get_global_size(1); 
	#pragma load
	float4 val = read_imagef(input, smp, coord);
	
	val.x = color;
	
	#pragma store
	int2 coord2 = (int2)(get_global_id(0), get_global_id(1));	
	#pragma store
	write_imagef(output, coord2, val);
}



__kernel void bestCase(__read_only image2d_t  input, __write_only image2d_t output, const float color)
{

	#pragma load
	const sampler_t smp = CLK_NORMALIZED_COORDS_TRUE |
		                 CLK_ADDRESS_CLAMP_TO_EDGE |
		                 CLK_FILTER_LINEAR; 
	#pragma load
	float2 coord = (float2)((float)get_global_id (0),(float)get_global_id (1));
	#pragma load
	coord.x /= get_global_size(0); 
	#pragma load
	coord.y /= get_global_size(1); 
	#pragma load
	float4 val = read_imagef(input, smp, coord);

	
	val = RtH(val);
	val.x = color;
	val = HtR(val);
	
	val.x = 255-val.x;
	val.y = 255-val.y;
	val.z = 255-val.z;
	
	int2 coord2 = (int2) (get_global_id(0), get_global_id(1));
	write_imagef(output, coord2, val);		                	
	
}

__kernel void complicated(__read_only image2d_t  input, __read_only image2d_t  input2, __write_only image2d_t output, __global TYPE * blur, const int dim, const float div, __global TYPE * sharpen, const float div2)
{
	__local float blurcopy[32]; 
	__local float sharpencopy[32]; 	
	event_t copyevent = async_work_group_copy(blurcopy,blur, (2*dim+1) * (2*dim+1), 0);
	event_t copyevent2 = async_work_group_copy(sharpencopy,sharpen, (2*dim+1) * (2*dim+1), 0);
	const sampler_t smp = CLK_NORMALIZED_COORDS_TRUE 
				| CLK_ADDRESS_CLAMP_TO_EDGE 
				| CLK_FILTER_LINEAR; 
	
	//image 1 manipluations
	float4 tmp = {0,0,0,0};
	float4 tmp2 = {0,0,0,0};
	
	//blur convolution to image1
	wait_group_events(1,&copyevent);
	for(int i = -dim; i <= dim; i++)
	{
		for( int j = -dim; j <= dim; j++)
		{
			float2 coord = (float2)((float)(get_global_id(0) + i)/get_global_size(0), (float)(get_global_id(1) + j)/get_global_size(1));
			float4 val = read_imagef(input, smp, coord);
			tmp += blurcopy[(i + dim) * ( 2 * dim + 1) + (j + dim)]*val;
		}
	}
	tmp /= div;
	//sharpen convolution to image2
	wait_group_events(1,&copyevent2);
	for(int i = -dim; i <= dim; i++)
	{
		for( int j = -dim; j <= dim; j++)
		{
			float2 coord = (float2)((float)(get_global_id(0) + i)/get_global_size(0), (float)(get_global_id(1) + j)/get_global_size(1));
			float4 val = read_imagef(input2, smp, coord);
			tmp2 += sharpencopy[(i + dim) * ( 2 * dim + 1) + (j + dim)]*val;
		}
	}
	tmp2 /= div2;
	tmp2.x = tmp2.x*.29 + tmp2.y*.60 + tmp2.z*.11;
	tmp2.y = tmp2.x;
	tmp2.z = tmp2.x;

	tmp.x = tmp.x*.29 + tmp.y*.60 + tmp.z*.11;
	tmp.y = tmp.x;
	tmp.z = tmp.x;	
			
	tmp = fabs(tmp - tmp2);
	tmp.w = 255;
	int2 coord2 = (int2) (get_global_id(0), get_global_id(1));
	write_imagef(output, coord2, tmp);		                	
}

__kernel void avgCase(__read_only image2d_t  input,__read_only image2d_t  input2, __write_only image2d_t output, __global TYPE * matrix, const int dim, const float div)
{
	__local float copy[32]; 
	event_t copyevent = async_work_group_copy(copy,matrix, (2*dim+1) * (2*dim+1) ,0);
	
	const sampler_t smp = CLK_NORMALIZED_COORDS_TRUE | //Natural coordinates
		                CLK_ADDRESS_CLAMP_TO_EDGE | //Clamp to edge
		                 CLK_FILTER_LINEAR;   
		                   
	float4 tmp = {0,0,0,0};
	float2 coord = (float2)((float)get_global_id(0)/get_global_size(0), (float) get_global_id(1)/get_global_size(1));
	float4 tmp2 = read_imagef(input2, smp, coord);
	tmp2 = RtH(tmp2);
		
	//resize happens automatically using relative coordination
	wait_group_events(1,&copyevent);
	
	//blur convolution
	for(int i = -dim; i <= dim; i++)
	{
		for( int j = -dim; j <= dim; j++)
		{
			float2 coord = (float2)((float)(get_global_id(0) + i)/get_global_size(0), (float)(get_global_id(1) + j)/get_global_size(1));
			float4 val = read_imagef(input, smp, coord);
			tmp += copy[(i + dim) * ( 2 * dim + 1) + (j + dim)]*val;
		}
	}
	tmp /= div;

	//convert both to hsv
	tmp = RtH(tmp);	
	//overlay image
	tmp = fabs(tmp - tmp2);
	tmp.w = 255;
	//convert to rgb
	tmp = HtR(tmp);	
	int2 coord2 = (int2)(get_global_id(0), get_global_id(1));
	write_imagef(output,coord2,tmp);
}

__kernel void worstCase(__read_only image2d_t  input,__read_only image2d_t  input2, __write_only image2d_t output,__global TYPE * matrix,__global TYPE * matrix2, const int dim, const float div)
{
	__local float copy[32]; 
	__local float copy2[32];
	event_t copyevent = async_work_group_copy(copy,matrix, (2*dim+1) * (2*dim+1) ,0);
	event_t copyevent2 = async_work_group_copy(copy2,matrix2, (2*dim+1) * (2*dim+1) ,0);
	const sampler_t smp = CLK_NORMALIZED_COORDS_TRUE | //Natural coordinates
	                CLK_ADDRESS_CLAMP_TO_EDGE | //Clamp to edge
	                 CLK_FILTER_LINEAR;   
	float4 tmp = 0;
	float4 tmp2 = 0;
	wait_group_events(1,&copyevent);	
	for(int i = -dim; i <= dim; i++)
	{
		for( int j = -dim; j <= dim; j++)
		{
			float2 coord = (float2)((float)(get_global_id(0) + i)/get_global_size(0), (float)(get_global_id(1) + j)/get_global_size(1));
			float4 val = read_imagef(input, smp, coord);
			tmp += copy[(i + dim) * ( 2 * dim + 1) + (j + dim)]*val;
		}
	}	 	
	wait_group_events(1,&copyevent2);
	for(int i = -dim; i <= dim; i++)
	{
		for( int j = -dim; j <= dim; j++)
		{
			float2 coord = (float2)((float)(get_global_id(0) + i)/get_global_size(0), (float)(get_global_id(1) + j)/get_global_size(1));
			float4 val = read_imagef(input2, smp, coord);
			tmp2 += copy2[(i + dim) * ( 2 * dim + 1) + (j + dim)]*val;
		}
	}
	tmp /=4;
	tmp2 /=4;
	tmp = fabs(tmp - tmp2);
	tmp.w = 255;
	int2 coord2 = (int2)(get_global_id(0), get_global_id(1));
	write_imagef(output,coord2,tmp);
}
