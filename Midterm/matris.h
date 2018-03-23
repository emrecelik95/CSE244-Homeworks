#ifndef MATRIS
#define MATRIS
#endif

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

double det(double** matrix,int n);
void printMatris(double** matris,int n);
void freeMatris(double** matris,int n);
double det(double **a,int n);
double** generateMatris(int n);
double** allocMatris(int n);
void copyMatris(double** mat1,double mat2[20][20],int n);
void CoFactor(double **a,int n,double **b);
void Transpose(double **a,int n);
void shiftedInverse(double** a,double** shifted,int n);
void fillMatris(double** matris,double** sub,int m,int n,int a,int b);
void randMatris(double **a,int n,int r);
void inverse(double** a,double** inv,int n);
void getSubMatris(double** matris,double** sub,int m,int n,int a,int b);
void convolution(double** in,int a,double** kernel,int b,double** out);
void writeMatris(double** matris,int n,FILE* file);
void multiplyMatris(double** matris,int n,double mult);