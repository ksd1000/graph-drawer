# The expression evaluator library can also run in the kernel

**WARNING: Do not run it in the physical machine like the author. To avoid some undiscovered bugs that may affect your system.**
Use a virtual machine to run it instead, unless you can guarantee there are no any bugs in libexpr may affect your system.

Steps:

1. Run **uname -r** to check your kernel version and ensure the source is at /usr/src/kernels/<version>
2. Test in userspace first:
```
make ustest
./userspace-test
```
Ensure there are no errors and ASAN reports. And the output is only like this:
```
evaluate: 2.5+3.5
result=6.000000
exiting ...
```
3. Build the kernel module and load it:
```
make
sudo insmod kexpr.ko
sudo rmmod kexpr.ko
```
The expected output in **dmesg** will be like this:
```
[613733.621003] evaluate: 2.5+3.5
[613733.621020] result=6.000000
[613743.767143] exiting ...
```

The core of the expression evaluator library may also be able to run in QEMU directly without any operating system, but this is not tested, because the author does not know how to write the code for testing.
