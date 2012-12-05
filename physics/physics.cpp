#include "physics.h"
#include "check.h"

cl_platform_id	platforms[16];
cl_uint 	num_platforms;
int param_value_buffer[8];
size_t param_value_size_ret;

//device infomation
cl_device_type device_type[8];
cl_device_id   device_list[8];

//context and queue information
cl_context context;
cl_command_queue queue;

cl_program program;
cl_kernel ballCollision_kernel; 	//check for ball collisions, transfer momentum as force
cl_kernel pocketCollisions_kernel; //check for pocket collisions
cl_kernel tableCollision_kernel;  //check for table collisions
cl_kernel moveballs_kernel; 	//move all the balls based on current velocities
cl_kernel impulse_kernel; //hit the queue ball
Game sg;
Game g;
void (*testfunc) (Game*);
int stopped =1;
int shadowEnabled = 0;
int window_width = 800;
int window_height = 600;
int iGLUTWindowHandle;
int guide = 0;
#define DEBUG 1
#define SIM_STEPS 2000
#define SIM_STEP .0001
void runTest()
{
	if(sg.cue.x != g.cue.x or sg.cue.y != g.cue.y)
	{
		sg.cue = g.cue;
		for(int i = 0; i < NUMBER_OF_BALLS;i++)
		{
			sg.b.pos[i] = g.b.pos[i];
			sg.b.vel[i] = g.b.vel[i];
			sg.b.stats[i] = g.b.stats[i];		
		}
		sg.t.stats = g.t.stats;
		for(int i = 0; i < 6; i++)
		{
			sg.t.pockets[i] = g.t.pockets[i];	
		}
		ball_transfer(&sg.b,1);
		table_transfer(&sg.t,1);
		impulse(&sg);	
		for(int i = 0; i < 3000; i++)
		{
			testfunc(&sg);	
		}
		ball_transfer(&sg.b,0);	
	}
} 

void KeyboardGL(unsigned char key, int x, int y)
{
	if(key == 1)
	{
		//perror("changing Angle Clockwise");
		g.cue.x += .1;
	}
	if(key == 4)
	{
		g.cue.x -= .1;
		//perror("changing Angle CounterClockwise");
	}

	if(key == 'a')
	{
		//perror("changing Angle Clockwise");
		g.cue.x += 1;
	}
	if(key == 'd')
	{
		g.cue.x -= 1;
		//perror("changing Angle CounterClockwise");
	}

	if(key == 23)
	{
		if(g.cue.y < .7)
			g.cue.y += .01;
		//perror("power up!");
	}	
	if(key == 19)
	{
		if(g.cue.y > 0)
			g.cue.y -= .01;
		//perror("power down!");
	}

	if(key == 'w')
	{
		if(g.cue.y < .7)
			g.cue.y += .1;
		//perror("power up!");
	}	
	if(key == 's')
	{
		if(g.cue.y > 0)
			g.cue.y -= .1;
		//perror("power down!");
	}
	if(key == 'A')
	{
		//perror("changing Angle Clockwise");
		g.cue.x += 10;
	}
	if(key == 'D')
	{
		g.cue.x -= 10;
		//perror("changing Angle CounterClockwise");
	}
	if(key == 'W')
	{
		if(g.cue.y < .7)	
			g.cue.y += .5;
		//perror("power up!");
	}	
	if(key == 'S')
	{
		if(g.cue.y > 0)
			g.cue.y -= .5;
		//perror("power down!");
	}			
	if(key == 13)
	{
		impulse(&g);
		sg.cue.y = -10;		
	}
	if(key == 't')
	{
		shadowEnabled ^= 1;
	}	
}

