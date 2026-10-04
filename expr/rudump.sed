h
s/^.*_r(\(.*\));$/\1/
s/,const struct expr_memtool \*restrict mtl//
s/const struct expr_memtool \*restrict mtl/void/
H
s/$/,/
s/\[.*\]//g
s/[^,]*\b\([A-Za-z0-9_]*\)/\1/g
s/.$//
H
g
s/^\(.*\)\b\(.*\)_r(\(.*\));\n\(.*\)\n\(.*\)$/\1\2(\4){\n\treturn \2_r(\5,expr_defmtl);\n}/
s/^\(void [A-Za-z].*\)return \(.*\)/\1\2/
s/^\(.*\)\b\(expr_.*(.*,\.\.\..*{\n\).*return \(.*\)\.\.\.\(.*;\)/\1\2\t\1r;\n\tva_list ap;\n\tva_start(ap,);\n\tr=\3ap\4\n\tva_end(ap);\n\treturn r;/
s/void,//
