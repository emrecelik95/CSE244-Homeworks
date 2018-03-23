#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "matris.h"


void printMatris(double** matris,int n){
	int i,j;
	for(i = 0 ; i < n ; ++i){
		for(j = 0 ; j < n ; ++j)
			fprintf(stdout,"%.3f\t",matris[i][j]);
		fprintf(stdout,"\n");
	}
}


void writeMatris(double** matris,int n,FILE* file){
	int i,j;
	fprintf(file,"[");
	for(i = 0 ; i < n ; ++i){
		for(j = 0 ; j < n ; ++j){
			fprintf(file,"%.3f ",matris[i][j]);
		}
		if(i != n - 1)
			fprintf(file,"; ");
	}
	fprintf(file,"]\n");
}


void freeMatris(double** matris,int n){
	int i;
	for(i = 0 ; i < n ; ++i)
		free(matris[i]);
	
	free(matris);
}

double** allocMatris(int n){
	int i,j;
	double** matris = (double**)malloc(n*sizeof(double*));
	for(i = 0 ; i < n ; i++)
		matris[i] = (double*)malloc(n*sizeof(double));

	return matris;
}

double** generateMatris(int n){
	int i,j,m = n/2,k,l;
	double d = 0,dThis = 0;
	double** tmp = allocMatris(m);

	double** matris = (double**)malloc(n*sizeof(double*));
	for(i = 0 ; i < n ; i++)
		matris[i] = (double*)malloc(n*sizeof(double));


	while(!dThis || dThis == NAN || dThis == INFINITY || dThis == -NAN || dThis == -INFINITY){

		randMatris(tmp,m,25);
		fillMatris(matris,tmp,0,m,0,m);
	
		randMatris(tmp,m,20);
		fillMatris(matris,tmp,0,m,m,2*m);
		
		randMatris(tmp,m,18);	
		fillMatris(matris,tmp,m,2*m,0,m);

		randMatris(tmp,m,22);
		fillMatris(matris,tmp,m,2*m,m,2*m);

		dThis = det(matris,n);
	}
	freeMatris(tmp,m);
	return matris;
}

void copyMatris(double** mat1,double mat2[20][20],int n){
	free(mat1);
	mat1 = allocMatris(n);

	int i,j;
	for(i = 0 ; i < n ; ++i)
		for(j = 0 ; j < n ; ++j){
			mat1[i][j] = mat2[i][j];
		}
}


double det(double** m,int n){ //http://easy-learn-c-language.blogspot.com.tr/2013/02/numerical-methods-determinant-of-nxn.html
	int i,j,k;
	double det,ratio;
	double** matrix = allocMatris(n);
	for(i = 0 ; i < n ; i++)
		for(j = 0 ; j < n ; j++)
			matrix[i][j] = m[i][j];

    for(i = 0; i < n; i++){
	    for(j = 0; j < n; j++){
	        if(j>i){
	            ratio = matrix[j][i]/matrix[i][i];

	            for(k = 0; k < n; k++)
	                matrix[j][k] -= ratio * matrix[i][k];
	        }
    	}
    }

    det = 1; 

    for(i = 0; i < n; i++)
	    det *= matrix[i][i];

    freeMatris(matrix,n);
    return det;
}

void CoFactor(double **a,int n,double **b) // https://www.cs.rochester.edu/~brown/Crypto/assts/projects/adj.html
{
   int i,j,ii,jj,i1,j1;
   double deter;
   double **c;

   c = malloc((n-1)*sizeof(double *));
   for (i=0;i<n-1;i++)
     c[i] = malloc((n-1)*sizeof(double));

   for (j=0;j<n;j++) {
      for (i=0;i<n;i++) {

         i1 = 0;
         for (ii=0;ii<n;ii++) {
            if (ii == i)
               continue;
            j1 = 0;
            for (jj=0;jj<n;jj++) {
               if (jj == j)
                  continue;
               c[i1][j1] = a[ii][jj];
               j1++;
            }
            i1++;
         }

         deter = det(c,n-1);

         b[i][j] = pow(-1.0,i+j+2.0) * deter;
      }
   }
   for (i=0;i<n-1;i++)
      free(c[i]);
   free(c);
}


