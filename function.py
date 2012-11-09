from statements import *
from kernelInvocation import *
class function:
	"""function class.  Stores all the statements in a function ordering them into four categories.  We can also input a series of functions which will then be fused."""
	ID = ""
	"""ID of function, basically the name"""
	ret = ""
	"""return value, not used.  Really assumed to be void"""
	args = []
	"""[Statement] to function.  We combine them all if we fuse"""
	clargs = []
	"""[Statement]: kernel argument statements which set the arguments of the kernel"""
	clkernels = []
	"""[Statement]: kernel enqueus"""
	preStatements = []
	"""[Statement]: statement before any opencl related calls"""
	postStatements = []
	"""[Statement]: statement which occure after the opencl calls.  Generally they take care of cleanup"""
	kernelInvocations = []
	"""[kernelInvocation]: set of kernel invocation objects"""
	newKernel = ""
	"""string:if we fused, this contains a string with the name of the new kernel"""
	type = 0
	"""if it was created via fusion or not"""
	ftype=''
	"""Vertical or horizontal fusion"""
	def __init__(self,ret,ID,tr,calls=None,type='VERTICAL'):
		self.ftype = type
		if(calls is None):
			self.ID = ID
			self.ret = ret
			self.args = []
			self.clargs = []
			self.clkernels = []
			clStatements = []
			self.preStatements = []
			self.postStatements = []			
			#print "\n\nStarting to parse top level statements in function:",ID			
			arg = []	
			while True:
				tok = tr.tw()
				
				if tok.type == 'COMMA':
					self.args.append(arg)
					#print "argument: ", arg
					arg = []			
				elif tok.type == 'RPAREN':
					self.args.append(arg)
					#print "argument: ", arg
					arg = []
					break
				else:
					arg.append(tok)
			while(tok.type != 'LBRACE'):
				tok = tr.tw()		
			state = 0

			statement = Statement(tr,1)
			while(statement.type != 'ENDSCOPE'):
				#print statement
				#print "obatined statement"
				if(statement.search("clEnqueueNDRangeKernel")):
					self.clkernels.append(statement)
					state = 2
				elif(statement.search("clSetKernelArg")):
					self.clargs.append(statement)	
					state = 2		
				elif(state == 0):
					self.preStatements.append(statement)
				elif(state == 2):
					self.postStatements.append(statement)
				statement = Statement(tr,1)
				while(statement.type == "DEAD"):
					statement = Statement(tr,1)
			#print "function done"
		else:
			print "Fusing Type: "
			self.type = 1
			self.ID = copy.deepcopy(calls[0].ID)
			self.ID.value = ""
			self.ret = calls[0].ret
			self.args = []
			self.clargs = []
			self.clkernels = []
			self.preStatements = []
			self.postStatements = []
			self.clStatements = []
			self.kernelInvocations = []
			gsizes = [] 
			for call in calls:
				"""Create a series of invocations"""
				c1 = []
				#print "adding to call :", call.ID.value
				self.ID.value += call.ID.value + "_"
				self.args += call.args;
				self.clargs += call.clargs
				self.clkernels += call.clkernels
				for kernel in call.clkernels:
					self.kernelInvocations.append(KernelInvocation(kernel,call.clargs))
					gsizes.append(kernel.children[0].children[4])
				self.preStatements += call.preStatements
				self.postStatements += call.postStatements
			self.args = removeDuplicates3(self.args)
			self.clargs = removeDuplicates2(self.clargs)
			self.clkernels = removeDuplicates2(self.clkernels)			
			self.preStatements = removeDuplicates2(self.preStatements)
			self.postStatements = removeDuplicates2(self.postStatements)
			
			"""combine them into a single invocation"""
			newinvocation = KernelInvocation(None,None,self.kernelInvocations,type)
			self.kernelInvocations.append(newinvocation)

			"""if we have a horizonatal, we need to do some extra work"""
			if(type == 'HORIZONTAL'):
				newstatement = Statement(None,1,'DECLARATION')
				newstatement.tokens = []
				newstatement.tokens.append(makeToken('size_t','TYPEID'));

				tok = lex.LexToken()
				tok.type = 'ID'
				tok.value = 'newGlobalSize'
				newinvocation.kernel.children[4].tokens[0] = tok;
				newstatement.tokens.append(tok);

				tok = lex.LexToken()
				tok.type = 'INDEX'
				tok.value = '[' + str(newinvocation.kernel.children[2]) +']'
				newstatement.tokens.append(tok);			
				dim = int(str(newinvocation.kernel.children[2]))
				self.preStatements.append(newstatement)
									
				for i in range(dim):
					newstat = Statement(None,1,'ASSIGMENT')
					newstat.tokens = []
					newstat.tokens.append(makeToken('newGlobalSize','ID'))
					newstat.tokens.append(makeToken('[' + str(i) +']','INDEX'))
					newstat.tokens.append(makeToken('=','EQUALS'))
					child = Statement(None,0,'OTHER')
					child.tokens = []
					tok= lex.LexToken()
					tok.type ='PLUS'
					tok.value = '+'
					for cl in self.clkernels:
						tmp = cl.getSub("clEnqueueNDRangeKernel")
						child.tokens += tmp.children[4].tokens
						child.tokens.append(makeToken('[' + str(i) +']','INDEX'))
						child.tokens.append(tok)
					child.tokens[-1] = makeToken(';','SEMICOLON')
					newstat.children.append(child)
					self.preStatements.append(newstat)
					tmp = self.clkernels[0].getSub("clEnqueueNDRangeKernel")
				newinvocation.args.append(('sizeof(size_t)',"&"+str(tmp.children[4]) + "[0]")) 
				newinvocation.kernel.children[4].tokens = [makeToken("newGlobalSize",'ID')]
			self.newKernel = str(newinvocation.kernel.children[1])	

	def __str__(self):
		string = ""
		string += self.ret.value + " " + self.ID.value + "("
		for i in range(len(self.args)):
			for tok in self.args[i]:
				string += tok.value + " "
			if(i < len(self.args)-1):
				string += ","
		string += ')\n{'
		
		string += "\n\t//pre opencl statements\n"
		
		for statement in self.preStatements:
			string += str(statement)
			if(statement.type != 'SCOPE'):
				string += ";/*"+statement.type +"*/\n"
		
		string += "\n\t//Set Kernel Arguments\n"
		if(self.type == 0):
			for arg in self.clargs:
				string +=str(arg)+ ";/*"+statement.type +"*/\n"
		
			string += "\n\t//Call Kernels\n"	
			for kernel in self.clkernels:
				string +=str(kernel) + ";/*"+statement.type +"*/\n"
		if(self.type == 1):
			string += str(self.kernelInvocations[-1])
			
		string += "\n\t//clean up\n"
		for statement in self.postStatements:
			string +=str(statement)		
			if(statement.type != 'SCOPE'):
				string += ";/*"+statement.type +"*/\n"
		string += "\n}\n"			
		return string

	def call(self):
		"""return the function call"""
		string = ""
		string += self.ret.value + " " + self.ID.value + "("
		for i in range(len(self.args)):
			for tok in self.args[i]:
				string += tok.value + " "
			if(i < len(self.args)-1):
				string += ","
		string += ');\n'	
		return string	

	def replaceArg(self,i, arg):
		"""replace an argument and the propogate the replacement as Dirty Tokens
			@param: i: argument number i
			@parem: arg: the replacement argument string
			"""
		replaced = self.args[i][-1].value
		self.args[i][-1].value = arg
		for statement in self.preStatements + self.postStatements + self.clkernels + self.clargs:
			statement.dirtyReplace(replaced,arg,True)
			
	def contaminate(self):
		"""
			contaminate all statements.  
			We contaminate all the pre statements as they are assigned from arguments
			we then propogate these contaminated IDs throughout the area
			"""
		while(True):
			#print self
			contaminated = []
			for statement in self.preStatements: #self.postStatements + self.clkernels + self.clargs:# + self.stores
				results = statement.contaminate()
				contaminated += results
			for statement in self.preStatements + self.postStatements + self.clkernels + self.clargs:
				for contaminant in contaminated:
					statement.propogateDown(contaminated)
												
			if(len(contaminated) <= 0):
				break	
				
	def reservedContaminate(self):				
		"""
			I don't think this would even work here.  It is a function which belongs within the kernel class.  Will explode if called
		"""
		while(True):
			contaminated = []
			for statement in self.loads + self.statements:# + self.stores
				contaminated += statement.contaminate()
			
			for statement in self.loads + self.statements + self.stores:
				for contaminant in contaminated:
					statement.propogateContamination(contaminant)
			if(len(contaminated) <= 0):
				break
							
class functionCall:
	"""Class for function calls within the main fall.  Not complicated, but designed to keep track of various calls with carious arguments.  Fused using the fuse function"""
	ID = -1
	call = ""
	args = []
	children = []
	def __init__(self,fun):
		self.call = fun
		self.args = []
	def addArg(self,arg):
		self.args.append(arg)
	def addChildren(self,children):
		self.children = children
	def __str__(self):
		string = ""
		if(self.ID != -1):
			string = self.ID.value + "->" + self.call.value + "("
			for i in range(len(self.args)-1):
				string += self.args[i].value + ", "
			if(len(self.args) > 0): 
				string += self.args[len(self.args)-1]
			string += ");\n"
		else:
			string =  self.call.value + "("
			for i in range(len(self.args)-1):
				string += self.args[i] + ", "
			if(len(self.args) > 0):
				string += self.args[len(self.args)-1]
			string += ");\n"
		return string
