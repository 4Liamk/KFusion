"""
This file is to remove some of the insanely specific nature of Kfusion by allow you to name some things based on how you OpenCL library may operate.
I used very simple names because I only created one OpenCL context queue and program, you may not.

//context and queue information
extern cl_context     	 context;
extern cl_command_queue queue;
extern cl_program program;
"""

#these are a collection of variable names KFusion will use as global variables throughout
clContexName = "context"
clQueueName = "queue"
clProgramName = "program"

#if you use any additional protected words, please list them here.
additionalProtectedWords = [""]

#additional tokens you may want to parse, should be added to tokens.py
