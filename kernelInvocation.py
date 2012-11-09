import copy
from tokens import *
class KernelInvocation:
	"""A kernel invocation keeps track of queueing a kernel and all the arguments which are called before.  
		We use this when fusing kernels in order to give the combined arguments the correct order.  
		This is where we fuse the kernel call at the library level
		
		It can be created with a given kernel and a set of args or a combination of invocation 
		"""
	args = []
	"""
		OpenCL kernel arguments a tuple:(argument statement,sizeof statement)
		"""
	kernel = ""
	"""
		Kernel statement 
		"""
	def __init__(self,kernel,args,invocations=None,type='VERTICAL'):
		"""Init function.  Extracts the relevant 
			information from the various arguments and creates an invocation object
			@param: kernel: kernel statement or a statement which contains a kernel statement.  It will be extracted either way
			@param: args: list of argument statements.  The actually statement will be extracted.
			@param: invocations: a list of kernel invocations.  If not None, it will be used to fuse kernel calls into a single call.
			"""
		self.args = []
		self.kernel = ""
		if(invocations == None):
			temp = kernel.getSub("clEnqueueNDRangeKernel")
			if(temp is not None):
				self.kernel = temp
			else:
				print "Invalid kernel statement"
			for arg in args:
				temp = arg.getSub("clSetKernelArg")
				if(temp is not None):
					if(temp.children[0] == self.kernel.children[1]):
						self.args.append((temp.children[2],temp.children[3]))
				else:
					print "invalid kernel argument"
		else:
			self.kernel = copy.deepcopy(invocations[0].kernel)
			self.kernel.children[1].tokens[0].value = ""
			for invocation in invocations:
				self.kernel.children[1].tokens[0].value += invocation.kernel.children[1].tokens[0].value.split('_')[0] + "_"
				self.args += invocation.args
			self.kernel.children[1].tokens[0].value += "kernel"
			if(type == 'HORIZONTAL'):
				"""Append an h"""
				self.kernel.children[1].tokens[0].value +='h'	
			self.args = removeDuplicates2(self.args)

	def __str__(self):
		"""print out all arguments makign new statements and ording the arguments correctly.  Then outputting the enqueue kernel"""
		string = ""
		for i in range(len(self.args)):
			(type,name) = self.args[i]
			string += "\tclSetKernelArg(" + self.kernel.children[1].tokens[0].value + "," + str(i) + "," + str(type) + "," + str(name) + ");\n"
		string += "\t"+str(self.kernel)	+ ";\n"
		return string
