#include<stdio.h>
#include"common.h"


void main(){

printf("\nGive a path:");

char path[1000];
scanf("%s",path);

printf("\nLast node:%s",last_path(path));
}
