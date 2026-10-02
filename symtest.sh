#! /bin/bash
for i in {2..100}
do
	gcc -Wall -DEXPR_SYMNEXT=$i symtest.c common_symbols.c -o symtest expr.c -lc -lm
#	v=1
#	for j in {1..10}
#	do
#		x="$(./symtest 10000 0 2>/dev/null)"
#		v="$(./calc "$v+$x")"
#	done
	./symtest 1000000 0 2>/dev/null
#	echo -ne "$(./calc "floor($v)")" '\t'
#	echo "EXPR_SYMNEXT=$i"

done
