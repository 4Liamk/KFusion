#ifndef PHYSICS_H
#define PHYSICS_H
#include "type.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <CL/cl.h>
#include <CL/cl_gl.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/glx.h>
#include "reader.h"
#include <math.h>

#define REFRESH_DELAY 30 //ms
#define WIDTH  1408
#define HEIGHT 1024

#define NUMBER_OF_BALLS	16
#define BALL_RADIUS 	0.05715/2
#define BALL_MASS  	0.162
#define BALL_DFRICTION 	0.065
#define BALL_SFRICTION  0.95
#define TABLE_HEIGHT  	2.4384
#define TABLE_WIDTH  	2.4384/2
#define TABLE_FRICTION 	0.9
#define TABLE_POCKET_MULT 2.25

typedef struct d4
{
	double x,y,z,w;
	
}double4;

//balls data type, encompases more or less the entire game.  Everything is a 4 element vector as theis transfers well to OpenCL
typedef struct balls
{
	int num;
	TYPE4 pos[NUMBER_OF_BALLS]; //x,y,z,inPlay
	TYPE4 vel[NUMBER_OF_BALLS]; //vx,vy,vz,???
	TYPE4 stats[NUMBER_OF_BALLS];//mass,radius,static-friction,dynamic-friction
	cl_mem gpu_pos;
	cl_mem gpu_vel;
	cl_mem gpu_stats;
}Balls;

//Table datatype, contains the relevant table information.  
typedef struct table
{
	TYPE4 stats; //width,height,depth,???
	TYPE4 pockets[6]; //x,y,z,radius
	cl_mem gpu_stats;
	cl_mem gpu_pockets;
}Table;

//contains all the game information.
typedef struct game
{
	Balls b;
	Table t;
	TYPE timestep;
	TYPE4 cue; //angle,power,SPIN
}Game;

//General Init Function:
void init(int platform, int device);

//transfer functions
/* move data to and from the ball
 * 
 */
void ball_transfer(Balls * b, int dest);
void table_transfer(Table * t, int dest);

/* transfer momentum to the queue ball, 
 * takes the game state and starts the ball rolling 
 */
void impulse(Game * g);

/* move the balls forward,
 * Game: g, TYPE timestep 
 */
void moveBalls(Game*g,TYPE timestep);

/* Collision functions.  Each is self explanitor in targe
 * It is colliding the 16 balls found in pool which various object
 * collideBalls, collidePockets, CollideWalls
 */
void collideBalls(Game*g,TYPE timestep);

void collidePockets(Game*g);
void collideWalls(Game*g);

//OpenCL Info
//platform informationtep 0.000033855438
extern cl_platform_id	platforms[16];
extern cl_uint 	num_platforms;
extern int param_value_buffer[8];
extern size_t param_value_size_ret;

//device infomation
extern cl_device_type device_type[8];
extern cl_device_id   device_list[8];

//context and queue information
extern cl_context context;
extern cl_command_queue queue;

extern cl_program program;
extern cl_kernel ballCollision_kernel; 	//check for ball collisions, transfer momentum as force
extern cl_kernel pocketCollisions_kernel; //check for pocket collisions
extern cl_kernel tableCollision_kernel;  //check for table collisions
extern cl_kernel moveballs_kernel; 	//move all the balls based on current velocities
extern cl_kernel impulse_kernel; //hit the queue ball

//OpenGL Info/Vars/functions
extern int keys[16];
//initialization
void InitGL(int* argc, char** argv);
//draw callback
void DisplayGL();
//keyboard callback
void KeyboardGL(unsigned char key, int x, int y);
//idle callback: runs the actual simulation in between when it is being drawn
void Idle();
void timerEvent(int value);
extern int stopped;
extern void (*testfunc) (Game*);

void run();
extern Game g;
extern Game sg;
extern int guide;
#endif
