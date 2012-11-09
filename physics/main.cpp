#include "physics.h"
#define DEBUG 1
#include "sys/time.h"
#include "type.h"
TYPE prev;
TYPE now;

double gettime()
{
	struct timeval start;
	gettimeofday(&start,NULL);
	return (double)start.tv_sec + start.tv_usec/1000000.0;
}

void runstep(Game * g)
{
	TYPE timestep = 0.0001;
	/*
	#pragma startfuse	
	collideBalls(g,timestep);	
	collidePockets(g);
	collideWalls(g);
	moveBalls(g,timestep);	
	#pragma endfuse	
	*/
}

void Idle()
{
	if(not stopped)
	{
		clFinish(queue);
		runstep(&g);	
	}
	//getc(stdin);
}



int main(int argc, char ** argv)
{
	prev = gettime();
	now = gettime();
	if(DEBUG) perror("Starting Up!");
	InitGL(&argc, argv);
	init(atoi(argv[1]),atoi(argv[2]));
	glutIdleFunc(Idle);
	testfunc = runstep;
	//run();


	g.cue.y = .5;
	//impulse(&g);
	TYPE timestep = 0.0001;
	for(int i = 0; i < 1000; i++)
	{
		{
			clFinish(queue);
			TYPE start = gettime();
				collideBalls(&g,timestep);	
				collidePockets(&g);
				collideWalls(&g);
				moveBalls(&g,timestep);	
			clFinish(queue);
			TYPE end = gettime();	
			printf("%.12f\t",end-start);
		}
		{
			clFinish(queue);
			TYPE start = gettime();	
			#pragma startfuse	
				collideBalls(&g,timestep);	
				collidePockets(&g);
				collideWalls(&g);
				moveBalls(&g,timestep);		
			#pragma endfuse	
			clFinish(queue);			
			TYPE end = gettime();	
			printf("%.12f",end-start);	
		}
		printf("\n");
	}

}
