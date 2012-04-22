obj-m += xeno-pwm.o
xeno-pwm-objs := xeno-pwm-drv.o pwm-task-proc.o

EXTRA_CFLAGS += -I/usr/include/xenomai/

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
