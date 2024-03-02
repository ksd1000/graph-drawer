#ifndef _SBMP_H_
#define _SBMP_H_
#include <stdint.h>
struct sbmp {
	int32_t width,height;
	uint64_t size;
	char c;
	unsigned char compressed:1;
	unsigned char startval:1;
	unsigned char :6;
	unsigned char unused[6];
	unsigned char data[];
};
struct sbmp *sbmp_compress(const struct sbmp *sp);
int sbmp_tstpixeln(const struct sbmp *sp,uint64_t n);
int sbmp_tstpixel(const struct sbmp *sp,int32_t x,int32_t y);
int sbmp_decompress(const struct sbmp *sp,struct sbmp *out);

#define SBMP_INDEX(sb,x,y) ((y)*(sb)->width+(x))
#define SBMP_TSTPIXEL(sb,n) ((sb)->data[(n)>>3]&(1<<((n)&7)))
#define SBMP_SETPIXEL(sb,n) ((sb)->data[(n)>>3]|=(1<<((n)&7)))
#define SBMP_MODPIXEL(sb,n) ((sb)->data[(n)>>3]^=(1<<((n)&7)))
#define SBMP_CLRPIXEL(sb,n) ((sb)->data[(n)>>3]&=~(1<<((n)&7)))
#endif
