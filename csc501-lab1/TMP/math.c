/* math.c */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <math.h>

#define RAND_MAX 32767


double pow(double x, int y){
        int i;
        double result = 1;
        for(i = 1; i <= y; i++){
                result = result  * x;
        }
        return(result);
}

double log(double x){
	double result = 0;
	int i;
	for(i=1;i<=20;i++){
		result += pow(-1, i+1) * pow(x-1,i)/i;
	}
	return result;
}


double expdev(double lambda) {
    double dummy;
    do
        dummy= (double) rand() / RAND_MAX;
    while (dummy == 0.0);

    return -log(dummy) / lambda;
}

