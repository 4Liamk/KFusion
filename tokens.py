import sys
import ply.ctokens
import ply.lex as lex
import copy

tokens = [
    # Literals (identifier, integer constant, float constant, string constant, char const)
    'ID', 'TYPEID', 'ICONST', 'FCONST', 'SCONST', 'CCONST','FLOAT','INTEGER','CHARACTER','STRING',

    # Operators (+,-,*,/,%,|,&,~,^,<<,>>, ||, &&, !, <, <=, >, >=, ==, !=)
    'PLUS', 'MINUS', 'TIMES', 'DIVIDE', 'MOD',
    'OR', 'AND', 'NOT', 'XOR', 'LSHIFT', 'RSHIFT',
    'LOR', 'LAND', 'LNOT',
    'LT', 'LE', 'GT', 'GE', 'EQ', 'NE',

    # Assignment (=, *=, /=, %=, +=, -=, <<=, >>=, &=, ^=, |=)
    'EQUALS', 'TIMESEQUAL', 'DIVEQUAL', 'MODEQUAL', 'PLUSEQUAL', 'MINUSEQUAL',
    'LSHIFTEQUAL','RSHIFTEQUAL', 'ANDEQUAL', 'XOREQUAL', 'OREQUAL',

    # Increment/decrement (++,--)
    'PLUSPLUS', 'MINUSMINUS',

    # Structure dereference (->)
    'ARROW',

    # Ternary operator (?)
    'TERNARY',

    # Delimeters ( ) [ ] { } , . ; :
    'LPAREN', 'RPAREN',
    'LBRACKET', 'RBRACKET',
    'LBRACE', 'RBRACE',
    'COMMA', 'PERIOD', 'SEMI', 'COLON',

    #other non identifies:  
    'IF','ELSE','SWITCH','FOR',

    # Ellipsis (...)
    'ELLIPSIS', 'COMMENT',

    # hash
    'HASH','COLONCOLON',
    #pragma #define
    'PRAGMA', 'DEFINE',
    # comment stuff
    'CPPCOMMENT','CCOMMENT',
]

protectedWords = ['cl_int','__local','event_t','auto','break', 'case','continue','default','di','register','return','short', 'sizeof','static','struct','typedef','union','volatile','while', 'cl_uint','for','while','if','else','switch','Image','ConvMatrix', 'const','unsigned','void','__global']

constants = ['ICONST', 'FCONST', 'SCONST', 'CCONST','FLOAT','INTEGER','CHARACTER','STRING']

operator = ['EQUALS', 'TIMESEQUAL', 'DIVEQUAL', 'MODEQUAL', 'PLUSEQUAL', 'MINUSEQUAL', 'LSHIFTEQUAL', 'RSHIFTEQUAL', 'ANDEQUAL', 'XOREQUAL', 'OREQUAL']
# Operators
t_PLUS             = r'\+'
t_MINUS            = r'-'
t_TIMES            = r'\*'
t_DIVIDE           = r'/'
t_MOD              = r'%'
t_OR               = r'\|'
t_AND              = r'&'
t_NOT              = r'~'
t_XOR              = r'\^'
t_LSHIFT           = r'<<'
t_RSHIFT           = r'>>'
t_LOR              = r'\|\|'
t_LAND             = r'&&'
t_LNOT             = r'!'
t_LT               = r'<'
t_GT               = r'>'
t_LE               = r'<='
t_GE               = r'>='
t_EQ               = r'=='
t_NE               = r'!='
t_HASH		   = r'\#'
t_COLONCOLON 	   = r'::'
# Assignment operators

t_EQUALS           = r'='
t_TIMESEQUAL       = r'\*='
t_DIVEQUAL         = r'/='
t_MODEQUAL         = r'%='
t_PLUSEQUAL        = r'\+='
t_MINUSEQUAL       = r'-='
t_LSHIFTEQUAL      = r'<<='
t_RSHIFTEQUAL      = r'>>='
t_ANDEQUAL         = r'&='
t_OREQUAL          = r'\|='
t_XOREQUAL         = r'^='

t_DEFINE	   = r'\#define .*\n'
t_PRAGMA	   = r'\#pragma .*\n'

# Increment/decrement
t_PLUSPLUS        = r'\+\+'
t_MINUSMINUS        = r'--'

# ->
t_ARROW            = r'->'

# ?
t_TERNARY          = r'\?'

# Delimeters
t_LPAREN           = r'\('
t_RPAREN           = r'\)'
t_LBRACKET         = r'\['
t_RBRACKET         = r'\]'
t_LBRACE           = r'\{'
t_RBRACE           = r'\}'
t_COMMA            = r','
t_PERIOD           = r'\.'
t_SEMI             = r';'
t_COLON            = r':'
t_ELLIPSIS         = r'\.\.\.'
t_TYPEID	   = r'int[2-8]*\**|TYPE[2-4]*|float[2-8]*\**|void\**|double\**|char\**|size_t\**|Image\**|convMatrix\**|unsigned int\**|unsigned\**|cl_int\**|cl_uint\**|cl_mem\**|__local\**|event_t\**|TYPE\**'
t_IF 		   = r'if'
t_ELSE	   	   = r'else'
t_SWITCH	   = r'switch'
t_FOR		   = r'for'
#t_ignore = " \t"
#t_SPACE = " "
#t_TAB = "\t"

