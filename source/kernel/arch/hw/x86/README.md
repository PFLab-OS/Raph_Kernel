# what is this?
Raph_Kernel is one part of Raphine Project.
It provides minimal operating system for this project which is highly adapted to distributed computing. This kernel also work well at multi processor environment and use your computer resource effectively.

The code is mainly maintained by Liva <sap.pcmail@gmail.com>. If you found a bug, please report to this email address. Also, we will be very glad if you propose us to join our team.

# how to
## first step
Run "vagrant up". This command prepares the building environment.

## how to create disk image
Just simply run "make". It will create a complete disk image.

## how to debug kernel
"make run" will launch qemu and show qemu console debugger. Open a new terminal and run "make vnc", you can see the display via VNC.

