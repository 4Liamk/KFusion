from tokens import *

class Statement:
	"""Statement Class:
		Stores a C statement.  They are parsed recursively.  Not all C statements are supported, switch cannot be used.   It may not parse everything exactly as it should and takes a few shortcuts.  It handles loops and scope particularly well though.  
		
		Can be parsed from file or if you pass in a type, it will return you a blank function of that type
		@var:tokens: tokens in statement
		"""
	tokens = []	
	"""Tokens in the statement: lexTok or dirtyToken"""
	children = []
	"""Child statements typically used in control structures:Statement"""	
	level = 0		
	"""indentation level: int"""
	type = ""		
	"""Type of Statement: string dictating the type"""
	end = False
	"""if true this is the last argument in a function call or last statement in a scope"""
	isMoveable = True
	"""If true, this statement may be shuffled around in order to take advantage of asynchronous communication. Default is True"""
	def __init__(self,tr,level=-1,type=None):
		"""
		Init function.  Parses from file if type is not defined, builds statements.
		"""
		self.tokens = []
		self.args = []
		self.level = level+1
		self.children = []
		self.type = ""	
		self.end = False
		if(type == None):
			tok = tr.tw()
			if(tok.type in 'SEMI'):
				self.type = "DEAD"
				return
			while(not(tok.type == 'SEMI' or tok.type == 'RPAREN' or tok.type =='COMMA')):
				if(tok.type in ['COMMENT','CCOMENT','CPPCOMMENT']):
					pass
				else:
					self.tokens.append(tok)		
						
				if(tok.type == 'PRAGMA'):
					self.type = 'PRAGMA'
					break
				elif(tok.type in ['EQUALS', 'TIMESEQUAL', 'DIVEQUAL', 'MODEQUAL', 'PLUSEQUAL', 'MINUSEQUAL','LSHIFTEQUAL','RSHIFTEQUAL', 'ANDEQUAL', 'XOREQUAL', 'OREQUAL']):
					self.type += "ASSIGMENT"
					self.children.append(Statement(tr))
					break
				elif(tok.type == 'LBRACE'):
					self.type = "SCOPE"
					#print self.level*"\t" + "BEGIN SCOPE"
					statement = Statement(tr,self.level)
					while(statement.type not in ['ENDSCOPE']):
						#print statement
						self.children.append(statement)
						statement = Statement(tr,self.level)
					self.children.append(statement)
					#print "END SCOPE"
					break			
				elif(tok.type == 'RBRACE'):
					self.type = 'ENDSCOPE'
					break	
				#if statement
				elif(tok.value == 'if'):
					tr.tw()
					self.type = 'IF'
					self.children.append(Statement(tr))
					self.children.append(Statement(tr,self.level))
					break
				#else statement
				elif(tok.value == 'else'):
					self.type = 'ELSE'
					self.children.append(Statement(tr,self.level))
					break	
				#for loop
				elif(tok.value == 'for'):
					self.type = "FOR"
					#print self.level*"\t","start FOR"
					tr.tw()
					self.children.append(Statement(tr)) #init
					self.children.append(Statement(tr)) #boundry
					self.children.append(Statement(tr)) #increment
					self.children.append(Statement(tr,self.level)) #body
					#print self.level*"\t","end FOR"	
					break
				#parse a while loop
				elif(tok.value == 'while'):
					self.type = 'WHILE'
					tr.tw()
					self.children.append(Statement(tr,self.level)) #expressionDirtyID
					while(Statement(tr,self.level).type == 'ENDARG'):
						print 'ENDARG FOUND, not viable body'
					self.children.append(Statement(tr,self.level)) #body of while			
					break
				
				#check to see if we're calling a function
				elif(tok.type == 'LPAREN'):
					state = 1
					self.type = "SUBEXPR"
					while state > 0:
						tok = tr.tw()
						self.tokens.append(tok)
						if(tok.type == 'LPAREN'):
							state +=1
						elif(tok.type == 'RPAREN'):
							state -=1
					self.children.append(Statement(tr))
					if(self.children[0].end):
						self.end = True
					return
				elif(tok.type == 'ID'):
					if(len(self.tokens) >= 2):
						if self.tokens[len(self.tokens)-2].type == 'ARROW' or self.tokens[len(self.tokens)-2].type == 'PERIOD':
							tok.type = 'BOUNDID'
					if(self.tokens[0].type == "TYPEID"):
						self.type = "DECLARATION"		
					elif(self.type == ""):
						tok2 = tr.tw()
						if(tok2.type == 'LPAREN'):
							self.type = "CALL"
							arg = Statement(tr)
							self.children.append(arg)
							while(arg.end != True):
								arg = Statement(tr)
							 	if(arg.tokens):
									self.children.append(arg)
							self.children.append(Statement(tr))	
							if(self.children[-1].end == True):
								self.end = True
							break
						elif(tok2.type == 'LBRACKET'):
							#self.type = 'INDEX'
							#self.children.append(Statement)
							state = 1
							self.tokens.append(tok2)
							while(state > 0):
								tok2 = tr.tw()
								self.tokens.append(tok2)
								if(tok2.type == 'LBRACKET'):
									state += 1
								elif(tok2.type == 'RBRACKET'):
									state -= 1
							
						else:
							tok = tok2
							continue
				tok = tr.tw()
			#print self,self.type
			if(tok.type == 'RPAREN'):
				self.end = True
			if(tok.type == 'COMMA'):
				self.type = 'COMMA'
				return	
			
		elif(type == "SCOPE"):
			self.type = 'SCOPE'
			return
		elif(type == 'ID'):
			self.type = 'ID'
			return	
		elif(type == 'ASSIGMENT'):
			self.type = type;
		else:
			self.type = type
	def __str__(self):
		string = "\t"*self.level
		if(self.type == "FOR"):
			string += "for("+ str(self.children[0]) + "; " + str(self.children[1]) + "; " + str(self.children[2]) +")\n"
			string += str(self.children[3])
		
		elif(self.type == "WHILE"):
			string += "while(" + str(self.children[0]) + ")\n"
			string += str(self.children[1])

		elif(self.type == "SCOPE"):
			string += "{\n"
			for child in self.children:
				if(child.type != 'DEAD' and child.type != 'ENDSCOPE'):	
					string += str(child)
					if(child.type not in ['IF','ELSE','SCOPE']):	
						string += ';\n'
			string += self.level*"\t" + "}\n" #+ "\t"*(self.level-1) + ""
		elif(self.type == "CAST"):
			string += ''
			for token in self.tokens:
				string += token.value + " "
			#if(len(self.children) > 0):	
			#	string += str(self.children[0])
			string += ') ' 
			if(len(self.children) > 0):	
				string += str(self.children[0])
		elif(self.type == 'IF'):
			string += "if(" + str(self.children[0]) + ")\n"
			string +=  str(self.children[1]) + "\n"
			if(self.children[1].type not in ['IF','ELSE','SCOPE']):	
				string += ';\n'
		elif(self.type == 'ELSE'):
			string += "else\n"+str(self.children[0])
			if(self.children[0].type not in ['IF','ELSE','SCOPE']):	
				string += ';\n'		
		elif(self.type == 'INDEX'):
			for token in self.tokens:
				string += token.value + " "
			string += '['
			string += self.children[0]
			string += ']'
			if(len(self.children[1]) > 0):
				string += self.children[1]			
		elif(self.type == 'SUBEXPR'):
			string += ""
			for tok in self.tokens:
				string += tok.value + ' '
			for child in self.children:
				string += str(child)
				string += ' '
				
			string += ""
		elif(self.type  in ['ASSIGMENT','DECLARATIONASSIGMENT','ALTEREDASSIGMENT','ALTEREDDECLARATIONASSIGMENT']):
			for tok in self.tokens:
				string += tok.value + " "
			for child in self.children:
				string += str(child)
		elif(self.type == 'CALL'):
			for tok in self.tokens:
				string += tok.value + "("
			for i in range(len(self.children)-2):
				string += str(self.children[i]) 
				if(i+1 < len(self.children)):
					if(self.children[i+1].type is not 'OPERATOR'):
						string += ','
			if(len(self.children) > 0):
				string += str(self.children[len(self.children)-2])
			
			string += ")"
			string += str(self.children[-1])		
		else:
			for tok in self.tokens:
				string += str(tok.value) + " "
				for child in self.children:
					string += str(child)
		#string += "/*"+self.type+"*/"
		return string		
	
	def strNoChildren(self):
		string = ""
		for tok in self.tokens:
			string += str(tok.value) + " "
		return string		
	
	def output(self):
		"""Output a tree representing the rough AST"""
		print "\t"*self.level +"<" + self.type + ": ",
		for tok in self.tokens:
			print str(tok.value) + " ",
		print ">"
		for child in self.children:
			child.output()
	
	def search(self,string):
		"""return true if statement contains a given token""" 
		for tok in self.tokens:
			if string == tok.value:
				return True
		for child in self.children:
			if(child.search(string)):
				return True
		return False
	
	def getSub(self,string):
		"""search for a statement which contains a given string and return used.  This is used to extract specific OpenCL function calls."""
		for tok in self.tokens:
			if string == tok.value:
				return self
		for child in self.children:
			val = child.getSub(string)
			if(val is not None):
				return val
		return None		
		
	def prefix(self,value):
		"""Add a prefix to any identifiers in the statement.  This is used to ensure unique function calls have unique variables names"""
		for tok in self.tokens:
			if(tok.type == 'DIRTY ID' and not tok.isArg):
					tok.ID += "_"+ value
					tok.update()	
		for child in self.children:
			child.prefix(value)
				
	def replace(self,search,replace):
		"""Find and replace any identifiers which match 'search' with 'replace'\n"""
		for child in self.children:
			child.replace(search,replace) 
		for tok in self.tokens:
			if search == tok.value and tok.type == 'ID' and tok.value not in protectedWords:
				tok.value = replace
				
	#this replace changes the token into a dirty token.  This will be propagated through the code
	def dirtyReplace(self,search,replace,isArg=False):
		"""
			Same as replace, but instead replaces the token with dirty tokens.  Will not replace an argument.  This dirty replacement will propogate through the code
			@param:search: String to find and replace
			@type:search: string
			@param:replace: string to replace with
			@type:replace: string
			"""
		for child in self.children:
			child.dirtyReplace(search,replace,isArg) 
		for i in range(len(self.tokens)):
			if search == self.tokens[i].value and (self.tokens[i].type == 'ID' or self.tokens[i].type == 'DIRTY ID')  and self.tokens[i].value not in protectedWords:
				#print "replacing: " + self.tokens[i].value + " with " + replace
				self.tokens[i] = dirtyToken(self.tokens[i])
				self.tokens[i].value = replace
				self.tokens[i].type = "DIRTY ID"
				self.tokens[i].parents.append(self.tokens[i])
				self.tokens[i].isArg = isArg

	def dirtyTokenReplace(self,search,replace,isArg=False):
		"""replace a token with value=search with dirty token replace"""
		for child in self.children:
			child.dirtyTokenReplace(search,replace) 
		for i in range(len(self.tokens)):
			if search.value == self.tokens[i].value:
				if (self.tokens[i].type == 'ID' or self.tokens[i].type == 'DIRTY ID')  and self.tokens[i].value not in protectedWords:
					self.tokens[i] = replace
					self.tokens[i].isArg = isArg
	def getDirty(self):
		"""recursively get all dirty tokens in a statement and their children"""
		dirtyIDs = []
		"""dirty Id's"""
		for child in self.children:
			dirtyIDs += child.getDirty()
		for tok in self.tokens:
			if tok.type == 'DIRTY ID':
				dirtyIDs.append(tok)
		return dirtyIDs
	def contaminate(self):
		"""Propogate dependencies via a 'contamination' mechanism.  A variable is contaminated when it is assigned to by a dirty token  Contaminated tokens spread down throughout the code"""
		contaminated = []
		"""List of newely contaminated tokens"""
		for child in self.children:
				contaminated += child.contaminate()			
		if(self.type in ['ASSIGMENT','DECLARATIONASSIGMENT']):
			dirtyIDs = []
			for child in self.children:
				dirtyIDs += child.getDirty()
			#for DID in dirtyIDs:
			#	print DID
			if(len(dirtyIDs) <= 0):
				return []
			else:						
				for i in range(len(self.tokens)):
					if(self.tokens[i].type == 'ID' or self.tokens[i].type == 'DIRTY ID'):	
						if self.tokens[i].type == 'ID':	
							self.tokens[i] = dirtyToken(self.tokens[i])
							self.tokens[i].parents = []
						if(not(self.tokens[i].isArg)):		
							contamination = False	
							for DID in dirtyIDs:
								for ID in DID.parents:
									if(ID not in self.tokens[i].parents):
										print "adding parent " + str(ID) + " to " + str(self.tokens[i])
										self.tokens[i].addParent(ID)
										contamination = True
								
							if(contamination):
								contaminated.append(self.tokens[i])
		subexpr = self.getSub('async_work_group_copy')
		if(subexpr):
			print subexpr
			for i in range(len(subexpr.children[0].tokens)):
				if subexpr.children[0].tokens[i].type == 'ID':
					subexpr.children[0].tokens[i] = dirtyToken(subexpr.children[0].tokens[i])
				if subexpr.children[0].tokens[i].type == 'DIRTY ID':
					if(subexpr.children[1].tokens[0] not in subexpr.children[0].tokens[i].parents):
						subexpr.children[0].tokens[i].addParent(subexpr.children[1].tokens[0])
						contaminated.append(subexpr.children[0].tokens[i])
		return contaminated	
		
	def propogateDown(self, contaminates):
		"""Back propogate contamination to ensure variable names don't change.  
			This also allows us to see all variables which effect another one.  
			Works well within a given function, when we combine functions this can raise problems."""
		for child in self.children:
			isDone = child.propogateDown(contaminates)
		for contaminate in contaminates:
			for i in range(len(self.tokens)):
				if(self.tokens[i].type == 'ID' or self.tokens[i].type == 'DIRTY ID'):
					match = False
					if(self.tokens[i].type == 'ID'):
						if(self.tokens[i].value == contaminate.ID):
							match = True
							gmatch = True
							self.tokens[i] = dirtyToken(self.tokens[i])
					elif(self.tokens[i].type == "DIRTY ID"):
						if(self.tokens[i].ID == contaminate.ID):
							match = True
							gmatch = True
					if(match):
						#print "match:",self.tokens[i].value,contaminate.ID
						for parent in contaminate.parents:
							contained = False
							for myparent in self.tokens[i].parents:
								if(parent.value == myparent.value):
									contained = True
							if(not contained):
								self.tokens[i].addParent(parent)	
	
	def propogateContamination(self,contaminates):
		"""Back propogate contamination to ensure variable names don't change.  
			This also allows us to see all variables which effect another one.  
			Works well within a given function, when we combine functions this can raise problems."""
		if(self.type not in ['DECLARATION','DECLARATIONASSIGMENT','ALTEREDDECLARATIONASSIGMENT']):
			for child in self.children[::-1]:
				isDone = child.propogateContamination(contaminates)
				if(isDone):
					contaminates.remove(contaminate)

		for contaminate in contaminates:
			gmatch = False
			for i in range(len(self.tokens)):
				if(self.tokens[i].type == 'ID' or self.tokens[i].type == 'DIRTY ID'):
					match = False
					if(self.tokens[i].type == 'ID'):
						if(self.tokens[i].value == contaminate.ID):
							match = True
							gmatch = True
							self.tokens[i] = dirtyToken(self.tokens[i])
					elif(self.tokens[i].type == "DIRTY ID"):
						if(self.tokens[i].ID == contaminate.ID):
							match = True
							gmatch = True
					if(match):
						print "match:",self.tokens[i].value,contaminate.ID
						for parent in contaminate.parents:
							contained = False
							for myparent in self.tokens[i].parents:
								if(parent.value == myparent.value):
									contained = True
							if(not contained):
								self.tokens[i].addParent(parent)
			if(gmatch and self.type in ['DECLARATION','DECLARATIONASSIGMENT']):
				print "\tstatement is declaration ending dontamination"
				contaminates.remove(contaminate)
		if(self.type in ['ASSIGMENT','ALTEREDASSIGMENT']):
			for tok in self.tokens:
				if(tok.type == 'DIRTY ID'):
					if(tok not in contaminates):
						contaminates += [tok]
					
	def __eq__(self,other):
		return str(self) == str(other)
		
	def update(self):
		for tok in self.tokens:
			if tok.type == 'DIRTY ID':
				tok.update
		for child in self.children:
			child.update()
		
	def removeAssigments(self):
		"""We remove self assigments as they can occure when we fuse 2 kernels with similar operations."""
		replaced = []
		for child in self.children:
			if(child.type == 'DECLARATIONASSIGMENT'):
				ID = 0
				for token in child.tokens:
					if token.type in ['DIRTYID','ID']:
						ID = token
				if(len(child.children) == 1):
					if(len(child.children[0].tokens) == 1):
						if(child.children[0].tokens[0].type in ['DIRTYID','ID'] and len(child.children[0].children) == 0):
							self.children.remove(child)
							replaced.append((ID, child.children[0].tokens[0]))
			else:
				replaced += child.removeAssigments()
		return replaced		

