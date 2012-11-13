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

##Creating Kfusion Libraries
###Library Annotations
###Kernel Annotations

##Using Kfusion Libraries
###Aplication annotation
