#include "mreader.h"
#include <string.h>
#include <ctype.h>

//read in a matrix from a tab spaced file 
//breaks horribly in windows  
//gonna have to do it with commas or something

SparseMatrix * bmread(char * input)
{
	SparseMatrix * A = (SparseMatrix*) malloc(sizeof(SparseMatrix));
	printf("loading: %s\n",input);
	FILE * out = fopen(input,"rb");
	if(out == NULL)
	{
		return NULL;
	}
	int i =0;
	fread(&A->valsLength,sizeof(int),1,out);
	fread(&A->colsLength,sizeof(int),1,out);
	fread(&A->indexLength,sizeof(int),1,out);
	printf("got: %d %d %d\n",A->valsLength,A->colsLength,A->indexLength);
	fflush(NULL);
	A->cpu_vals = (TYPE*) malloc(sizeof(TYPE)*A->valsLength);
	A->cpu_cols = (int*) malloc(sizeof(int)*A->colsLength);
	A->cpu_index = (int*) malloc(sizeof(int)*A->indexLength);

	fread(A->cpu_vals,sizeof(TYPE),A->valsLength,out);
	fread(A->cpu_cols,sizeof(int),A->colsLength,out);
	fread(A->cpu_index,sizeof(int),A->indexLength,out);
	fclose(out);
	return A;
}

SparseMatrix * bmpaddedread(char * input, int pad)
{
	SparseMatrix * A = (SparseMatrix*) malloc(sizeof(SparseMatrix));
	printf("loading: %s\n",input);
	FILE * out = fopen(input,"rb");
	if(out == NULL)
	{
		return NULL;
	}
	int padsize;
	fread(&A->valsLength,sizeof(int),1,out);
	fread(&A->colsLength,sizeof(int),1,out);
	fread(&A->indexLength,sizeof(int),1,out);
	padsize = (1+((A->indexLength-1)/pad))*pad + 1;
	printf("padding vector from %d to %d\n",A->indexLength, padsize);
	printf("got: %d %d %d\n",A->valsLength,A->colsLength,A->indexLength);
	fflush(NULL);
	A->cpu_vals = (TYPE*) malloc(sizeof(TYPE)*A->valsLength);
	A->cpu_cols = (int*) malloc(sizeof(int)*A->colsLength);
	A->cpu_index = (int*) malloc(sizeof(int)*padsize);

	fread(A->cpu_vals,sizeof(TYPE),A->valsLength,out);
	fread(A->cpu_cols,sizeof(int),A->colsLength,out);
	fread(A->cpu_index,sizeof(int),A->indexLength,out);
	for(int i = A->indexLength+1; i < padsize; i++)
	{
		A->cpu_index[i] = A->cpu_index[i-1];
	}
	A->indexLength = padsize;
	fclose(out);
	return A;
}

SparseMatrix * bmwrite(char * input, SparseMatrix * A)
{
	FILE  * out = fopen(input,"wb");
	printf("writing a matrix with: %d %d %d\n",A->colsLength,A->colsLength,A->indexLength);
	fwrite(&(A->colsLength),sizeof(int),1,out);
	fwrite(&(A->colsLength),sizeof(int),1,out);
	fwrite(&(A->indexLength),sizeof(int),1,out);
	fwrite(A->cpu_vals,sizeof(TYPE),A->colsLength,out);
	fwrite(A->cpu_cols,sizeof(int),A->colsLength,out);
	fwrite(A->cpu_index,sizeof(int),A->indexLength,out);
	fclose(out);
	return A;
}

