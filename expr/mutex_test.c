#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
//#include "mutex.h"
#include "expr.h"
uint32_t l=0;
size_t x=0,y=0;
pthread_mutex_t m=PTHREAD_MUTEX_INITIALIZER;
void *tmain(void *arg){
	for(int i=0;i<100000;++i){
	//	mutex_lock(&l);
	//	pthread_mutex_lock(&m);
	//	mutex_atomic((mutex_t *)&l)
		expr_mutex_lock(&l);
		++x;
		expr_mutex_unlock(&l);
	//	mutex_unlock(&l);
	//	pthread_mutex_unlock(&m);
		++y;
	}
	return NULL;
}
#define n 50
int main(void){
	pthread_t t[n];
	for(int i=0;i<n;++i)
		pthread_create(t+i,NULL,tmain,NULL);
	for(int i=0;i<n;++i)
		pthread_join(t[i],NULL);
	printf("x=%zu,y=%zu\n",x,y);
}