void InitGL(int* argc, char** argv)
{
    // initialize GLUT 
    glutInit(argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowPosition (glutGet(GLUT_SCREEN_WIDTH)/2 - window_width/2, 
                            glutGet(GLUT_SCREEN_HEIGHT)/2 - window_height/2);
    glutInitWindowSize(window_width, window_height);
    iGLUTWindowHandle = glutCreateWindow("OpenCL/GL Interop (VBO)");

    // register GLUT callback functions
    glutDisplayFunc(DisplayGL);
    glutKeyboardFunc(KeyboardGL);
    glutTimerFunc(REFRESH_DELAY, timerEvent,0);

	// initialize necessary OpenGL extensions
    glewInit();
    GLboolean bGLEW = glewIsSupported("GL_VERSION_2_0 GL_ARB_pixel_buffer_object"); 

    // default initialization
    glClearColor(0.0, 0.0, 0.0, 1.0);
    //glDisable(GL_DEPTH_TEST);

    // viewport
    glViewport(0, 0, window_width, window_height);



    return;
}

void timerEvent(int value)
{

    ball_transfer(&g.b,0);
    //if everything has stopped moving
    stopped = 1;
    for(int i = 0; i < NUMBER_OF_BALLS;i++)
    {
    	//printf("%d: x:%f\ty:%f\tvx:%f\tvy:%f\t%f\n",i,g.b.pos[i].x,g.b.pos[i].y,g.b.vel[i].x,g.b.vel[i].y,g.b.pos[i].w);
    	if((fabs(g.b.vel[i].x) > .1 || fabs(g.b.vel[i].y) > .1) && g.b.pos[i].w)
    		stopped = 0;
    }
    if(stopped)
    {
	if(shadowEnabled)
		runTest();

    	if(g.b.pos[0].w == 0)
    	{
    		g.b.pos[0] = {TABLE_WIDTH/2,TABLE_HEIGHT/4,0,1};
    		g.b.vel[0] = {0,0,0,1};    		
    		ball_transfer(&g.b,1);
    	}
    }
    glutPostRedisplay();
	glutTimerFunc(REFRESH_DELAY, timerEvent,0);
}

void DisplayGL()
{
    // clear graphics then render 
    ball_transfer(&(g.b),0);
    	
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
    // projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (GLfloat)window_width / (GLfloat) window_height, 0.1, 10.0);

    // set view matrix
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRotatef(90,0,0,1);
    glTranslatef(-TABLE_WIDTH/2, -TABLE_HEIGHT/2, -2);  
    
    glBegin(GL_QUADS);
	//draw outer table
    	glColor4f(.3,.3,0,0);
    	glVertex3f(-0.1,-0.1,0.1);
    	glVertex3f(-0.1,0.1+g.t.stats.y,0.1);
    	glVertex3f(0.1+g.t.stats.x,0.1+g.t.stats.y,0.1);
    	glVertex3f(0.1+g.t.stats.x,-0.1,0.1);

	//draw inner table
    	glColor4f(.1,.5,.1,0);	
    	glVertex3f(-0.0,-0.0,0.1);
    	glVertex3f(-0.0,0.0+g.t.stats.y,0.1);
    	glVertex3f(0.0+g.t.stats.x,0.0+g.t.stats.y,0.1);
    	glVertex3f(0.0+g.t.stats.x,-0.0,0.1);    	
    glEnd();
    //draw pockets
    for(int i = 0; i < 6; i++)
    {
        glColor4f(.1,.1,.1,0);
    	glBegin(GL_POLYGON);
    	for(int j = 0; j < 360; j += 30)
    	{
    		glVertex3f(g.t.pockets[i].x + g.t.pockets[i].w*cos(j*(M_PI/180.0)),g.t.pockets[i].y + g.t.pockets[i].w*sin(j*(M_PI/180.0)) , g.t.pockets[i].z + .1);	
    	}
    	glEnd();
    }
    
    //draw balls
    for(int i = 0; i < NUMBER_OF_BALLS; i++)
    {
    	if(i == 0) glColor4f(.9,.9,.9,0);
	else if(i == 8) glColor4f(.0,.0,.0,0);
	else if(i == 9) glColor4f(.9,.1,.1,0);
	else if(i % 2 == 0) glColor4f(.9,.1,.1,0);
	else if(i % 2 == 1) glColor4f(.1,.1,.9,0);

	glPushMatrix();
		glTranslatef(g.b.pos[i].x -g.b.stats[i].y/2,g.b.pos[i].y-g.b.stats[i].y/2,g.b.pos[i].z + g.b.stats[i].y/2+.1);
		glutSolidSphere(g.b.stats[i].y,9,9);
	glPopMatrix();
    }
    
    //draw cue
    if(stopped)
    {
	    glPushMatrix();
	   	 glColor4f(.7,.7,.7,0); 
		 glTranslatef(g.b.pos[0].x-g.b.stats[0].y/2,g.b.pos[0].y-g.b.stats[0].y/2,g.b.pos[0].z+.1);
	   	 glRotatef(g.cue.x,0,0,1);
		 glTranslatef(g.b.stats[0].y*2+g.cue.y/5,0,0);
		 glRotatef(-90,0,1,0);
	   	 glutSolidCone(g.b.stats[0].y,g.b.stats[0].y*2,9,9);
	    glPopMatrix();   	 

	   //draw stats
		char string[128];
		glColor4f(.9,.9,.9,0);
		sprintf(string,"Cue Angle: %f\nCue Power: %f",g.cue.x,g.cue.y);
		glRasterPos3f(2,2,-1);
		glutBitmapString(GLUT_BITMAP_HELVETICA_12,(unsigned char*)string); 

 	  //draw guide
 	  if(shadowEnabled)
 	  {
		    for(int i = 0; i < NUMBER_OF_BALLS; i++)
		    {
		    	if(i == 0) glColor4f(.45,.45,.45,0);
			else if(i == 8) glColor4f(.0,.0,.0,0);
			else if(i == 9) glColor4f(.9/2,.1/2,.1/2,0);
			else if(i % 2 == 0) glColor4f(.9/2,.1/2,.1,0);
			else if(i % 2 == 1) glColor4f(.1/2,.1/2,.9,0);

			glPushMatrix();
				glTranslatef(sg.b.pos[i].x -sg.b.stats[i].y/2,sg.b.pos[i].y-sg.b.stats[i].y/2,sg.b.pos[i].z + sg.b.stats[i].y/2+.1);
				glutSolidSphere(sg.b.stats[i].y,9,9);
			glPopMatrix();
			if(sg.b.pos[i].w)
			{
				glBegin(GL_LINES);
					glColor4f(.0,.0,.0,0);
					glVertex3f(sg.b.pos[i].x - sg.b.vel[i].x*.2,sg.b.pos[i].y - sg.b.vel[i].y*.2,sg.b.pos[i].z+.1);
				    	if(i == 0) glColor4f(.45,.45,.45,0);
					else if(i == 8) glColor4f(.0,.0,.0,0);
					else if(i == 9) glColor4f(.9/2,.1/2,.1/2,0);
					else if(i % 2 == 0) glColor4f(.9/2,.1/2,.1,0);
					else if(i % 2 == 1) glColor4f(.1/2,.1/2,.9,0);
					glVertex3f(sg.b.pos[i].x + sg.b.vel[i].x*.2,sg.b.pos[i].y + sg.b.vel[i].y*.2,sg.b.pos[i].z+.1);
				glEnd();
			}
		    }	  
 	  
 	  }
	  	     
   }
   
 	
   glutSwapBuffers();

}

//setup the game.  It sets all the required variables to be what they need to be!
void rack(Game * g)
{
	double y;
	g->b.pos[0] = {TABLE_HEIGHT/4,TABLE_HEIGHT/4,0,1};
	int ball = 1;
	
	g->b.pos[1] = {TABLE_HEIGHT/4,(TABLE_HEIGHT-TABLE_HEIGHT/4),0,1};
	
	g->b.pos[2] = {TABLE_HEIGHT/4-BALL_RADIUS*1.1,(TABLE_HEIGHT-TABLE_HEIGHT/4)+BALL_RADIUS*2.2,0,1};
	g->b.pos[3] = {TABLE_HEIGHT/4+BALL_RADIUS*1.1,(TABLE_HEIGHT-TABLE_HEIGHT/4)+BALL_RADIUS*2.2,0,1};
	
	g->b.pos[4] = {TABLE_HEIGHT/4+BALL_RADIUS*1.1,(TABLE_HEIGHT-TABLE_HEIGHT/4)+3*BALL_RADIUS*2.2,0,1};
	g->b.pos[5] = {TABLE_HEIGHT/4-2*BALL_RADIUS*1.1,(TABLE_HEIGHT-TABLE_HEIGHT/4)+2*BALL_RADIUS*2.2,0,1};
	g->b.pos[6] = {TABLE_HEIGHT/4+2*BALL_RADIUS*1.1,(TABLE_HEIGHT-TABLE_HEIGHT/4)+2*BALL_RADIUS*2.2,0,1};
	
	g->b.pos[7] = {TABLE_HEIGHT/4-BALL_RADIUS*1.1,(TABLE_HEIGHT-TABLE_HEIGHT/4)+3*BALL_RADIUS*2.2,0,1};
	g->b.pos[8] = {TABLE_HEIGHT/4,(TABLE_HEIGHT-TABLE_HEIGHT/4)+2*BALL_RADIUS*2.2,0,1};
	g->b.pos[9] = {TABLE_HEIGHT/4-3*BALL_RADIUS*1.1,(TABLE_HEIGHT-TABLE_HEIGHT/4)+3*BALL_RADIUS*2.2,0,1};
	g->b.pos[10] = {TABLE_HEIGHT/4+3*BALL_RADIUS*1.1,(TABLE_HEIGHT-TABLE_HEIGHT/4)+3*BALL_RADIUS*2.2,0,1};
	
	g->b.pos[11] = {TABLE_HEIGHT/4,(TABLE_HEIGHT-TABLE_HEIGHT/4)+4*BALL_RADIUS*2.2,0,1};
	g->b.pos[12] = {TABLE_HEIGHT/4-2*BALL_RADIUS*1.1,(TABLE_HEIGHT-TABLE_HEIGHT/4)+4*BALL_RADIUS*2.2,0,1};
	g->b.pos[13] = {TABLE_HEIGHT/4+2*BALL_RADIUS*1.1,(TABLE_HEIGHT-TABLE_HEIGHT/4)+4*BALL_RADIUS*2.2,0,1};
	g->b.pos[14] = {TABLE_HEIGHT/4-4*BALL_RADIUS*1.1,(TABLE_HEIGHT-TABLE_HEIGHT/4)+4*BALL_RADIUS*2.2,0,1};
	g->b.pos[15] = {TABLE_HEIGHT/4+4*BALL_RADIUS*1.1,(TABLE_HEIGHT-TABLE_HEIGHT/4)+4*BALL_RADIUS*2.2,0,1};
	g->t.stats = {TABLE_WIDTH,TABLE_HEIGHT,1,0};
	
	g->t.pockets[0] = {0,0,0,2.25*BALL_RADIUS};
	g->t.pockets[2] = {0,TABLE_HEIGHT,0,2.25*BALL_RADIUS};
	
	g->t.pockets[3] = {TABLE_HEIGHT/2,0,0,2.25*BALL_RADIUS};
	g->t.pockets[5] = {TABLE_HEIGHT/2,TABLE_HEIGHT,0,2.25*BALL_RADIUS};

	g->t.pockets[1] = {0,TABLE_HEIGHT/2,0,2.25*BALL_RADIUS};
	g->t.pockets[4] = {TABLE_HEIGHT/2,TABLE_HEIGHT/2,0,2.25*BALL_RADIUS};
	
	for(int i= 0; i < NUMBER_OF_BALLS;i++)
	{
		g->b.stats[i] = {BALL_MASS,BALL_RADIUS,BALL_SFRICTION,BALL_DFRICTION};
	}
	g->cue = {-90,0,0,0};
	ball_transfer(&g->b,1);				
	table_transfer(&g->t,1);
}

void init(int platform, int device)
{
	//initialize OpenGL
	//InitGL(&argc, argv);
	if(DEBUG) printf("----------------------------------------------\n");
	if(DEBUG) printf("------INITIATING OPENCL PLEASE STAND BY-------\n");
	if(DEBUG) printf("----------------------------------------------\n");

	unsigned int i; 
	int result;
	//platform information
 	if(DEBUG) perror("getting basic openCL data: platform info and number of devices");
	
	check(clGetPlatformIDs(8,platforms, &num_platforms));
	printf("num platforms: %d\n",num_platforms);
	//get info for first platform
	unsigned int num_devices = 0;
	for(i = 0; i < num_platforms; i++)
	{
		if(DEBUG) printf("Platform %d\n", i);
		check(clGetPlatformInfo(platforms[i], CL_PLATFORM_PROFILE, (size_t) 32, (void*)	param_value_buffer, &param_value_size_ret));
 		if(DEBUG) printf("\tProfile: %s\n",(char*) param_value_buffer);	
		check(clGetPlatformInfo(platforms[i], CL_PLATFORM_VERSION , (size_t) 32, (void*)	param_value_buffer, &param_value_size_ret));
 		if(DEBUG) printf("\tVersion: %s\n", (char*) param_value_buffer);	
		check(clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, (size_t) 32, (void*)	param_value_buffer, &param_value_size_ret));
 		if(DEBUG) printf("\tName: %s\n", (char*) param_value_buffer);	
		check(clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, (size_t) 32, (void*)	param_value_buffer, &param_value_size_ret));
 		if(DEBUG) printf("\tVendor: %s\n",(char*)  param_value_buffer);	
		//check(clGetPlatformInfo(platforms[i], CL_PLATFORM_EXTENSIONS, (size_t) 32,(void*) param_value_buffer, &param_value_size_ret));
 		if(DEBUG) printf("\tExtensions: %s\n",(char*)  param_value_buffer);	
		check(clGetPlatformInfo(platforms[i], CL_PLATFORM_PROFILE, (size_t) 32, (void*)	param_value_buffer, &param_value_size_ret));	
		check(clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 8, device_list, &num_devices));
	
		char p1[512];
		p1[511] = '\0';
		if(DEBUG) printf("\tnum devices: %d\n\n", num_devices);
		for(i = 0; i < num_devices; i++)
		{
			cl_ulong size;
			check(clGetDeviceInfo(device_list[i], CL_DEVICE_NAME, 512, (void*) p1, NULL));
			if(DEBUG) printf("\tdevice %d: %s\n", i, p1);
			check(clGetDeviceInfo(device_list[i], CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, sizeof(cl_ulong), (void*) &size, NULL));
			if(DEBUG) printf("\t\tGlobal Cache Size: %d\n", i, size);
			check(clGetDeviceInfo(device_list[i], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), (void*) &size, NULL));
			if(DEBUG) printf("\t\tGlobal Mem Size: %d\n", i, size);
			check(clGetDeviceInfo(device_list[i], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), (void*) &size, NULL));
			if(DEBUG) printf("\t\tlocal mem size: %d\n",size);
			check(clGetDeviceInfo(device_list[i], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), (void*) &size, NULL));		
			if(DEBUG) printf("\t\tmemory available: %d\n", i, p1[0]);
		}	
	}
	if(DEBUG) perror("Creating context");
	check(clGetDeviceIDs(platforms[platform], CL_DEVICE_TYPE_ALL, 8, device_list, &num_devices));
	//create compute context over all available devices
	context = clCreateContext(NULL,1,&device_list[device],NULL,NULL,&result);	
	check(result);	

	//get devices available to context
	size_t nContextDescriptorSize = 0;
	
	clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, 0, &nContextDescriptorSize);
	
	cl_device_id * aDevices = (cl_device_id*) malloc(nContextDescriptorSize);
	
	clGetContextInfo(context, CL_CONTEXT_DEVICES, nContextDescriptorSize, aDevices, 0);	

	//make cpu queue
	queue = clCreateCommandQueue(context,aDevices[0],NULL,&result);
	check(result);

	const char* program_source = (char*) readin("kernel.cl");
	
	program = clCreateProgramWithSource(context,1, &program_source,NULL,&result);
	if (result != CL_SUCCESS)
  	{
  		check(result);
        	size_t len;
        	char buffer[5000];
        	clGetProgramBuildInfo(program, aDevices[0], CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        	printf("%s\n", buffer);
        	exit(1);

    	}
    	if(DEBUG) perror("program loaded");

	//build for cpu
	result = clBuildProgram(program,1,aDevices,NULL,NULL,NULL);
	if (result != CL_SUCCESS)
  	{
  		check(result);
  		if(DEBUG) printf("build fail\n");
        	size_t len;
        	char buffer[5000];
        	for(i = 0; i < num_devices; i++)
        	{
        		memset(buffer,0,sizeof(buffer));
      		  	clGetProgramBuildInfo(program, device_list[i], CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        		printf("%s\n", buffer);
		}
        	exit(1);

    	}
    	
    	if(DEBUG) perror("program built");
	//build compute kernel
	
	ballCollision_kernel = clCreateKernel(program,"ballCollision",&result);
	check(result);
	pocketCollisions_kernel = clCreateKernel(program,"pocketCollisions",&result);
	check(result);
	tableCollision_kernel = clCreateKernel(program,"tableCollision",&result);
	check(result);
	moveballs_kernel = clCreateKernel(program,"moveballs",&result);
	check(result);
	impulse_kernel = clCreateKernel(program,"impulse",&result);
	check(result);		
	if(DEBUG) perror("compute kernels ready");	
	
	//setup buffers and whatnot
	g.b.gpu_pos = clCreateBuffer(context,CL_MEM_READ_WRITE, sizeof(TYPE4)*NUMBER_OF_BALLS,NULL,&result);
	g.b.gpu_vel = clCreateBuffer(context,CL_MEM_READ_WRITE, sizeof(TYPE4)*NUMBER_OF_BALLS,NULL,&result);
	g.b.gpu_stats = clCreateBuffer(context,CL_MEM_READ_WRITE, sizeof(TYPE4)*NUMBER_OF_BALLS,NULL,&result);		
	
	g.t.gpu_stats = clCreateBuffer(context,CL_MEM_READ_WRITE, sizeof(TYPE4),NULL,&result);	
	g.t.gpu_pockets = clCreateBuffer(context,CL_MEM_READ_WRITE, sizeof(TYPE4)*6,NULL,&result);
	

	sg.b.gpu_pos = clCreateBuffer(context,CL_MEM_READ_WRITE, sizeof(TYPE4)*NUMBER_OF_BALLS,NULL,&result);
	sg.b.gpu_vel = clCreateBuffer(context,CL_MEM_READ_WRITE, sizeof(TYPE4)*NUMBER_OF_BALLS,NULL,&result);
	sg.b.gpu_stats = clCreateBuffer(context,CL_MEM_READ_WRITE, sizeof(TYPE4)*NUMBER_OF_BALLS,NULL,&result);		
	
	sg.t.gpu_stats = clCreateBuffer(context,CL_MEM_READ_WRITE, sizeof(TYPE4),NULL,&result);	
	sg.t.gpu_pockets = clCreateBuffer(context,CL_MEM_READ_WRITE, sizeof(TYPE4)*6,NULL,&result);	
			
	rack(&g);
}

//impart force on queueball
void impulse(Game *g)
{
	//__kernel void impulse(__global TYPE4 * vel, __global TYPE4 * stats,const TYPE angle, const TYPE power)
	size_t gsize = NUMBER_OF_BALLS;
	check(clSetKernelArg(impulse_kernel,0,sizeof(cl_mem),&g->b.gpu_vel));
	check(clSetKernelArg(impulse_kernel,1,sizeof(cl_mem),&g->b.gpu_stats));
	check(clSetKernelArg(impulse_kernel,2,sizeof(TYPE),&g->cue.x));
	check(clSetKernelArg(impulse_kernel,3,sizeof(TYPE),&g->cue.y));	
	check(clEnqueueNDRangeKernel(queue,impulse_kernel,1,0,&gsize,NULL,0,NULL,NULL));	
}

//move the balls forward
void moveBalls(Game *g,TYPE timestep)
{
	//__kernel void moveballs(__global TYPE4 * pos,__global  TYPE4 * vel,__global  TYPE4 * stats,__global  TYPE4 * table, const TYPE timestep)
	size_t gsize = NUMBER_OF_BALLS;
	check(clSetKernelArg(moveballs_kernel,0,sizeof(cl_mem),&g->b.gpu_pos));
	check(clSetKernelArg(moveballs_kernel,1,sizeof(cl_mem),&g->b.gpu_vel));
	check(clSetKernelArg(moveballs_kernel,2,sizeof(cl_mem),&g->b.gpu_stats));
	check(clSetKernelArg(moveballs_kernel,3,sizeof(cl_mem),&g->t.gpu_stats));	
	check(clSetKernelArg(moveballs_kernel,4,sizeof(TYPE),&timestep));		
	check(clEnqueueNDRangeKernel(queue,moveballs_kernel,1,0,&gsize,NULL,0,NULL,NULL));	
}

//collide balls with wall
void collideWalls(Game*g)
{
	size_t gsize = NUMBER_OF_BALLS;
	//__kernel void tableCollision(__global TYPE4 * pos,__global  TYPE4 * vel,__global  TYPE4 * stats,__global  TYPE4 * table)	
	check(clSetKernelArg(tableCollision_kernel,0,sizeof(cl_mem),&g->b.gpu_pos));
	check(clSetKernelArg(tableCollision_kernel,1,sizeof(cl_mem),&g->b.gpu_vel));
	check(clSetKernelArg(tableCollision_kernel,2,sizeof(cl_mem),&g->b.gpu_stats));
	check(clSetKernelArg(tableCollision_kernel,3,sizeof(cl_mem),&g->t.gpu_stats));	
	check(clEnqueueNDRangeKernel(queue,tableCollision_kernel,1,0,&gsize,NULL,0,NULL,NULL));	
}

void run()
{
	
	glutMainLoop();
}

void ball_transfer(Balls*b,int dest)
{
	if(dest == 1)
	{
		clEnqueueWriteBuffer(queue,b->gpu_pos,CL_FALSE,0,sizeof(TYPE4)*NUMBER_OF_BALLS,b->pos,0,NULL,NULL);
		clEnqueueWriteBuffer(queue,b->gpu_vel,CL_FALSE,0,sizeof(TYPE4)*NUMBER_OF_BALLS,b->vel,0,NULL,NULL);
		clEnqueueWriteBuffer(queue,b->gpu_stats,CL_FALSE,0,sizeof(TYPE4)*NUMBER_OF_BALLS,b->stats,0,NULL,NULL);
	}
	if(dest == 0)
	{
		clEnqueueReadBuffer(queue,b->gpu_pos,CL_FALSE,0,sizeof(TYPE4)*NUMBER_OF_BALLS,b->pos,0,NULL,NULL);	
		clEnqueueReadBuffer(queue,b->gpu_vel,CL_FALSE,0,sizeof(TYPE4)*NUMBER_OF_BALLS,b->vel,0,NULL,NULL);		
	}
}

//balls collide with pockets
void collidePockets(Game * g)
{
	size_t gsize = NUMBER_OF_BALLS;
	char pockets = 6; 
	//__kernel void pocketCollisions(__global TYPE4 * pos,__global TYPE4 * ps, const char pockets)
	check(clSetKernelArg(pocketCollisions_kernel,0,sizeof(cl_mem),&g->b.gpu_pos));
	check(clSetKernelArg(pocketCollisions_kernel,1,sizeof(cl_mem),&g->t.gpu_pockets));	
	check(clSetKernelArg(pocketCollisions_kernel,2,sizeof(char),&pockets));		
	check(clEnqueueNDRangeKernel(queue,pocketCollisions_kernel,1,0,&gsize,NULL,0,NULL,NULL));	
}

//collide balls with balls
void collideBalls(Game*g,TYPE timestep)
{
	size_t gsize = NUMBER_OF_BALLS;
	char balls = NUMBER_OF_BALLS; 	
	check(clSetKernelArg(ballCollision_kernel,0,sizeof(cl_mem),&g->b.gpu_pos));
	check(clSetKernelArg(ballCollision_kernel,1,sizeof(cl_mem),&g->b.gpu_vel));
	check(clSetKernelArg(ballCollision_kernel,2,sizeof(cl_mem),&g->b.gpu_stats));
	check(clSetKernelArg(ballCollision_kernel,3,sizeof(char),&balls));	
	check(clSetKernelArg(ballCollision_kernel,4,sizeof(TYPE),&timestep));		
	check(clEnqueueNDRangeKernel(queue,ballCollision_kernel,1,0,&gsize,&gsize,0,NULL,NULL));		
	//__kernel void ballCollision(__global TYPE4 * pos,__global  TYPE4 * vel,__global  TYPE4 * stats, const char balls)	
}
void table_transfer(Table*t,int dest)
{
	if(dest == 1)
	{
		clEnqueueWriteBuffer(queue,t->gpu_stats,CL_FALSE,0,sizeof(TYPE4)*1,&t->stats,0,NULL,NULL);
		clEnqueueWriteBuffer(queue,t->gpu_pockets,CL_FALSE,0,sizeof(TYPE4)*6,t->pockets,0,NULL,NULL);
	}
	if(dest == 0)
	{
	}
}
