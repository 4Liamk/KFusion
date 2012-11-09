from statements import *

class kernel:
	"""A kernel parsing class for parsing OpenCL kernel code.  It can be parsed from stdin, or it can be created by merging several kernels together"""	
	ID = 0
	"""string:Kernel's name"""
	call = 0
	"""Not used"""
	args = []
	"""The kernels arguments"""
	statements = []
	"""Statements present which are neither load or store operations"""
	loads = []
	"""Load operations.  This are considered input of the kernel"""
	stores = []
	"""Store operations these are considered to be the output of the kernel"""
	count = 0
	cid = 0
	"""used for arguments as they are renaimed to ensure there is no collisions"""
	def __init__(self,tr,kernels=None):
		"""Init function.  Will parse unless given a set of kernels in which case they will be combined.  This method is depreciated as we now use fusion trees to handle kernel fusion"""
		self.args = []
		self.statements = []
		self.loads = []
		self.stores = []
		self.ID = 0
		self.call = 0
		self.count = 0
		if(kernels == None):
			tr.tw()
			self.ID = tr.tw()
			tr.tw()
			arg = []
			while True:
				tok = tr.tw()
				if tok.type == 'COMMA':
					self.args.append(arg)
					arg = []			
				elif tok.type == 'RPAREN':
					self.args.append(arg)
					arg = []
					break
				else:
					arg.append(tok)
					
			while(tok.type != 'LBRACE'):
				tok = tr.tw()		
			state = 1
			statement = ""
			isLoad = False
			isStore = False
			isDirty = False
			isMoveable = True
			statement = Statement(tr,1)
			dList = []
			while(statement.type != 'ENDSCOPE'):
				#self.statements.append(statement)
				if(statement.type == "PRAGMA"):
					text = statement.tokens[0].value.split()
					if(text[1] == "load"):
						isLoad = True
					elif(text[1] == "store"):
						isStore = True
					if(len(text) > 2):
						if(text[2] == "inplace"):
							isMoveable = False				
				elif(statement.type == "DEAD"):
					pass				
				elif(isStore):
					statement.isMoveable = isMoveable
					isMoveable = True
					self.stores.append(statement)
					isStore = False
				elif(isLoad):
					statement.isMoveable = isMoveable
					isMoveable = True
					self.loads.append(statement)
					isLoad = False	
				else:
					statement.isMoveable = isMoveable
					isMoveable = True
					self.statements.append(statement)
				if(isDirty):
					for i in range(len(statement.tokens)):
						if(statement.tokens[i].type == 'ID'):
							dList.append(statement.tokens[i])
				statement = Statement(tr,1)
			for tok in dList:
				for statement in self.loads + self.stores + self.statements:
					statement.dirtyReplace(tok.value,tok.value)
		else:
			self.ID = copy.deepcopy(kernels[0].ID)
			self.ID.value = ""
			
			for kernel in kernels:
				self.loads += kernel.loads
				self.stores += kernel.stores
				self.statements += kernel.statements
				self.ID.value += kernel.ID.value + "_"
				self.args += kernel.args
				
			contaminated = []
			for statement in self.loads:# + self.statements:#  + self.stores:
				contaminated += statement.contaminate()
			
			for statement in self.loads:# + self.statements + self.stores:
				for contaminant in contaminated:
					statement.propogateContamination(contaminant)
			self.loads = removeDuplicates2(self.loads)
			self.stores = copy.deepcopy(kernels[-1].stores)
			self.statements = removeDuplicates2(self.statements)
			self.args = removeDuplicates3(self.args)
								
	def __str__(self):
		string  = "__kernel void " + self.ID.value + "("
		for i in range(len(self.args)):
			for item in self.args[i]:
				string += item.value + " "
			if(i != len(self.args)-1):
				string += ","
		string += ")\n{\n"
		string += "\t//load operations:\n"
		for load in self.loads:
			string += str(load) 
			if(len(load.children) > 0): 
				if(load.type not in ['SCOPE','IF','ELSE']):
					string +=";/*"+load.type +"*/\n" 
			else:
				string +=";/*"+load.type +"*/\n" 
		string += "\t//Main operations:\n"
		for statement in self.statements:
			string += str(statement)
			if(len(statement.children) > 0): 
				if(statement.type not in ['SCOPE','IF','ELSE']):
					string +=";/*"+statement.type +"*/\n" 
			else:
				string +=";/*"+statement.type +"*/\n" 
		string += "\t//Store operations:\n"
		for store in self.stores:
			string += str(store)
			if(len(store.children) > 0): 
				if(store.children[-1].type not in ['SCOPE','IF','ELSE']):
					string +=";/*"+store.type +"*/\n" 
			else:
				string +=";/*"+store.type +"*/\n" 		
		string += "\n}\n"
		return string
	def __eq__(self,other):
		if (self.ID != other.ID):
			return False
		elif(self.count != other.count):
			return False
		else:
			return True
			
	def call(self):
		"""Return the prototype of this kernel"""
		string = ""
		string += self.ret.value + " " + self.ID.value + "("
		for i in range(len(self.args)):
			for tok in self.args[i]:
				string += tok.value + " "
			if(i < len(self.args)-1):
				string += ","
		string += ');\n'		

	def replaceArg(self,i, arg):
		"""replace an argument number i with string arg.
			int: i
			string: arg
			"""
		replaced = self.args[i][-1].value
		self.args[i][-1].value = arg
		contaminated = []
		for statement in self.loads:
			statement.dirtyReplace(replaced,arg,True)
		for statement in self.statements:
			statement.dirtyReplace(replaced,arg,True)
		for statement in self.stores:
			statement.dirtyReplace(replaced,arg,True)
		#self.contaminate()
		
	def contaminate(self):		
		"""Contaminate IDs exactly like with functions.
			Keeps track of data dependencies
			"""		
		while(True):
			contaminated = []
			for statement in self.loads + self.statements:# + self.stores
				contaminated += statement.contaminate()
			if(len(contaminated) <= 0):
				break						
			for statement in self.loads + self.statements + self.stores:
				statement.propogateDown(contaminated)	
				
	def reservedContaminate(self):		
		"""
			currently exactly the same as contaminate.  Main idea is we contaminate less things
			"""		
		self.contaminate()
				
	def removeAssigments(self):
		"""removes any self assigments as they are unnecessary"""
		replaced = []
		print "removing assigments:"
		for statement in self.loads:
			if(statement.type == 'DECLARATIONASSIGMENT'):
				ID = 0
				#print "considering:",statement
				for token in statement.tokens:
					#print token
					if token.type in ['ID','DIRTY ID']:
						ID = token
						break
					else:
						pass
				if(ID is not 0):
					#print "ID is valid"
					if(len(statement.children[0].tokens) == 1 and len(statement.children[0].children) == 0):
						if(statement.children[0].tokens[0].type in ['DIRTY ID']):
							if(ID.value == statement.children[0].tokens[0].value):
								print ID , "==" , statement.children[0].tokens[0]
								self.loads.remove(statement)
								replaced.append((ID,statement.children[0].tokens[0]))
								#print "Added to list"
					else:
						pass
						#print "Rejected ", len(statement.children[0].tokens) 
				else:
					pass
					#print "ID is:",ID
			else:
				replaced += statement.removeAssigments()
			#print ""
			
		for orig,new in replaced:
			if(orig is not 0):
				#print 'replacing',orig,'with',new
				for statement in self.loads + self.stores + self.statements:
					statement.dirtyTokenReplace(orig,new)				
	def clean(self):
		"""General clean function
			we run a single contaminate and clean any self assigments or duplicate instructions
			"""
		self.contaminate()
		#print self
		self.args = removeDuplicates3(self.args)
		#print "duplicated removed: \n", self
		self.removeAssigments()
		#print self
		self.loads = removeDuplicates(self.loads)
		#print self		
		self.stores = removeDuplicates(self.stores)	
		#print self
		self.statements = removeDuplicates(self.statements)
		#print self
		self.cullDuplicateStores()

	def getInputs(self):
		"""return a list of inputs to the kernel.  
			This currently includes any explicit load instructions for arguments
			return:[dirtyIDs]
			"""
		DirtyIDs = []
		for statement in self.loads:	
			if statement.type in ['ASSIGMENT','DECLARATIONASSIGMENT']:
				DirtyIDs += statement.children[0].getDirty()
			else:
				DirtyIDs += statement.getDirty()
		return DirtyIDs

	def getOutputs(self):
		"""return a list of outputs of a kernel.
			return:[dirtyIDs]
			"""
		DirtyIDs = []
		for statement in self.stores:	
			if statement.type in ['ASSIGMENT','DECLARATIONASSIGMENT']:
				for child in statement.children[0].getDirty():
					temp = copy.deepcopy(child)
					temp.parents = [statement.tokens[0]]
					DirtyIDs += [temp]
			else:
				DirtyIDs += statement.getDirty()
		return DirtyIDs
	def cullDuplicateStores(self):
		for store in self.stores[::-1]:
			val = store.strNoChildren()
			for store2 in self.stores[::-1]:
				val2 = store2.strNoChildren()
				if(val == val2):
					print "matching strings", val, val2
					if(store != store2):
						self.stores.remove(store2)
					else:
						print "string is same",store,store2
					
			
			

