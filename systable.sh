#! /bin/bash
if [ -n "$PREFIX" ];then
	:
else
	PREFIX="/usr"
fi
gcc -dM -E $PREFIX/include/syscall.h |grep __NR_ |sed 's/#define __NR_/register_syscall(/g' |sed 's/ .*$/)/g' |grep register_syscall
