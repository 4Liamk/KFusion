#KFusion

A tool which re-modularizes OpenCL code at compile time to improve performance.  It combines kernels and functios in order to amortize memory access costs associated with bandwidth and latency.  It is aimed at GPGPU computing, but works for any platform OpenCL is capable of executing on.  KFusion allows a user to use simple pragmas to fuse functions and their underlying kernels.  This supports abstraction, but completes low level trasnformations in order to improve performance.

##Invocaton
Kfusion is a prototype tool implemented in python.  The main preprocessor is preprocessor and it takes three inputs:

1. The main file which executes a series of library functions to be fused.
2. The library file which contains a series of library functions which each call kernels
  *Each library function to be fused must contain a kernel
  *The library with name xyz.ext is assumed to have a header file named xyz.h
3. A kernel file containing a series of kernels which are executed.

As KFusion is a prototype, there must be exactly one of each file.  This is invoked:
*python preprocessor main.c library.c kernel.cl*

Each set of files is assumed to be annotated.  This allows a library developer to effectively create an library which changes to match it's use case and a application developer to leverage libraries and attain near hand coded performance.


##Examples and Case Studies
Examples of how to build/leverage Kfusion enabled libraries can be seen in the three case studes included in this project.  Feel free to extend these to meet your needs.

* **imagMan** - contains a series image manipulation operations and operates on png files. This provides the best performance case for KFusion.
* **lclBlas** - contains a small linear algebra library, suitable for such things as the conjugate gradient method
* **physics** - contains a simple rigid body simulation used to create a pool game.  Needs to be extended to handle more complex physical interaction between the pool balls and table, but it's a good start.

##Using Kfusion Libraries
###Aplication annotation
At the application level, a user can annotate which functions to fuse using 2 easy to use pragmas:
* #Pragma startfuse
* #Pragma endfuse
Any functions within this region will be combined using Kfusion.  This will produce a new function and new kernel which combines the functionality within the pragmas.  It's important to note, KFusion eliminates intermediate results and therefore will only produce the result of the last function fused.

##Creating Kfusion Libraries
In order to allow for an OpenCL library to leverage Kfusion, it must follow a few simple requirements as well as leverage a set of annotations. 

###Library requirements
There are a few requirements which need to be met in order to use KFusion:
* A library must seperate functionality into a series of function which call kernels.  
* The library file should be seperate from the kernels which are loaded from a seperate text file.  This allows each to be eddited and new functions to be created.  
* The libary must use an *void init(int argc, char ** argv)* function to initialize OpenCL and load kernels.  KFusion will build on this.
* Each library function must call one kernel directly.

###Library Annotations
At the library level, functions need to be annotated with regards to synchronization.  This details if a function requires synchronized output.  

This involves a single pragma: 
* #pragma sync in | out

This determines if the function will be fused with the surrounding kernels.  This ensures safety and that functions will be correctly combined.  This makes it see the fusion pragma's can be used with no negative repercussions.

An example of a sync in operation is a matrix vector multiplication as each element of the vector will be required.

An example of a sync out operation is a dot product as it will reduce to a single value.

###Kernel Annotations

There are three kernel annotations.  
* #pragma immovable -- ensures an asynchronous operation will not be rearranged during optimization.
* #pragma load -- declares the next operation is a load instruction.  
* #pragma store -- declares the next operation is a store instruction.

The immovable pragma prevents potentially destructive operations.

The following pragmas denote load and store operations.  These will be used to fuse kernels.  Fusion occurs by matching input and outputs in order to build a dependency tree.  The tree is collapsed by matching inputs to outputs.  Asynchronous instructions are rearranged and then the final result is output.  This amortizes costs and greatly improves performance - on par with hand fused kernels.

Thanks

--Liam