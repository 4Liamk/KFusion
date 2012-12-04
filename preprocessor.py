#!/usr/bin/python
import sys
import copy
from tokens import *
from statements import *
from function import *
from kernelInvocation import *
from kernel import *
from fusion import *
			

replacements = dict()

"""
	KFusion preprocessor.  This will go through your source files and produce new ones.  
	The process is verbose and somewhat unpolished.  This means it is somewhat stricts

	File requirements:
		KFusion expects three files: a mainfile, a library file and a kernel file.
		It also expects for library lib.cpp or lib.c there is a header file lib.h which has the appropriate header guards.
		KFusion will produce another set of files which have the original name appended with -out
		main.c library.c library.h kernel.cl ==> main-out.c library-out.c library-out.h kernel-out.cl

	Main file requirements:
		The main file should call an init function and be where all the fusion statement are.  
		These statements should involve library functions, if functions are not found within the library, this will cause an error
		the init function must be called "init".  init should take two integers as arguments.  This should change to be a more general case.
	Library File:
		The library file needs a series of global variables for the OpenCL context.  some of these will require specific names like in the example file as listed here:
			cl_context context;
			cl_command_queue queue;
			cl_program program;
	Kernel File:
		The kernel file needs each of it's kernels annotated using #pragma load and #pragma store

"""	
def main():
	"""Main method:
		read in from three files: main, library and kernel.  
		In the main we find fusion regions and fuse them depending on the type of fusion.
		
		Vertical fusion fuses functions vertically removing I/O and is effectively deforestation
		
		Horization fusion fuses functions horizatonally effectively allowing for several independent operations to occure on the same hardware in the same kernel.  This improve capacity and allows us to leverage concurrency.
		"""

	global replacements
	replacements = dict()	
	if(len(sys.argv) < 3) :
		print "Correct Usage : ./preproc2 <mainfile> <libfile> <kernelfile>"
		
	mainfile = sys.argv[1]	#"main.cpp"
	libfile = sys.argv[2]	#"img.cpp"
	kernelfile = sys.argv[3]	#"kernels.cl"
	namespace = "../imagproc-c/lclImage"
	
	temp = libfile.split(".")[0] + ".h"
	headerFileName = libfile.split(".")[0] + ".h"
	header = open(temp,"r")
	lexer = lex.lex()
	
	temp = mainfile.split(".")[0] + "-out." + mainfile.split(".")[1]
	mainout = open(temp,"w")
	
	temp = libfile.split(".")[0] + "-out." + libfile.split(".")[1]
	libout = open(temp,"w")
	
	temp = kernelfile.split(".")[0] + "-out." + kernelfile.split(".")[1]
	kernelout = open(temp,"w")
	
	temp= libfile.split(".")[0] + "-out.h"
	headerout=open(temp,"w")
	

	replacements["\""+libfile.split(".")[0] + ".h" + "\""] = "\""+libfile.split(".")[0] + "-out.h" + "\""
	replacements["\""+kernelfile+"\""] = "\""+ kernelfile.split(".")[0] + "-out." + kernelfile.split(".")[1] + "\""

	#go through main file
	state = 0
	calls = []
	
	#collection of fused calls
	fusedfunctions = []
	print "Kernel File Analysis"
	lexer = TokenReader(kernelfile,replacements,kernelout)
	kernels = []
	while True:
		tok = lexer.tw()
		if not tok:
		   break
		elif(tok.value == "__kernel"):
			kernels.append(kernel(lexer))
			

	print "Library Analysis"
	""""
	plow through our library an make some functions 
		"""		   
	state = 0
	pre = 0
	outfile = libout
	functions = []
	lexer = TokenReader(libfile,replacements,libout)
	print "opening lib file: ", libfile
	isInit = False
	isSyncIn = False
	isSyncOut = False
	while True:
		tok = lexer.tw()
		if not tok:
		   break
		elif(tok.type == "PRAGMA"):
			words =  tok.value.split()
			if("synchronize" in words):
				if("out" in words):
					isSyncOut = True
				if("in" in words):
					isSyncIn = True	   
		if(tok.type == 'TYPEID'):
			tok2 = lexer.tw()
			if not tok2:
				break
			if(tok2.type == 'ID'):
			   tok3 = lexer.tw()
			   if not tok3:
				break
			   if(tok3.type == 'LPAREN'):
				functions.append(function(tok,tok2,lexer))
				functions[-1].isSyncIn = isSyncIn
				functions[-1].isSyncOut = isSyncOut
				isSyncIn = False
				isSyncOut = False			
	

	print "Main File Analysis and Synthesis"
	"""
	State 0 - nothing special look for start fuse, but otherwise print out input
	State 1 - we are in a fusion region - catalogue called functions and arguments. also look for exit
	"""
	lexer = TokenReader(mainfile,replacements,mainout)
	fusionType = ''
	while True:
		tok = lexer.token()

		if not tok: 
		    break
		if(tok.value in replacements):
			mainout.write(replacements[tok.value])
		elif(tok.type == "PRAGMA"):
			if(tok.value.split()[1] == "startfuse"):
				state = 1
				print "Starting fusion on line:", tok.lineno
				fusionType = 'VERTICAL'
			elif(tok.value.split()[1] == "starthfuse"):
				state = 1
				fusionType = 'HORIZONTAL'
				print "Starting fusion on line:", tok.lineno
			elif(tok.value.split()[1] == "endfuse"):
				state = 0
				if(calls):	
					fusedfunctions.append(funfusion(calls,fusionType))
					calls = []		
					print "we now have fused function:", fusedfunctions[-1] , fusedfunctions[-1].type
					mainout.write(fusedfunctions[-1].fusedcall.__str__())

			else:
				mainout.write(tok.value)
		elif(state == 1):
			if(tok.type == 'ID'):
				tok4 = lexer.token()
				if(tok4.type == 'LPAREN'):
					"""LOOK A FUNCTION CALL"""
					call = functionCall(tok)
					#handle any synchronization requirement.  A syncIn must fuse all previous calls, a SyncOut must immediately fuse after the call
					found = False
					for fun in functions:
						if fun.ID.value == call.call.value:
							found = True
							if(fun.isSyncIn):
								if(calls):
									fusedfunctions.append(funfusion(calls,fusionType))	
									calls = []	
									mainout.write(fusedfunctions[-1].fusedcall.__str__())
									print "we now have fused function:", fusedfunctions[len(fusedfunctions)-1] , fusedfunctions[len(fusedfunctions)-1].type									
							calls.append(call)
							tok5 = lexer.token()
							arg = ""
							while(tok5.type != 'RPAREN'):
								if(tok5.type == 'COMMA'):
									#print "arg:",arg
									calls[-1].addArg(arg)
									arg = ""
								else:
									arg += str(tok5.value)
								tok5 = lexer.token()
								if(tok5.type == 'RPAREN'):
									#print "END:",arg
									calls[-1].addArg(arg)							
							if(fun.isSyncOut):	
								fusedfunctions.append(funfusion(calls,fusionType))	
								calls = []
								mainout.write(fusedfunctions[-1].fusedcall.__str__())
								print "we now have fused function:", fusedfunctions[len(fusedfunctions)-1] , fusedfunctions[len(fusedfunctions)-1].type
							break;
					if(not found):
						print "Error 1: Function: ", call.call.value , " Not found in library file: ", sys.argv[2]
						exit(1)

		elif(tok.value == "init"):
			mainout.write("initFusion")

		else:
			mainout.write(tok.value)
	mainout.close()
	
	print "library synthesis"
	fusions = []
	setOutput(libout)	
	for fun in fusedfunctions:
		print "creating fusion:",str(fun),"type: ",fun.type
		type = fun.type
		tofuse = []
		for call in fun.funs:
			for f in functions:
				if(f.ID.value == call.call.value):
					cp = copy.deepcopy(f)
					tofuse.append((call,cp))
					print f.ID,cp.ID
		argDict = dict()
		childfunctions = []
		count = 0
		for call, fun in tofuse:
			for i in xrange(len(call.args)):
				if(call.args[i] not in argDict):
					argDict[call.args[i]] = "arg_" + str(count) 
					count += 1
				fun.replaceArg(i,argDict[call.args[i]])
			fun.contaminate()
			childfunctions.append(fun)
		print "Performing ", fun.type, " fusion"
		newfunction = function("void","NEW",None,childfunctions,type)
		print newfunction
		fusions.append(newfunction) 
			
	#take care of having to parse new kernels by adding a new init function
	print "Creating new code to parse newely created kernels"
	for fun in fusions:
		libout.write("cl_kernel " + fun.newKernel + ";\n")
		libout.write(str(fun))
			
	libout.write("void initFusion(int one, int two)\n{\n")
	string = "\tinit(one, two);\n"
	string += "cl_int result;\n"
	for fun in fusions:
		string += "\t" + fun.newKernel + "= clCreateKernel(program,\""+fun.newKernel.split("kernel")[0].strip()
		
		if(fun.ftype == 'HORIZONTAL'):
			string += 'h'
		
		string+="\",&result);" + "\n"
		string += "\tcheck(result);\n" 
	string +="}\n"
	libout.write(string)

	#add used function definitions to the header file
	#this assumes you use header guards.  If you don't it will add an extraneous #endif at the end 
	print "Updating library header file"
	for line in header:
		if(line.strip() != "#endif"):
			headerout.write(line)
	for fun in fusions:
		headerout.write(str(fun.call()))
		headerout.write("extern cl_kernel " + fun.newKernel + ";\n")
	headerout.write("void initFusion(int one, int two);\n")
	headerout.write("#endif\n")
	headerout.close()

	#create the new kernels
	print "Creating new Kernels"
	setOutput(kernelout)
	for fun in fusions:
		tofuse = []
		argDict = dict()
		count = 0
		ftype = fun.ftype
		#print fun.kernelInvocations[-1].args[-1]
		newArg = Statement(None,-1,'OTHER')
		newArg.tokens = []
		newArg.tokens.append(makeToken("const int",'TYPEID'))
		newArg.tokens.append(makeToken("newSize",'ID'))
		cid = 0
		for clkernel in fun.kernelInvocations[:-1]:
			for k in kernels:
				if(clkernel.kernel.children[1].tokens[0].value.split("_")[0] == k.ID.value):
					tofuse.append(copy.deepcopy(k))	
					for i in range(len(clkernel.args)):
						type, value =  clkernel.args[i]
						value = str(value)
						if value not in argDict:
							argDict[value] = "arg_" + str(count)
							count += 1 
						tofuse[-1].replaceArg(i,argDict[value])	
					tofuse[-1].contaminate()
					tofuse[-1].cid = str(cid)
					cid += 1
					if(ftype == 'HORIZONTAL'):
						tofuse[-1].args.append(newArg.tokens)
		#print tofuse[-1]
		tree = fusionTree(tofuse,ftype)
		#print tree.node.call
		kernelout.write(str(tree.node.call))

	
main()


