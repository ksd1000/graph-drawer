h
s/^.*_r(\(.*\));$/\1/
s/,const uint8_t \*restrict table//
s/const uint8_t \*restrict table,//
s/const uint8_t \*restrict table/void/
H
s/$/,/
s/\[.*\]//g
s/[^,]*\b\([A-Za-z0-9_]*\)/\1/g
s/.$//
H
g
s/^\(.*\)\b\(.*\)_r(\(.*\));\n\(.*\)\n\(.*\)$/#define \2(\5) \2_r(\5,expr_writefmts_table_default)/
s/\.\.\.,expr_writefmts_table_default/expr_writefmts_table_default,__VA_ARGS__/
s/syms,expr_writefmts_table_default,__VA_ARGS__/expr_writefmts_table_default,syms,__VA_ARGS__/g
s/void,//
s/(void)/()/
