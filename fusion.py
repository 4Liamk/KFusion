from tokens import *
from function import *

def fuse(calls):
	"""fuses a series of function calls"""
	name = calls[0]
	name.value = ""
	name.type = 'ID'
	for call in calls:
		name.value += call.call.value + "_"
	IDS = []
	for call in calls:
		IDS.append(call.ID)
	newfun = functionCall(name)
	IDS = removeDuplicates(IDS)
	args = []
	for call in calls:
		for arg in call.args:
			args.append(arg)
	args = removeDuplicates(args)
	for arg in args:
		newfun.addArg(arg)
	newfun.children = calls
	return newfun


class funfusion:
	"""fuses a series of function calls"""
	fusedcall = ""
	funs = []
	type=''
	def __init__(self,calls,fusiontype="VERTICAL"):
		self.funs = calls
		self.fusedcall = fuse(calls)
		self.type = fusiontype
		print "creating fusion of type: ", fusiontype, self.type
	def construct(filename):
		print "To be implemented"
	def __str__(self):
		string = self.fusedcall.__str__()
		#for fun in self.funs:
		#	string += "\t" + fun.__str__()
		return string	

class fusionTree:
	"""Fusion tree class contains a fusion node, which is the root of the tree.  It can be continually assigned nodes which are added in reverse order of execution.

		Tree then compacts to produce fusion.  It will attempt to compact until it just can't anymore.  In the event  it ends with two nodes in the tree, this is a failure.
		"""
	node = 0
	"""Node"""
	calls = 0
	"""fusion kernel calls"""
	type = 0
	"""type, vertical fusion is assumed"""
	def __init__(self,calls,type='VERTICAL'):
		self.type = type
		self.calls = calls
		self.node = fusionNode(calls[-1])	
		print "Type: ", type
		for call in self.calls:
			print "call: ", call.ID.value
			print "\tInputs:"
			for val in call.getInputs():
				print "\t\t",val.value,"(",
				for p in val.parents:
					print p.value, "," ,
				print ")"
			print "\tOutputs:"
			for val in call.getOutputs():
				print "\t\t",val.value,"(",
				for p in val.parents:
					print p.value, "," ,
				print ")"
		if(type == 'VERTICAL'):
			for call in calls[-2::-1]:
				self.node.addParent(call)
			self.compact()
			
		elif(type == 'HORIZONTAL'):
			self.node = fusionNode(calls[0])	
			self.node.call.args += calls[1].args
			ifstate = Statement(None,1,'IF')
			ifstate.children = []
			state = Statement(None,1,'OTHER')
			state.tokens = []
			state.tokens.append(makeToken('get_global_id(0)','ID'))
			state.tokens.append(makeToken('<','LESSTHAN'))
			state.tokens.append(makeToken(str(calls[0].args[-1][-1].value),"ID"))
			ifstate.children.append(state)
			ifstate.children.append(Statement(None,1,'SCOPE'))
			ifstate.children[1].children = []
			ifstate.children[1].children += calls[0].loads
			ifstate.children[1].children += calls[0].statements
			ifstate.children[1].children += calls[0].stores
			self.node.call.loads = []
			self.node.call.stores= []
			self.node.call.statements = [ifstate]
			elsestate = Statement(None,1,'ELSE')
			elsestate.children.append(Statement(None,1,'SCOPE'))
			elsestate.children[0].children = []
			elsestate.children[0].children += calls[1].loads
			elsestate.children[0].children += calls[1].statements
			elsestate.children[0].children += calls[1].stores
			self.node.call.statements.append(elsestate)
			self.node.call.ID.value = self.node.ID.value
			self.node.call.ID.value += "_" + calls[1].ID.value + "_h"	
			ifstate.replace("get_global_size","-newSize + get_global_size")
			elsestate.replace("get_global_size","-newSize + get_global_size")
			elsestate.replace("get_global_id","-newSize + get_global_id")
			self.node.call.args = removeDuplicates3(self.node.call.args[::-1])[::-1];
			
	def compact(self):
		self.node.ID.value  = ""#self.calls[0].ID.value +  "_" 
		self.node.call.args = []	
		print self.node.call
		for call in self.calls:
			self.node.ID.value += call.ID.value + "_"
			self.node.call.args += call.args
		while(len(self.node.parents) > 0):
			print self.node
			print self.node.call
			if(not(self.node.compact())):
				print self.node
				return
		self.node.call.clean()			
		self.node.call.ID = self.node.ID
		self.node.call.args = removeDuplicates3(self.node.call.args)
		self.node.reorder()


