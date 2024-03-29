# KFusion
A prototype tool which re-modularizes OpenCL code at compile time to improve performance.  It combines kernels and functios in order to amortize memory access costs associated with bandwidth and latency.  It is aimed at GPGPU computing, but works for any platform OpenCL is capable of executing on.  KFusion allows a user to use simple pragmas to fuse functions and their underlying kernels.  This supports abstraction, but completes low level trasnformations in order to improve performance.

## Dependencies
KFusion requires [PLY](http://www.dabeaz.com/ply/) the Python Lex Yacc library.  It uses this in order to tokenize the underlying C code.  It seems the best way to ensure compatibility is to copy the ply folder found inside the ply download directly into KFusion directory.

The examples are C programs as require MAKE as well as NVCC.  NVCC is the NVidia C compiler which correctly links in OpenCL, it is possible to use other OpenCL distributions during compilation, but this may require you to alter the makefiles.


## Kfusion Invocaton
Kfusion is a prototype tool implemented in python.  The main preprocessor is preprocessor and it takes three inputs:

1. The main file which executes a series of library functions to be fused.
2. The library file which contains a series of library functions which each call kernels
  *Each library function to be fused must contain a kernel
  *The library with name xyz.ext is assumed to have a header file named xyz.h
3. A kernel file containing a series of kernels which are executed.

As KFusion is a prototype, there must be exactly one of each file.  This is invoked:
*python preprocessor main.c library.c kernel.cl*

Each set of files is assumed to be annotated.  This allows a library developer to effectively create an library which changes to match it's use case and a application developer to leverage libraries and attain near hand coded performance.


## Examples and Case Studies
Examples of how to build/leverage Kfusion enabled libraries can be seen in the three case studes included in this project.  Feel free to extend these to meet your needs.
* **imagMan** - contains a series image manipulation operations and operates on png files. This provides the best performance case for KFusion.
* **lclBlas** - contains a small linear algebra library, suitable for such things as the conjugate gradient method
* **physics** - contains a simple rigid body simulation used to create a pool game.  Needs to be extended to handle more complex physical interaction between the pool balls and table, but it's a good start.

## Using KFusion Libraries
In order to use KFusion, it requires an OpenCL library annotated as given below.  Given a library, fusing functions and kernels can be done entirely at the application level:

### Application level Annotations
At the application level, a user can annotate which functions to fuse using 2 easy to use pragmas:
* #Pragma startfuse
* #Pragma endfuse

Any functions within this region will be combined using Kfusion.  This will produce a new function and new kernel which combines the functionality within the pragmas.  It's important to note, KFusion eliminates intermediate results and therefore will only produce the result of the last function fused.

## Creating Kfusion Libraries
In order to allow for an OpenCL library to leverage Kfusion, it must follow a few simple requirements as well as leverage a set of annotations. 

### Library requirements
There are a few requirements which need to be met in order to use KFusion:
* A library must seperate functionality into a series of function which call kernels.  
* The library file should be seperate from the kernels which are loaded from a seperate text file.  This allows each to be eddited and new functions to be created.  
* The libary must use a `void init(int argc, char ** argv)` function to initialize OpenCL and load kernels.  KFusion will build on this.
* Each library function must call one kernel directly.
* kFusion expects you to wrap OpenCL calls in a function refered to as `check(short ERRCODE)`.  There are examples of this in check.c and check.h found in each of the example libraries

### Library Annotations
At the library level, functions need to be annotated with regards to synchronization.  This details if a function requires synchronized output.  

This involves a single pragma: 
* #pragma sync in | out

This determines if the function will be fused with the surrounding kernels.  This ensures safety and that functions will be correctly combined.  This makes it see the fusion pragma's can be used with no negative repercussions.

An example of a sync in operation is a matrix vector multiplication as each element of the vector will be required.

An example of a sync out operation is a dot product as it will reduce to a single value.

### Kernel Annotations

There are three kernel annotations.  
* #pragma immovable -- ensures an asynchronous operation will not be rearranged during optimization.
* #pragma load -- declares the next operation is a load instruction.  
* #pragma store -- declares the next operation is a store instruction.

The immovable pragma prevents potentially destructive operations.

The following pragmas denote load and store operations.  These will be used to fuse kernels.  Fusion occurs by matching input and outputs in order to build a dependency tree.  The tree is collapsed by matching inputs to outputs.  Asynchronous instructions are rearranged and then the final result is output.  This amortizes costs and greatly improves performance - on par with hand fused kernels.

## For Apple Users:
You may have to make significant ---trying--- changes to the codebases in order to have them execute on Apple hardware.  Details on these changes will be coming

## Issues
KFusion is still a prototype tool and, while it works for the cases presented as examples here, may not work for all codebases.  It makes some assumptions and has limitations.  If it fails for you, feel free to contact me by email and let me know.

Thanks


--Liam