void Transpose(double **a,int n)
{
   int i,j;
   double tmp;

   for (i=1;i<n;i++) {
      for (j=0;j<i;j++) {
         tmp = a[i][j];
         a[i][j] = a[j][i];
         a[j][i] = tmp;
      }
   }
}

void shiftedInverse(double** a,double** shifted,int n){
	int m = n/2;
	double** sub = allocMatris(m);
	double** subInv = allocMatris(m);

	getSubMatris(a,sub,0,m,0,m);
	inverse(sub,subInv,m);	
	fillMatris(shifted,subInv,0,m,0,m);

	getSubMatris(a,sub,0,m,m,2*m);
	inverse(sub,subInv,m);
	fillMatris(shifted,subInv,0,m,m,2*m);
		
	getSubMatris(a,sub,m,2*m,0,m);
	inverse(sub,subInv,m);	
	fillMatris(shifted,subInv,m,2*m,0,m);

	getSubMatris(a,sub,m,2*m,m,2*m);
	inverse(sub,subInv,m);
	fillMatris(shifted,subInv,m,2*m,m,2*m);
	
	freeMatris(sub,m);
	freeMatris(subInv,m);
}

void inverse(double** a,double** inv,int n){
	CoFactor(a,n,inv);
	Transpose(inv,n);
	multiplyMatris(inv,n,1/det(a,n));
}

void fillMatris(double** matris,double** sub,int m,int n,int a,int b){
	int k = 0 , l,i,j;
	for(i = m ; i < n ; ++i){
	    l = 0;
		for(j = a ; j < b ; ++j){
			matris[i][j] = sub[k][l];
			l++;
		}
		k++;
	}
}

void randMatris(double** a,int n,int r){
	double d = 0;
	int i,j;
	srand(time(NULL));
	while(!d || d == NAN || d == INFINITY || d == -NAN || d == -INFINITY){
		for(i = 0 ; i < n ; i++)
			for(j = 0 ; j < n ; j++)
				a[i][j] = (double)(rand() % r) + 3;

		d = det(a,n);
	}
}

void getSubMatris(double** matris,double** sub,int m,int n,int a,int b){
	int k = 0 , l,i,j;
	for(i = m ; i < n ; ++i){
	    l = 0;
		for(j = a ; j < b ; ++j){
			sub[k][l] = matris[i][j];
			l++;
		}
		k++;
	}
}

void convolution(double** in,int a,double** kernel,int b,double** out){//http://www.songho.ca/dsp/convolution/convolution.html
	// find center position of kernel (half of kernel size)
	int kCenterX = b / 2;
	int kCenterY = b / 2;
	int rows = a , cols = a;
	int kRows = b , kCols = b;
	int i,j,m,n,mm,nn,ii,jj;

	for(i=0; i < rows; ++i)              // rows
	{
	    for(j=0; j < cols; ++j)          // columns
	    {
	        for(m=0; m < kRows; ++m)     // kernel rows
	        {
	            mm = kRows - 1 - m;      // row index of flipped kernel

	            for(n=0; n < kCols; ++n) // kernel columns
	            {
	                nn = kCols - 1 - n;  // column index of flipped kernel

	                // index of input signal, used for checking boundary
	                ii = i + (m - kCenterY);
	                jj = j + (n - kCenterX);

	                // ignore input samples which are out of bound
	                if( ii >= 0 && ii < rows && jj >= 0 && jj < cols )
	                    out[i][j] += in[ii][jj] * kernel[mm][nn];
	            }
	        }
	    }
	}
}


void multiplyMatris(double** matris,int n,double mult){
	int i,j;
	for(i = 0 ; i < n ; ++i)
		for(j = 0 ; j < n ; ++j)
			matris[i][j] *= mult;
}