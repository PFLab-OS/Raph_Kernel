# what is this?
Raph_Kernel is one part of Raphine Project.
It provides minimal operating system for this project which is highly adapted to distributed computing. This kernel also work well at multi processor environment and use your computer resource effectively.

The code is mainly maintained by Liva <sap.pcmail@gmail.com>. If you found a bug, please report to this email address. Also, we will be very glad if you propose us to join our team.

# how to
## how to create disk image
First, you have to run "make disk" in this directory(where README.md is located). It will create complete disk image of Raph Kernel. After that, if you need to rebuild kernel, the only thing you have to do is just type "make". You don't have to rerun "make disk" anymore. (until you delete disk image manually)

## how to debug kernel
"make run" will launch qemu and show qemu console debugger.

## for developper's
I higly recommend to use Raph_Kernel_DevEnv if you want to debug & develop Raph_Kernel. It creates well-prepared virtual machine environment and if you build Raph_Kernel on it, you will not need to consider about any software dependencies or environment specific problems. Of course, VM may make your PC slow, but if you don't want to be getting caught by some trivial matters and mess your times up, it would be a good option, I guess.

# references
There are many references to create Raph_Kernel. In the source code, I used shorthand expression to note it. These are the correspondence.

- intel64 manual : intel64 and IA-32 architectures software developer's manual
- mp spec : MultiProcessor Specification
- IOAPIC manual : 82093AA I/O Advanced Programmable Interrupt Controller (IOAPIC)
- acpi spec: Advanced Configuration and Power Interface Specification