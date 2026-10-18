# graph-drawer

![logo](https://raw.githubusercontent.com/ksd1000/graph-drawer/refs/heads/main/logo.png)

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
./wave 'sin(440*2*pi*t)' -c 1 -o 1.wav
```
generate a 1s 440Hz sound .wav file

compare with [zserge's expr](https://github.com/zserge/expr)

||zserge expr|tinyexpr|graph-drawer expr|
|:---:|:---:|:---:|:---:|
|Data type|float|double|double|
|Initialize speed|Medium 2.3x|Fast 7.8x|Slow 1.0x|
|Memory cost|Medium 2.3x|Low 1.0x|High 41.7x|
|bitwise and/or/xor support|Cast to integer|No|Yes|
|Number parse|Digit accumulate|Digit accumulate|Pseudo strtod|
|Number parse speed|Fast-Medium|Fast|Slow|
|libc required|Yes|Yes|No for core. But builtin symbols and frontend needs.|
|Core arch independence|Yes|Yes|x86/arm64 for mem* and syscall|
|Non-unix support|Untested|Untested(the author tested Yes)|Yes(\*.exe in Release)|
|User variable defined in code|Yes|Yes|Yes|
|User variable defined in expr|Yes(Auto)|No|Yes(value-->a/a=:value/var(a,[value]))|
|Allow write variables in expr|Yes(=)|No|Yes(--\>,\->)|
|User constant|No|No(only zero args pure function)|Yes|
|Active|No (Latest 2017-12-3)|Yes|Yes|
|Amount of bugs|Few-Medium|Few|Many|
|Size|Lightweight|Lightweight|Bloated|
|Allow unsafe mode|No|No|Yes|
|Gnu extension required|No (C99)|No (C99)|Yes|
|Power operator|\*\*|^|\*\*|
|Amount of file|Single header file|Single .c file+header|Multi-file|
|Hot function|Yes|No|Yes|
|Optimize|No|Yes|Yes|
|Symbol system|struct expr_var_list(one-next list)/struct expr_func(array)|te_variable(array)|struct expr_symset(multi-next list)/struct expr_builtin_symbol(array)|
|Symbol create API|expr_var()|No|expr_symset_add()|
|Eval API|expr_eval()|te_eval()|expr_eval()|
|One-shot API|No|te_interp()|expr_calc()|
|Custom memory allocator|Yes(global variable required,patch before header)|Yes(global variable required,patch before header)|Yes(global+non-global)|
|MT-safe for same object|Yes(except = and context)|Yes|No(an object only for one thread)|
|Reuseability|No(allocate/free require global variable)|No(similarly)|Yes(*_r)|
|JIT support|Yes|No|No|
|Branch and loop|No|No|Yes|
|Syscall support|No|No|Yes|
|AST|Yes|Yes|No|
|Project type|standalone library|standalone library|backend library of graph-drawer|
|Bugfix speed|Unactive|Fast|Slow|
|Code readability|Clear|Clear|Chaos|
|Build system|make|make|cmake|
|Start date|07-08-2016|01-21-2016|02-19-2024|
|Popularity|Medium|High|Low|
|License|MIT|Zlib|GPL v3+|
