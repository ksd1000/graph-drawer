/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include "text.c.h"
#include <stddef.h>
//#include <stdio.h>
//#include <stdlib.h>
const struct sbmp *text_getsbmp(char c){
	struct sbmp *sp;
	for(int i=0;i<TEXT_COUNT;++i){
		sp=(struct sbmp *)texts[i];
		if(sp->c==c)return sp;
	}
	return NULL;
}
