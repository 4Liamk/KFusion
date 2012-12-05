#include "physics.h"
#define DEBUG 1
#include "sys/time.h"
#include "type.h"
TYPE prev;
TYPE now;

/**
 * OpenCL Pool: This application accompishes a fairly simple pool implementation
 * 
*/


//returns the current time as a double
double gettime()
{
	struct timeval start;
	gettimeofday(&start,NULL);
	return (double)start.tv_sec + start.tv_usec/1000000.0;
}

//runs one step of the simulation.  Uncomment if you would like to fuse it.  Kfusion does not allow you to repeat yourself: only one unqiue combination of functions may be fused once.
void runstep(Game * g)
{
	TYPE timestep = 0.0001;
	
	//#pragma startfuse	
	collideBalls(g,timestep);	
	collidePockets(g);
	collideWalls(g);
	moveBalls(g,timestep);	
	//#pragma endfuse
}

//run the game when the OpenGL context is not rendering:
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
	//this does not require any arguments
	prev = gettime();
	now = gettime();
	if(DEBUG) perror("Starting Up!");
	InitGL(&argc, argv);
	init(atoi(argv[1]),atoi(argv[2]));
	glutIdleFunc(Idle);
	testfunc = runstep;
	
	//if you would like to play the game proper, uncommment this:
	//run();

	//actual test code to obtain results:
	//If you want to play the game, comment out the following code with some /* */
	g.cue.y = .5;
	//impulse(&g);
	TYPE timestep = 0.0001;
	for(int i = 0; i < 1000; i++)
	{
		{
			//basic excecution unfused.  Inscoped to allow for similarly named variables as second case
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
			//second case: This one will be fused.
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
