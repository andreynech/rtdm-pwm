#include <linux/module.h>
#include <rtdm/rtdm_driver.h>
#include "pwm-task-proc.h"
#include "div100.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrey Nechypurenko <andreynech@gmail.com>");
MODULE_DESCRIPTION("Generates RC PWMs with GPIO");

#define DEVICE_NAME "gpio-pwm"
#define SOME_SUB_CLASS 4711

/**
 * The context of a device instance
 *
 * A context is created each time a device is opened and passed to
 * other device handlers when they are called.
 *
 */
typedef struct context_s
{
} context_t;


/**
 * Open the device
 *
 * This function is called when the device shall be opened.
 *
 */
static int 
pwm_rtdm_open_nrt(struct rtdm_dev_context *context,
		  rtdm_user_info_t *user_info, 
		  int oflags)
{
  //context_t *ctx = (context_t*)context->dev_private;  

  return 0;
}

/**
 * Close the device
 *
 * This function is called when the device shall be closed.
 *
 */
static int 
pwm_rtdm_close_nrt(struct rtdm_dev_context *context,
		   rtdm_user_info_t * user_info)
{
  return 0;
}

/**
 * Read from the device
 *
 * This function is called when the device is read in non-realtime
 * context.
 *
 */
static ssize_t 
pwm_rtdm_read_nrt(struct rtdm_dev_context *context,
		  rtdm_user_info_t * user_info, void *buf,
		  size_t nbyte)
{
  //context_t *ctx = (context_t*)context->dev_private;
  size_t size;
  char uptime[32];
  size = sprintf(uptime, "%u", div100(getpwmwidth(0)));
  if(rtdm_safe_copy_to_user(user_info, buf, uptime, size))
    rtdm_printk("ERROR : can't copy data from driver\n");

  return size;
}

/**
 * Write in the device
 *
 * This function is called when the device is written in non-realtime context.
 *
 */
static ssize_t 
pwm_rtdm_write_nrt(struct rtdm_dev_context *context,
		   rtdm_user_info_t * user_info,
		   const void *buf, size_t nbyte)
{
  //context_t *ctx = (context_t*)context->dev_private;

  int duty_perc = simple_strtoul(buf, NULL, 0);
  setpwmwidth(0, duty_perc);
  return nbyte;
}

/**
 * This structure describe the simple RTDM device
 *
 */
static struct rtdm_device device = {
  .struct_version = RTDM_DEVICE_STRUCT_VER,

  .device_flags = RTDM_NAMED_DEVICE,
  .context_size = sizeof(context_t),
  .device_name = DEVICE_NAME,

  .open_nrt = pwm_rtdm_open_nrt,

  .ops = {
    .close_nrt = pwm_rtdm_close_nrt,
    .read_nrt = pwm_rtdm_read_nrt,
    .write_nrt = pwm_rtdm_write_nrt,
  },

  .device_class = RTDM_CLASS_EXPERIMENTAL,
  .device_sub_class = SOME_SUB_CLASS,
  .profile_version = 1,
  .driver_name = "GPIO-PWM",
  .driver_version = RTDM_DRIVER_VER(0, 1, 2),
  .peripheral_name = "GPIO PWM generator",
  .provider_name = "Andrey Nechypurenko",
  .proc_name = device.device_name,
};


/**
 * This function is called when the module is loaded
 *
 * It simply registers the RTDM device.
 *
 */
int __init pwm_rtdm_init(void)
{
  pwm_desc_t pwm[1];
  int res;

  res = rtdm_dev_register(&device);
  if(res == 0)
    rtdm_printk("PWM driver registered without errors\n");
  else
    {
      rtdm_printk("PWM driver registration failed: \n");
      switch(res)
	{
	case -EINVAL: 
	  rtdm_printk("The device structure contains invalid entries. "
		      "Check kernel log for further details.");
	  break;

	case -ENOMEM: 
	  rtdm_printk("The context for an exclusive device cannot be allocated.");
	  break;

	case -EEXIST:
	  rtdm_printk("The specified device name of protocol ID is already in use.");
	  break;

	case -EAGAIN: rtdm_printk("Some /proc entry cannot be created.");
	  break;
	
	default:
	  rtdm_printk("Unknown error code returned");
	  break;
	}
      rtdm_printk("\n");
    }

  // Initialize with default values
  pwm[0].channel = 0;
  pwm[0].pwmMinWidth = 950;
  pwm[0].pwmMaxWidth = 2050;

  res = initpwm(pwm, sizeof(pwm) / sizeof(pwm[0]));
  if(res != 0)
    rtdm_printk("PWM: initialization error: %i was returned\n", res);
  else
    setpwmwidth(0, 50); // 50% duty is middle position

  return res;
}

/**
 * This function is called when the module is unloaded
 *
 * It unregister the RTDM device, polling at 1000 ms for pending users.
 *
 */
void __exit pwm_rtdm_exit(void)
{
  rtdm_printk("PWM: stopping pwm tasks\n");
  cleanuppwm();
  rtdm_dev_unregister(&device, 1000);
  rtdm_printk("PWM: uninitialized\n");
}


module_init(pwm_rtdm_init);
module_exit(pwm_rtdm_exit);
