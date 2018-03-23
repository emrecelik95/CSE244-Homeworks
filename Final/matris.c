#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#include "matris.h"

/*
int main(void){

	int r = 10,
		c = 10;

	double** A = generateMatris(r,c);
	double** b = generateMatris(r,1);
	double** sudo = allocMatris(c,1);


	printMatris(A,r,c);
	printf("\n");
	printMatris(b,r,1);

	printf("\n");

	pseudoInverse(A,b,r,c,sudo);

	printMatris(sudo,c,1);

	freeMatris(A,r);
	freeMatris(b,r);
	freeMatris(sudo,r);

	return 0;
}
*/

double** pseudoInverse(double** a,double** b,int m,int n,double** sudoInverse){
	double **t = allocMatris(m,n);
	double **mult = allocMatris(n,m);
	double **inv = allocMatris(n,m);
	double **mult2 = allocMatris(n,m);

	int i,j;
	for(i = 0 ; i < m ; i++)
		for(j = 0 ; j < n ; j++)
			t[i][j] = a[i][j];

	Transpose(a,t,m,n);


	multiple(t,n,m,a,m,n,mult);


	
	inverse(mult,inv,n,m);

	multiple(inv,n,m,t,n,m,mult2);

	multiple(mult2,n,m,b,m,1,sudoInverse);

	freeMatris(t,m);
	freeMatris(inv,n);
	freeMatris(mult,n);
	freeMatris(mult2,n);

}

void Diff(double** a,double **b,int m,int n,double **diff){
	int i,j;
	for(int i = 0 ; i < m ; i++)
		for(int j = 0 ; j < n ; j++)
			diff[i][j] = a[i][j] - b[i][j]; 
}

void multiple(double** a,int m,int n,double **b,int p,int q,double **mult){
	int c ,d,k;
	double sum = 0;

	for (c = 0; c < m; c++) {
  	  for (d = 0; d < q; d++) {
  	    for (k = 0; k < p; k++) {
          sum = sum + a[c][k]*b[k][d];
        }
 
        mult[c][d] = sum;
        sum = 0;
      }
    }
}

void printMatris(double** matris,int r,int c){
	int i,j;
	
	for(i = 0 ; i < r ; ++i){
		for(j = 0 ; j < c ; ++j)
			fprintf(stdout,"%.2f\t",matris[i][j]);
		fprintf(stdout,"\n");
	}
}


void freeMatris(double** matris,int r){
	int i;
	for(i = 0 ; i < r ; ++i)
		if(matris[i] != NULL)
			free(matris[i]);
	
	if(matris != NULL)
		free(matris);
}

double** allocMatris(int r,int c){
	int i,j;
	double** matris = (double**)malloc(r*sizeof(double*));
	for(i = 0 ; i < r ; i++)
		matris[i] = (double*)malloc(c*sizeof(double));

	return matris;
}

double** generateMatris(int r,int c){

	struct timespec t;

	int i,j;
	double** matris = allocMatris(r,c);
	
	clock_gettime(CLOCK_REALTIME,&t);

	srand(t.tv_nsec);

	for(i = 0 ; i < r ; ++i)
		for(j = 0 ; j < c ; ++j){
			matris[i][j] = rand() % (i*j + 100) + 1;
		}


	return matris;
}


void writeMatris(double** matris,int r,int c,FILE* file){
	int i,j;
	fprintf(file,"[");
	for(i = 0 ; i < r ; ++i){
		for(j = 0 ; j < c ; ++j){
			fprintf(file,"%.3f ",matris[i][j]);
		}
		if(i != r - 1)
			fprintf(file,"; ");
	}
	fprintf(file,"]\n");
}


void Transpose(double **a,double** t , int r ,int c)
{
   int i,j;

	for(i=0; i<r; ++i)
	    for(j=0; j<c; ++j)
	    {
	        t[j][i] = a[i][j];
	    }

}


void inverse(double** a,double** inv,int m,int n){
	double **t = allocMatris(m,n);
	int i,j;


	CoFactor(a,m,n,inv);

	Transpose(inv,t,m,n);
	for(i = 0 ; i < m ; i++)
		for(j = 0 ; j < m ; j++)
			inv[i][j] = t[i][j];

	freeMatris(t,m);

	multiplyMatris(inv,m,n,1/det(a,m,n));
}



void CoFactor(double **a,int m,int n,double **b) // https://www.cs.rochester.edu/~brown/Crypto/assts/projects/adj.html
{
   int i,j,ii,jj,i1,j1;
   double deter;
   double **c;

   c = malloc((m-1)*sizeof(double *));
   for (i=0;i<m-1;i++)
     c[i] = malloc((n-1)*sizeof(double));

   for (j=0;j<m;j++) {
      for (i=0;i<n;i++) {

         i1 = 0;
         for (ii=0;ii<m;ii++) {
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

         deter = det(c,m-1,n-1);

         b[i][j] = pow(-1.0,i+j+2.0) * deter;
      }
   }
   for (i=0;i<m-1;i++)
      free(c[i]);
   free(c);
}



double det(double** m,int r,int c){ //http://easy-learn-c-language.blogspot.com.tr/2013/02/numerical-methods-determinant-of-nxn.html
	int i,j,k;
	double det,ratio;
	double** matrix = allocMatris(r,c);
	for(i = 0 ; i < r ; i++)
		for(j = 0 ; j < c ; j++)
			matrix[i][j] = m[i][j];

    for(i = 0; i < r; i++){
	    for(j = 0; j < c; j++){
	        if(j>i){
	            ratio = matrix[j][i]/matrix[i][i];

	            for(k = 0; k < r; k++)
	                matrix[j][k] -= ratio * matrix[i][k];
	        }
    	}
    }

    det = 1; 

    for(i = 0; i < r; i++)
	    det *= matrix[i][i];

    freeMatris(matrix,r);
    return det;
}




void multiplyMatris(double** matris,int m,int n,double mult){
	int i,j;
	for(i = 0 ; i < m ; ++i)
		for(j = 0 ; j < n ; ++j)
			matris[i][j] *= mult;
}

/*
void copyMatris(double** mat1,double mat2[20][20],int n){
	free(mat1);
	mat1 = allocMatris(n);

	int i,j;
	for(i = 0 ; i < n ; ++i)
		for(j = 0 ; j < n ; ++j){
			mat1[i][j] = mat2[i][j];
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


*/