class fusionNode:
	"""Fusion node class.  It allows us to build a tree and then compact it.  It is built and destroyed from the bottom up"""
	parents = []
	"""[Statement]:Parent is a node who's output matches this nodes inputs.  Nodes are added only if dependency is not already filled.  In the case where a dependency is filled, we pass the new node onto the correct parent"""
	call = 0
	"""[Statment]:kernel which is called"""
	args = []
	"""[Statement]:kernel arguments"""
	ID = []
	"""[dirtyToken]: identifiers for various kernels"""
	kernelinvocation = ""
	"""kernelInvocation: out kernel invocation"""
	kernel = ""
	"""string: our kernel name updated as we contract"""
	function = ""
	"""not used"""
	used = []
	"""collection of dependency in the OpenCL kernel which have been satisfied via compacting"""
	inputs = []
	""" This kernels inputs"""
	outputs = []
	""" this kernels outputs"""
	level = 0
	"""indententation for when we output the tree to screen"""
	
	def __init__(self,call, level=0):
		"""kernel call is input and relevant information is extracted to build node"""
		self.level = level
		self.ID = copy.deepcopy(call.ID)
		self.call = copy.deepcopy(call)
		self.args = copy.deepcopy(call.args)
		for statement in self.call.loads + self.call.statements + self.call.stores:
			statement.prefix(str(self.call.cid + str(level)))
		self.parents = []
		self.inputs = self.call.getInputs()
		self.outputs = self.call.getOutputs()
	def addParent(self,call):
		"""call is input,"""
		for p in self.parents:
			if(p.addParent(call)):
				return True
		match = False
		outputs = fusionNode(call,self.level+1).outputs
		#print self.level*"\t"+"Adding to", self.ID.value
		for input in self.inputs:
			for output in outputs:			
				for p1 in input.parents:
					for p2 in output.parents:

						if(p1 == p2):
							match = True
							#print "\tmatch:",output,"==",input
						else:
							pass
							#print "failed:",output,"!=",input
		if(match):
			self.parents.append(fusionNode(call,self.level+1))
			return True
		elif(self.level == 0):
			print "failed to find matching parent cannot build tree.  Kernels Independent.  Will attempt to merge kernels"
			tmp = fusionNode(call,self.level)
			self.call.loads += tmp.call.loads
			self.call.stores += tmp.call.stores
			self.call.statements += tmp.call.statements
			self.inputs = self.call.getInputs()
			self.outputs = self.call.getOutputs()	
			return True		
		else:
			return False
	
	def cullRedeclarations(self):
		decs = []
		for s1 in self.call.loads:
			if s1.type in ['DECLARATION','DECLARATIONASSIGMENT','ALTEREDDECLARATIONASSIGMENT']:
				dec = (s1.tokens[0].value, s1.tokens[1].value)
				if dec in decs:
					s1.tokens[0].value = "/*REDACTED*/"
				else:
					decs.append(dec)
	
	def compact(self):
		"""Compact this node by conrtacting one of its parents.
			This function searches though all parents.  If a given parent satisfies one of its dependencies, it's source code is used to replace the given load function.  Scope is used to prevent similar variables destroying each other
			"""
		for p in self.parents:
			compacted = False		
			for output in p.outputs:
				for statement in self.call.loads:
					#print "matching statment:", statement	
					if(statement.type in ["ASSIGMENT","DECLARATIONASSIGMENT"]):
						for i in range(len(statement.children)):
							child = statement.children[i]
							if(child.search("read_imagef")):
								DID = child.children[0].tokens[0]
								if DID.parents[0] in output.parents:
									compacted = True
									statement.children[i] = Statement(None,child.level,'ID')
									if(output not in statement.children[i].tokens):
										statement.children[i].tokens.append(output)
										statement.update()
										statement.type = 'ALTERED' + statement.type
							elif(child.type not in ['CALL']):
								if(child.tokens[0].type in ['DIRTY ID']):
									DID = child.tokens[0]									
									#print "Checking for output for", DID.value ,i
									if DID.parents[0] in output.parents:
										#print "found match in", output.value
										compacted = True
										statement.children = [Statement(None,child.level,'ID')]
										tmp = copy.deepcopy(output)
										statement.children[0].tokens = [tmp]
										statement.type = 'ALTERED' + statement.type
			if(compacted):			
				scope = Statement(None,1,'SCOPE')
				scope.children = p.call.statements
				for c in scope.children:
					c.level += 1
				self.call.loads = p.call.loads + [scope] + self.call.loads
				#self.call.stores = p.call.stores + self.call.stores	
				for p2 in p.parents:
					p2.level -= 1
				self.parents.remove(p)
				self.parents += p.parents
				self.call.args += p.call.args
				self.call.clean()
				#self.cullRedeclarations()
				print self
				return True
			else:
				print "Could not compact"
				return False	
				
	def reorder(self):
		"""Take any things in the first scope and shuffle them around.  Declarations and asynchronous reads go to the top!"""
		decs = []
		asyncs = []
		assigns = []
		for statement in self.call.loads:
			#move delcarations to the top	
			if statement.type == 'DECLARATION':
				if statement.isMoveable:
					self.call.loads.remove(statement)
					decs.append(statement)
			#move asynchronous operations to just below declarations
			elif statement.type in ['DECLARATION','DECLARATIONASSIGMENT']:
				if(statement.search("async_work_group_copy")):
					if statement.isMoveable:
						self.call.loads.remove(statement)
						asyncs.append(statement)
			"""
			elif statement.type == 'ASSIGMENT':
				if(statement.search("async_work_group_copy")):
					if statement.isMoveable:
						self.call.loads.remove(statement)
						assigns.append(statement)				
			"""
		self.call.loads = decs + asyncs + assigns + self.call.loads;
	
	def __str__(self):
		string  = self.level*"\t" + "__kernel void " + self.ID.value + "("
		for i in range(len(self.call.args)):
			for item in self.call.args[i]:
				string += item.value + " "
			if(i != len(self.call.args)-1):
				string += ","
		string += ")\n"
		for p in self.parents:
			string += str(p)
		return string
