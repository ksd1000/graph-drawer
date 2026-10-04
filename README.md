# graph-drawer

Example:
```
./draw -o 1.bmp 'sin(t)'
```
draw a graph of sin(t) to 1.bmp

```
./calc 'sum(n,1,100,1,n)'
```
output the sum from 1 to 100

```
./wave 'sin(440*2*pi*t)'
```
play a 440Hz sound
```
./calc '0-->x,0-->y,for(n,1,x<=555555,(x->y,prime((n+1)->n)->x),y)'
```
find the greatest prime <=555555 (555523)
```
./calc 'for(n,0,n<100000,(prime((n+1)->n)-->p,while(p>=1,(write(1,p%10+48),(p/10)->p),0),write(1,"\n")),exit(0))'|rev

or

./calc 'for(n,1,100000,1,print(prime(n)))'
```
print the first 100000 primes
```
./calc "\
syscall(sys_socket,AF_INET,SOCK_STREAM,IPPROTO_TCP)-->fd;\
p=byte(16);\
bzero(p,16);\
_w16(p,AF_INET);\
_w16(p#2,htons(12345));\
syscall(sys_bind|sysp2,fd,p,16);\
syscall(sys_listen,fd,1);\
rfd=:0;\
while(syscall(sys_accept,fd,0,0)->rfd>=0){\
	b=byte(128*1024*1024);\
	sz=:0;\
	while(read(rfd,b,128*1024*1024)->sz>0){\
		write(rfd,b,sz);\
	};\
	syscall(sys_close,rfd);\
};\
syscall(sys_close,fd);\
"
```
a simple echo server with port 12345

compare with [zserge's expr](https://github.com/zserge/expr)

(By evaluating sqrt(4)+(0.75|1.5) for 10^8 times)

```
~/zserge_expr $ gcc e.c -o e -lm -Wall -O3
//a Syntax error will occur if -Ofast
~/zserge_expr $ time ./e 'sqrt(4)+(0.75|1.5)'
result: 3.000000

real    0m1.328s
user    0m1.318s
sys     0m0.006s
~/zserge_expr $ time ./calc --count 100000000 'sqrt(x0+4)+(0.75|1.5)'
//x0=0.
//The expression will be optimized
//into the constant 3.75 (the real
//time became 0m0.287s) if there
//is not x0
3.75

real    0m1.153s
user    0m1.142s
sys     0m0.005s
```
e.c:
```c
#include "expr.h"
static float mysqrt(struct expr_func *f, vec_expr_t *args, void *c) {
  float a = expr_eval(&vec_nth(args, 0));
  return sqrtf(a);
}
static struct expr_func user_funcs[] = {
    {"sqrt", mysqrt, NULL, 0},
    {NULL, NULL, NULL, 0},
};
int main(int c,char **v) {
  const char *s = c>1?v[1]:"5+2";
  struct expr_var_list vars = {0};
  struct expr *e = expr_create(s, strlen(s), &vars,user_funcs);
  int count=100000000;
  float result;
  if (e == NULL) {
    printf("Syntax error");
    return 1;
  }
redo:
  result = expr_eval(e);
	if(--count)goto redo;
  printf("result: %f\n", result);

  expr_destroy(e, &vars);
  return 0;
}

```

For older versions of this project, see [xgraph](https://github.com/ksd1000/xgraph) and [xgraph-front](https://github.com/ksd1000/xgraph-front)
