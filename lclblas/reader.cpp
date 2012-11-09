#include "reader.h"
//reads in kernels - very simple doesn't do a whole lot
char* readin(char* name)
{
	int rlen = 128;
	int i = 0;
	char* rturn = (char*) malloc(rlen); 
	FILE* input = fopen(name,"r");
	while(!feof(input))
	{
		if(i >= rlen)
		{
			rlen = rlen*2;
			rturn = (char*) realloc(rturn, rlen);
		}
		rturn[i] = fgetc(input);
		i++;	
	}
	rturn =  (char*) realloc(rturn, i+2);
	rturn[i] = '\0';
	rturn[i-1] = '\0';
	return  rturn;
}
