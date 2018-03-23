#ifndef MATRIS
#define MATRIS

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

void printMatris(double** matris,int r,int c);
void freeMatris(double** matris,int r);
double** generateMatris(int r,int c);
double** allocMatris(int r,int c);
void writeMatris(double** matris,int r,int c,FILE* file);
void multiple(double** a,int m,int n,double **b,int p,int q,double **mult);
void Transpose(double **a,double** t , int r ,int c);
void inverse(double** a,double** inv,int m,int n);
void CoFactor(double **a,int m,int n,double **b); // https://www.cs.rochester.edu/~brown/Crypto/assts/projects/adj.html
void multiplyMatris(double** matris,int m,int n ,double mult);
double det(double** m,int r,int c);//http://easy-learn-c-language.blogspot.com.tr/2013/02/numerical-methods-determinant-of-nxn.html
double** pseudoInverse(double** a,double** b,int m,int n,double** sudoInverse);
void Diff(double** a,double **b,int m,int n,double **diff);


/*
double det(double** matrix,int n);
double det(double **a,int n);
void copyMatris(double** mat1,double mat2[20][20],int n);
void CoFactor(double **a,int n,double **b);
void shiftedInverse(double** a,double** shifted,int n);
void fillMatris(double** matris,double** sub,int m,int n,int a,int b);
void randMatris(double **a,int n,int r);
void inverse(double** a,double** inv,int n);
void getSubMatris(double** matris,double** sub,int m,int n,int a,int b);
void convolution(double** in,int a,double** kernel,int b,double** out);
void multiplyMatris(double** matris,int n,double mult);*/

#endif
