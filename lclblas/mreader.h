#ifndef MREADER_H
#define MREADER_H
#include <stdio.h>
#include "sparse.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
SparseMatrix * mread(char* file);
SparseMatrix * bmread(char* file);
SparseMatrix * bmwrite(char * file, SparseMatrix * A);
int mkill(Matrix * destroy);
SparseMatrix * bmpaddedread(char * input, int pad);
#endif