SparseMatrix * mread(char* file)
{
	//open input matrix file
	FILE* input = fopen(file,"r");
	
	//make matrix A
	SparseMatrix* A = (SparseMatrix*) malloc(sizeof(SparseMatrix));
	A->colsLength = 2;
	A->valsLength = 2;
	A->indexLength = 2;
	A->cpu_cols = (int*)malloc(sizeof(int)*A->colsLength);
	A->cpu_vals = (TYPE*)malloc(sizeof(TYPE)*A->valsLength); 
	A->cpu_index = (int*)malloc(sizeof(int)*A->indexLength);
	assert(A->cpu_cols);
	assert(A->cpu_vals);
	assert(A->cpu_index);
	
	
	char c;
	int state = 0;
	int i = 0;
	int j = 0;
	char value[128];
	
	perror("setup done");
	c = fgetc(input);
	while(c != EOF)
	{
		//read in first row, it's all integers
		if(c == '\r')
		{
			
		}
		else if(state == 0)
		{
			if(c == '\n' || c == ',')
			{
				//if no space TYPE it up and go
				if(j >=  A->colsLength)
				{
					//perror("WHAT?1");
					A->colsLength += 1000;
					//printf("expanding from %d to %d. j is %d\n",A->colsLength - 100,A->colsLength,j);
					A->cpu_cols = (int*) realloc(A->cpu_cols,sizeof(int)*A->colsLength);
					assert(A->cpu_cols);
					value[i] = '\0';
					//perror("WHAT?2");
					A->cpu_cols[j++] = atoi(value);
					//perror("WHAT?3");
					i = 0;
				}
				//else we are good, just slot the value in
				else
				{
					//perror("good");
					value[i] = '\0';
					A->cpu_cols[j++] = atoi(value);
					i = 0;
				}
				
				if(c == '\n')
				{
				
					A->colsLength = j;
					A->cpu_cols = (int*) realloc(A->cpu_cols,sizeof(int)*A->colsLength);
					assert(A->cpu_cols);
					state++;
					j = 0;
					i = 0;
					perror("Load Cols");
				}
			}
			else
			{
				
				assert(i < 128);
				value[i++] = c;
			}
		}
		//second row is all integers as well.
		else if(state == 1)
		{
			if(c == '\n' || c == ',')
			{
				//if no space TYPE it up and go
				if(j >=  A->indexLength)
				{
					A->indexLength += 1000;
					A->cpu_index = (int*) realloc(A->cpu_index,sizeof(int)*A->indexLength);
					assert(A->cpu_index);
					value[i] = '\0';
					A->cpu_index[j++] = atoi(value);
					memset(value,0,128);
					i = 0;
				}
				//else we are good, just slot the value in
				else
				{
					value[i] = '\0';
					A->cpu_index[j++] = atoi(value);
					i = 0;
					memset(value,0,128);
				}
				
				if(c == '\n')
				{
				
					A->indexLength = j; 
					A->cpu_index = (int*) realloc(A->cpu_index,sizeof(int)*A->indexLength);
					//A->index[j+1] = A->colsLength;
					assert(A->cpu_index);
					state++;
					j = 0;
					i = 0;
					perror("Load Index");
					memset(value,0,128);
				}
			}
			else
			{
				value[i++] = c;
			}		
		}
		else if(state == 2)
		{
			if(c == '\n' || c == ',')
			{
				//if no space TYPE it up and go
				value[i] = '\0';
				//if(j <= 10) printf("%d,%s : %e\n",j,value, atof(value));
				if(j >=  A->valsLength)
				{
					A->valsLength += 1000;
					A->cpu_vals = (TYPE*) realloc(A->cpu_vals,sizeof(TYPE)*A->valsLength);
					assert(A->cpu_vals);
					
					A->cpu_vals[j] = strtod(value,NULL);
					i = 0;
					j += 1;
				}
				//else we are good, just slot the value in
				else
				{
					A->cpu_vals[j] = strtod(value,NULL);
					i = 0;
					j += 1;
				}
				//done, shrink it down to proper size and call it a day
				if(c == '\n')
				{
					A->valsLength = j;
					A->cpu_vals = (TYPE*) realloc(A->cpu_vals,sizeof(TYPE)*A->valsLength);
					assert(A->cpu_vals);
					state++;
					j = 0;
					perror("Load Data");
				}
				if(j <= 10) printf("%d,%s : %.30e\n",j-1,value, A->cpu_vals[j-1]);
			}
			else
			{
				value[i++] = c;
			}		
		}
		else
		{
			printf("DONE!\n");
			break;
		}		
		c = fgetc(input);
	}
	//trim2(A);
	fclose(input);
	printf("cols: %d, vals: %d, index: %d\n",A->colsLength,A->valsLength,A->indexLength);
	return A;

}
int mkill(SparseMatrix * destroy);
