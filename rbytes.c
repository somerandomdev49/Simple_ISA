#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(){
	long i = 0;
	srand(time(NULL));
	for(;i < 65534; i++){putchar(rand()&255);}
	return 0;
}
