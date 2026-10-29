#ifdef USERSPACE_TEST
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#define MODULE_LICENSE(lic) const char *license=(lic)
#define MODULE_VERSION(ver) const char *version=(ver)
#define MODULE_DESCRIPTION(desc) const char *description=(desc)
#define module_init(p) int (*const initfunc)(void)=(p)
#define module_exit(p) void (*const exitfunc)(void)=(p)
#define __init
#define __exit
#define kernel_fpu_available() 1
#define kernel_fpu_begin() ((void)0)
#define kernel_fpu_end() ((void)0)
#define kmalloc(size,a1) malloc(size)
#define kfree free
#define printk printf
static int __init einit(void);
static void __exit eexit(void);
int main(void){
	int r;
#if !(ASAN)
	struct rlimit rlim[1];
	if(getrlimit(RLIMIT_STACK,rlim)<0)
		return -1;
	rlim->rlim_cur=8*1024;
	if(setrlimit(RLIMIT_STACK,rlim)<0)
		return -1;
#endif
	r=einit();
	eexit();
	return r;
}
#else
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/fpu.h>
#endif
MODULE_LICENSE("GPL");
MODULE_VERSION("0.01");
MODULE_DESCRIPTION("Test module for graph-drawer expr");
int eval_expr_write_result(const char *e,char *buf,size_t len,void *heap,size_t size);
#define MTL_HEAP_SIZE 8192
static char *expression="2.5+3.5+sum(n,1,100,1,n)";
static int __init einit(void){
	char outbuf[64];
	void *p;
	int r;
	if(!kernel_fpu_available()){
		printk("fpu is unavailable\n");
		printk("ABORTING\n");
		return -1;
	}
	printk("evaluate: %s\n",expression);
	p=kmalloc(MTL_HEAP_SIZE,GFP_KERNEL);
	if(!p){
		printk("memory allocation failed\n");
		printk("ABORTING\n");
		return -1;
	}
	kernel_fpu_begin();
	r=eval_expr_write_result(expression,outbuf,sizeof(outbuf),p,MTL_HEAP_SIZE);
	kernel_fpu_end();
	kfree(p);
	if(r<0)
		printk("error code=%d\n",r);
	else
		printk("result=%s\n",outbuf);
	return 0;
}
static void __exit eexit(void){
	printk("exiting ...\n");
	return;
}
module_init(einit);
module_exit(eexit);
