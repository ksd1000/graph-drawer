#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "mutex.h"
mutex_t l=0;
size_t x=0,y=0;
pthread_mutex_t m=PTHREAD_MUTEX_INITIALIZER;
void *tmain(void *arg){
	for(int i=0;i<100000;++i){
	//	mutex_lock(&l);
	//	pthread_mutex_lock(&m);
		mutex_atomic(&l)
			++x;
	//	mutex_unlock(&l);
	//	pthread_mutex_unlock(&m);
		++y;
	}
	return NULL;
}
void *t1main(void *arg){
	for(int i=0;i<10000;++i){
		printf("%d lock\n",gettid());
		mutex_lock(&l);
		printf("%d locked\n",gettid());
	return NULL;
		++x;
		printf("%d %d\n",gettid(),i);
		printf("%d unlock\n",gettid());
		mutex_unlock(&l);
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
