h
s/^.*_r(\(.*\));$/\1/
s/,const struct expr_memtool \*restrict mtl//
s/const struct expr_memtool \*restrict mtl,//
s/const struct expr_memtool \*restrict mtl/void/
H
s/$/,/
s/\[.*\]//g
s/[^,]*\b\([A-Za-z0-9_]*\)/\1/g
s/.$//
H
g
s/^\(.*\)\b\(.*\)_r(\(.*\));\n\(.*\)\n\(.*\)$/#define \2(\5) \2_r(\5,expr_defmtl)/
s/\.\.\.,expr_defmtl/expr_defmtl,##__VA_ARGS__/
s/syms,expr_defmtl,##__VA_ARGS__/expr_defmtl,syms,##__VA_ARGS__/g
s/void,//
s/(void)/()/
