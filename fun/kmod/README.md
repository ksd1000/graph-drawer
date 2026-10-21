# The libexpr can also run in kernel

**WARNING: Do not run it in physical machine like the author. Avoid some undiscovered bugs which may affect your system.** 
Use a virtual machine to run it instead, unless you can guarantee there is not any bug in libexpr may affect your system.

Steps:

- 1. Run **uname -r** to check you kernel version and ensure the source is at /usr/src/kernels/<version>
- 2. Test in userspace first:
```
make ustest
./userspace-test
```
Ensure there is not any error and ASAN report. And only output like this:
```
evaluate: 2.5+3.5
result=6.000000
exiting ...
```
- 3. Build kernel module and load it:
```
make
sudo insmod kexpr.ko
sudo rmmod kexpr.ko
```
The expected output will be like this:
```
[613733.621003] evaluate: 2.5+3.5
[613733.621020] result=6.000000
[613743.767143] exiting ...
```

The expression evaluator library may be also run in qemu directly, but this is not tested, for the author does not know how to write the code.