def t_error(t):
    t.type = t.value[0]
    t.value = t.value[0]
    t.lexer.skip(1)
    print t
    return t

outfile = ""

def t_space(t):
    r'[ \t\r]'
    global outfile
    outfile.write(t.value)


def t_newline(t):
    r'\n+'
    t.lexer.lineno += t.value.count("\n")
    outfile.write(t.value)

# Identifiers
t_ID = r'[A-Za-z_][A-Za-z0-9_]*'

# Integer literal
t_INTEGER = r'\d+([uU]|[lL]|[uU][lL]|[lL][uU])?'

# Floating literal
t_FLOAT = r'((\d*)(\.\d+)(e(\+|-)?(\d+))? | (\d+)e(\+|-)?(\d+))([lL]|[fF])?'

# String literal
t_STRING = r'\"([^\\\n]|(\\.))*?\"'

# Character constant 'c' or L'c'
t_CHARACTER = r'(L)?\'([^\\\n]|(\\.))*?\''

#def t_ERROR(e):
#	print "error:" , e
#	return e

# Comment (C-Style)
def t_COMMENT(t):
    r'/\*(.|\n)*?\*/'
    t.lexer.lineno += t.value.count('\n')
    return t

# Comment (C++-Style)
def t_CPPCOMMENT(t):
    r'//.*\n'
    t.lexer.lineno += 1
    return t

def makeToken(value,type):
	"""Helper function to quickly make a token"""
	tok = lex.LexToken();
	tok.value = value
	tok.type = type
	return tok	


def removeDuplicates(inlist):
	"""Functions which remove duplicates from list"""
	outlist = []
	for item in inlist:
		if item not in outlist:
			outlist.append(item)
	return outlist
	
def removeDuplicates2(inlist):
	"""This ones uses a direct comparison"""
	outlist = []
	for item in inlist:
		contained = False
		for item2 in outlist:
			if item == item2:
				contained = True
		if not contained:
			outlist.append(item)
	return outlist	

def toktostr(tokens):
	"""Converts a token to a string"""
	string = ""
	for tok in tokens:
		string += tok.value + " "
	return string
	
def removeDuplicates3(inlist):
	"""Removes Duplicates in list by comparing all tokens.  Only works with statements"""
	outlist = []
	for items in inlist:
		contained = False
		for items2 in outlist:
			if toktostr(items) == toktostr(items2):
				contained = True
		if not contained:
			outlist.append(items)
	return outlist	

class dirtyToken:
	"""" Class: dirtyToken Upgraded token class used to track dependency information. This is done so when we combine two of exactly the same function, they don't annihilate each other when we remove duplicate intructions
		variables:parents parent identifiers of dirtyToken
		"""
	parents = []
	value = 0	
	type = 0 	
	token = 0
	ID =  0 
	isArg = False
	def __init__(self, token, parent=None):
		"""Create a dirty token from the original"""
		self.parents = []
		self.token = token
		self.value = copy.deepcopy(token.value)
		self.type = 'DIRTY ID'
		self.ID = copy.deepcopy(token.value)
		#print "building dirty id, original:",self.value,parent
		if(parent is not None):
			self.addParent(parent)
	def addParent(self,parent):
		"""Add a Parent Dependency.  This changes the token value adding the parents id to it"""
		self.parents.append(parent)
		self.value = copy.deepcopy(self.ID)
		self.parents = removeDuplicates(self.parents)
		self.parents.sort(key=dirtyToken.__str__)
		for parent in self.parents:
			self.value += "_" + parent.value
		#print "token is now:",self.value
	def update(self):
		self.value = copy.deepcopy(self.ID)
		self.parents.sort(key=dirtyToken.__str__)
		for parent in self.parents:
			self.value += "_" + parent.value
		#print "token is now:",self.value	
		
	def __str__(self):
		"""Tostring method"""
		string = '[' + str(self.value) + ',' + str(self.type) + ',(' 
		for par in self.parents:
			string += par.value + ","
		
		string += '),'+ ']'
		return string
	def __eq__(self,other):
		"""equivalent test, if values match we are done here.  Line numbers are irrelevant.  We're basically checking that they're the same identify"""
		if(self.value == other.value):
			return True
		else:
			return False
			
class TokenReader:
	lexer=0
	replacements = []
	outfile = 0

	def __init__(self,input,replacements,outfile2):
		global outfile
		outfile = outfile2
		self.lexer = lex.lex()
		lex.input(open(input).read())
		self.replacements = replacements
		self.outfile = outfile2
	
	def tw(self):
		"""parse a token and immediately write it to file"""
		tok = self.lexer.token()
		if tok:
			if tok.value in self.replacements:
				self.outfile.write(self.replacements[tok.value])
			else:
				self.outfile.write(tok.value)			
		return tok
	
	def token(self):
		return self.lexer.token()

def setOutput(out):
	global outfile
	outfile = out	
		
def isProtected(word):
	return word in protectedWords	
