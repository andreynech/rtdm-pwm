rtdm-pwm
========

Xenomai RTDM driver to generate standard RC PWMs with GPIO on BeagleBoard xM.

There are sources for the kernel module and test program which is implemented in xeno-pwm-app.c. This program changes pwm duty cycle from 0 to 100% with 10% step and finally sets the duty cycle to 50%. It should be the middle position of the servo motor.

To build the kernel module xeno-pwm.ko, just run make out of the kernel source tree (i.e. in the directory where where you cloned git repository). To build test application, run make -f Makefile.cli . If you need more instructions how we set-up our (cross)compilation environment, please refer the following article: http://veter-project.blogspot.com/2012/03/comfortable-kernel-workflow-on.html . If everything goes fine, there will xeno-pwm.ko and xeno-pwm-app binary files.

To test the application, first insert module with insmod xeno-pwm.ko. Make sure to check dmesg for possible error messages. All messages should be preffixed with "PWM:". If there are no error messages, and you can see the xeno_pwm in the output of the lsmod, then you can run xeno-pwm-app to change PWM duty cycles.
