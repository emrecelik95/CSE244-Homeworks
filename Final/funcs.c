#include <unistd.h>
#include <stdlib.h>
#include "funcs.h"

int readData (int s,void *buf,int n)
{
	int bcount;
	int br;
	bcount= 0;
	br = 0;
	
	while (bcount < n) { 
		if ((br= read(s,buf,n-bcount)) > 0) {
			bcount += br;
			buf += br;
		}
		else if (br < 0)
			return(-1);
	}

	return(bcount);
}

int writeData (int s,void *buf,int n)
{
	int bcount;
	int br;
	bcount= 0;
	br = 0;
	
	while (bcount < n) { 
		if ((br= write(s,buf,n-bcount)) > 0) {
			bcount += br;
			buf += br;
		}
		else if (br < 0) 
			return(-1);
	}

	return(bcount);
}