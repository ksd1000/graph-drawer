#ifndef _SBMP_H_
#define _SBMP_H_
struct sbmp {
	int32_t width,height;
	char c;
	char data[];
};
#define SBMP_TSTPIXEL(sb,x,y) ((sb)->data[((y)*(sb)->width+(x))>>3]&(1<<(((y)*(sb)->width+(x))&7)))
#define SBMP_SETPIXEL(sb,x,y) ((sb)->data[((y)*(sb)->width+(x))>>3]|=(1<<(((y)*(sb)->width+(x))&7)))
#define SBMP_MODPIXEL(sb,x,y) ((sb)->data[((y)*(sb)->width+(x))>>3]^=(1<<(((y)*(sb)->width+(x))&7)))
#define SBMP_CLRPIXEL(sb,x,y) ((sb)->data[((y)*(sb)->width+(x))>>3]&=~(1<<(((y)*(sb)->width+(x))&7)))
#endif
