import random
import sys

numCharsToGenerate = int(sys.argv[1])

random.seed(numCharsToGenerate)

try:
    random.seed(int(sys.argv[2]))
except:
    pass

tokendict = ['<', '<<', '>', 
	    '>>', '>&', '|', 
	    '|&', ';', '&', 
	    '&&', '||', '(',
	    ')', 'a', 'b',
	    '\n', '\t', ' ',
	    '"', "'", '1', '2'
	    '""', "''", '\\',
	    '#']

toReturn = ""

for i in xrange(0, numCharsToGenerate):
    toReturn += tokendict[random.randint(0, len(tokendict) - 1)]

print toReturn

