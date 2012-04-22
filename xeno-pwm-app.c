#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <rtdm/rtdm.h>

#define DEVICE_NAME "gpio-pwm"


int 
main(int argc, char *argv)
{
  char buf[64];
  int c;
  ssize_t bufsize = sizeof(buf);
  ssize_t size;
  int device;
  int ret;

  // open the device
  device = rt_dev_open(DEVICE_NAME, 0);
  if (device < 0)
    {
      printf("ERROR : can't open device %s (%s)\n",
	     DEVICE_NAME, strerror(-device));
      return 1;
    }

  for(c = 0; c <= 100; c += 10)
    {
      size = sprintf(buf, "%u", c);
      size = rt_dev_write (device, (const void*)buf, size);
      printf("Write to device %s\t: %d bytes - %s\n", DEVICE_NAME, size, buf);

      sleep(1);
      memset(buf, 0, bufsize);

      size = rt_dev_read (device, (void*)&buf, bufsize);
      printf("Read from device %s\t: %d bytes - %s\n", DEVICE_NAME, size, buf);
    }

  size = sprintf(buf, "50");
  size = rt_dev_write (device, (const void*)buf, size);

  // close the device
  ret = rt_dev_close(device);
  if (ret < 0)
    {
      printf("ERROR : can't close device %s (%s)\n",
	     DEVICE_NAME, strerror(-ret));
      return 2;
    }

  return 0;
}
