#include "text.c.h"
#include <stdlib.h>
#include "sbmp.h"
const struct sbmp *text_getsbmp(char c){
	struct sbmp *sp;
	for(int i=0;i<TEXT_SIZE;++i){
		sp=(struct sbmp *)texts[i];
		if(sp->c==c)return sp;
	}
	return NULL;
}
