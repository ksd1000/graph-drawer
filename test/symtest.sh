#! /bin/bash
if [ $# -gt 1 ];then
	rm -vf expr-aci.so symtest
	exit $?
fi
gcc -Wall -Wextra -I ../expr -o expr-aci.so -shared -fPIC ../expr/expr-internal.c ../expr/expr-alloc.c -O3
if [ $# -eq 1 ];then
	gcc -Wall -Wextra -DEXPR_SYMNEXT="$1" -I ../expr symtest.c ../expr/expr-core.c -o symtest "$PWD/expr-aci.so" -O3
	exit $?
fi
for i in {2..100}
do
#	gcc -Wall symtest.c common_symbols.c -o symtest expr.c -lc -lm
	gcc -Wall -Wextra -DEXPR_SYMNEXT=$i -I ../expr symtest.c ../expr/expr-core.c -o symtest "$PWD/expr-aci.so" -O3
#	v=1
#	for j in {1..10}
#	do
#		x="$(./symtest 10000 0 2>/dev/null)"
#		v="$(./calc "$v+$x")"
#	done
#	./symtest 1000000 0 2>/dev/null
	#LD_LIBRARY_PATH+=:"$PWD" ./symtest >/dev/null
	./symtest >/dev/null
#	echo -ne "$(./calc "floor($v)")" '\t'
#	echo "EXPR_SYMNEXT=$i"

done
