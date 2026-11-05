#define _GNU_SOURCE
#include <stdio.h>
int main(void){
	size_t len=6;
	printf("expect:123456\n");
	printf("result:%.*s\n",(int)len,"123456789");
	return 0;
}
