#define TYPE double
#define TYPE4 double4
#define TYPE2 double
#define VLENGTH 4
#define LOCK __local int
#define MAX_LOCAL 256

#pragma OPENCL EXTENSION cl_khr_fp64: enable

TYPE dot3(TYPE4 one, TYPE4 two)
{
	return one.x*two.x + one.y*two.y + one.z*two.z;
}

TYPE4 solveCollisionPos(TYPE4 p1, TYPE4 v1, TYPE4 p2, TYPE4 v2, TYPE d, TYPE timestep)
{
	TYPE h = -100*timestep;
	TYPE l = 0;
	TYPE err = 1;
	while(fabs(h-l) > .00000001)
	{
		TYPE mid = (h+l)/2;
		TYPE4 d2 = (p1 + v1*mid) - (p2 + v2*mid);
		d2 *= d2;
		TYPE cd = sqrt(d2.x + d2.y + d2.z);
		cd = cd - d;
		if(cd < 0)
			l = mid;
		else
			h = mid;
	}
	return (p1 + v1*h);
}

//collide anything which needs to be collided
__kernel void ballCollision(__global TYPE4 * pos,__global  TYPE4 * vel,__global  TYPE4 * stats, const char balls, const TYPE timestep)
{
	#pragma load
	__local TYPE4 comppos[16];
	#pragma load
	__local TYPE4 compvel[16];
	#pragma load
	__local TYPE4 compstats[16];

	#pragma load
	event_t events[3];

	#pragma load	
	events[0] = async_work_group_copy(comppos,pos,balls,0);
	#pragma load	
	events[1] = async_work_group_copy(compvel,vel,balls,0);
	#pragma load	
	events[2] = async_work_group_copy(compstats,stats,balls,0);
	
	#pragma load
	int gid = get_local_id(0);

	#pragma load	
	TYPE4 mypos = pos[gid];
	#pragma load	
	TYPE4 myvel = vel[gid];
	#pragma load
	TYPE4 mystats = stats[gid];		
	if(mypos.w==1)
	{
		//load data from memory for this ball, this will stay in memory during execution
		//ball collision
		wait_group_events(3,events);
		for(int i = 0; i < balls; i++)
		{
		
			if(i != gid) //do not collide with self
			{
				//if distance is less than combined radius, this is a collision! Woot Woo!
				TYPE4 dv = (mypos - comppos[i]);
				TYPE4 dv2 = dv*dv;
				TYPE d = sqrt(dv2.x + dv2.y + dv2.z);
				if(d < mystats.y + compstats[i].y)
				{
					//we have a collision
					dv = mypos - comppos[i];
				
					//calculate collision normal
					TYPE4 cNorm = dv/d;
				
					//calculate normal components
					TYPE4 Vn1 = dot3(myvel,-cNorm)*-cNorm;
					TYPE4 Vn2 = dot3(compvel[i],cNorm)*cNorm;
	
					//calculate tangental
					TYPE4 vt1 = Vn1 - myvel;
					TYPE4 vt2 = Vn2 - compvel[i];
				
					//momentum is conserved!  Bon temps, only deal with you[i]
					mypos = solveCollisionPos(mypos,myvel,comppos[i],compvel[i],mystats.y + compstats[i].y,timestep);	
					myvel = (vt1 + Vn2)*mystats.z + myvel*(1-mystats.z);
				}
			}
		}
		mypos.w = 1;
	}
	else
	{
		myvel = 0;
	}	
	#pragma store
	vel[gid] = myvel;
	#pragma store
	pos[gid] = mypos;
}

__kernel void pocketCollisions(__global TYPE4 * pos,__global TYPE4 * ps, const char pockets)
{
	#pragma load
	int gid = get_local_id(0);	
	#pragma load
	TYPE4 mypos = pos[gid];	
	#pragma load	
	__local TYPE4 lps[6];
	#pragma load	
	event_t e = async_work_group_copy(lps,ps,pockets,0);		
	if(mypos.w==1)
	{
		wait_group_events(1,&e);
		for(int i = 0; i < pockets; i++)
		{
			TYPE4 dv2 = (mypos - lps[i])*(mypos - lps[i]);
			TYPE d = sqrt(dv2.x + dv2.y + dv2.z);
			if(d < lps[i].w)
			{
				mypos = lps[i];
				mypos.w = 0;
			}
		}
		
	}
	#pragma store
	pos[gid] = mypos;
}

__kernel void tableCollision(__global TYPE4 * pos,__global  TYPE4 * vel,__global  TYPE4 * stats,__global  TYPE4 * table)
{
	#pragma load
	int gid = get_local_id(0);	
	#pragma load
	TYPE4 mypos = pos[gid];
	#pragma load
	TYPE4 mystats = stats[gid];
	#pragma load
	TYPE4 myvel = vel[gid];	
	if(mypos.w==1)
	{
		if(mypos.y - mystats.y <= 0 || mypos.y + mystats.y >= table[0].y)
		{
			myvel *= mystats.z;
			myvel.y *= -1;
			if(mypos.y - mystats.y < 0)
				mypos.y = 0+(1.01*mystats.y);
			else if(mypos.y + mystats.y > table[0].y)
				mypos.y = table[0].y-(1.01*mystats.y);
		}
		else if(mypos.x - mystats.y <= 0 || mypos.x + mystats.y >= table[0].x)
		{
			myvel *= mystats.z;
			myvel.x *= -1;
			if(mypos.x - mystats.y < 0)
				mypos.x = 0 + (1.01*mystats.y);
			else if(mypos.x + mystats.y > table[0].x)
				mypos.x = table[0].x - (1.01*mystats.y);			
		}
		mypos.w = 1;
	}
	else
	{
		myvel = 0;
	}
	#pragma store
	vel[gid] = myvel;
	#pragma store
	pos[gid] = mypos;
}

__kernel void moveballs(__global TYPE4 * pos,__global  TYPE4 * vel,__global  TYPE4 * stats,__global  TYPE4 * table, const TYPE timestep)
{
	#pragma load
	int gid = get_local_id(0);	
	#pragma load
	TYPE4 mypos = pos[gid];
	#pragma load
	TYPE4 mystats = stats[gid];
	#pragma load
	TYPE4 myvel = vel[gid];
	if(mypos.w==1)
	{
		TYPE magnitude = sqrt(dot3(myvel,myvel));
		if(magnitude > 0)
		{
			TYPE4 norm = myvel/magnitude;
			magnitude -= (mystats.w*9.81)*timestep;
			if(magnitude < 0)
				magnitude = 0;
			if(magnitude > 10)
				magnitude = 10;
			myvel = norm*magnitude;
			mypos += myvel*timestep;
	
		}
		else
		{
			myvel = 0;
		}
		mypos.w = 1;		
	}
	#pragma store	
	pos[gid] = mypos;
	#pragma store
	vel[gid] = myvel;
	
}

__kernel void impulse(__global TYPE4 * vel, __global TYPE4 * stats,const TYPE angle, const TYPE power)
{
//	#pragma load		
	int gid = get_local_id(0);
//	#pragma load	
	TYPE4 cue = {-cos(angle*(M_PI/180.0)),-sin(angle*(M_PI/180.0)),0,0};
//	#pragma load	
	cue *= power;

	if(gid == 0) 
		vel[gid] += (cue/stats[gid].x);
}
