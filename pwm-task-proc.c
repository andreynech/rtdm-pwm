#include <rtdm/rtdm_driver.h>
#include "pwm-task-proc.h"
#include "div100.h"


//extern lldiv_t_rr __aeabi_ldivmod(long long, long long);

/*
 * IEN  - Input Enable
 * IDIS - Input Disable
 * PTD  - Pull type Down
 * PTU  - Pull type Up
 * DIS  - Pull type selection is inactive
 * EN   - Pull type selection is active
 * M0   - Mode 0
 */

#define IEN     (1 << 8)

#define IDIS    (0 << 8)
#define PTU     (1 << 4)
#define PTD     (0 << 4)
#define EN      (1 << 3)
#define DIS     (0 << 3)

#define M0      0
#define M1      1
#define M2      2
#define M3      3
#define M4      4
#define M5      5
#define M6      6
#define M7      7

#define OFFSET(OFF) (OFF / sizeof(ulong))
#define RANGE_MAP100(c, d, x) (c + div100((d - c) * x))

static int RC_NUM = 0;
static rtdm_timer_t up_timer;
static rtdm_timer_t down_timer[8];
static nanosecs_rel_t up_period[8];
static uint8_t reconfigured[8];


// Initialized with default pulse ranges
static int ranges[8][2] = {
  {950, 2050},
  {950, 2050},
  {950, 2050},
  {950, 2050},
  {950, 2050},
  {950, 2050},
  {950, 2050},
  {950, 2050}
};

// Memory for triggering gpio
static void __iomem *gpio = NULL;


void 
pwm_up(rtdm_timer_t *timer)
{
  size_t channel = 0; // TODO: should be looked up using timer parameter
  int retval;

  //set_data_out has offset 0x94
  iowrite32(0x40000000, gpio + 0x6094);
  //rtdm_task_sleep(2000*1000);
  //iowrite32(0x40000000, gpio + 0x6090);
  //return;

  if(reconfigured[channel])
    {
      reconfigured[channel] = 0;
      rtdm_timer_stop(&down_timer[channel]);
      retval = rtdm_timer_start(&down_timer[channel], 
				up_period[channel], // we will use periodic timer
				20000000,
      				RTDM_TIMERMODE_RELATIVE);

      //rtdm_timer_stop_in_handler(&down_timer[channel]);
      //retval = rtdm_timer_start_in_handler(&down_timer[channel], 
      //				   0, // we will use periodic timer
      //				   up_period[channel],
      //				   RTDM_TIMERMODE_RELATIVE);
      if(retval)
	  rtdm_printk("PWM: error reconfiguring down-timer #%i: %i\n", 
		      channel, retval);
    }

}


void 
pwm_down(rtdm_timer_t *timer)
{
  //clear_data_out has offset 0x90
  iowrite32(0x40000000, gpio + 0x6090);
}


void 
setpwmwidth(int channel, int percentage)
{
  //rtdm_printk("PWM: %i -> %i\n", channel, percentage);

  up_period[channel] = 1000 * RANGE_MAP100(ranges[channel][0], 
					   ranges[channel][1], 
					   percentage);
  reconfigured[channel] = 1;
}


nanosecs_rel_t 
getpwmwidth(int channel)
{
  return up_period[channel];
}


int 
initpwm(pwm_desc_t *channels, int nchannels)
{
  int i;
  int retval;
  void __iomem *pinconf;

  if(nchannels < 0 || nchannels > 8)
    {
      rtdm_printk("PWM: number of channels should be between 1 and 8\n");
      return 1;
    }

  RC_NUM = nchannels;
  for(i = 0; i < RC_NUM; i++)
    {
      ranges[channels[i].channel][0] = channels[i].pwmMinWidth;
      ranges[channels[i].channel][1] = channels[i].pwmMaxWidth;
      up_period[channels[i].channel] = 
        1000 * RANGE_MAP100(ranges[i][0], ranges[i][1], 50);
      reconfigured[i] = 0;
    }

  rtdm_printk("PWM: pulse lengths initialized\n");

  rtdm_printk("PWM: configuring I/O pads mode\n");
  pinconf = ioremap(0x48000000, 0x05cc);
  if(!pinconf)
  {
    rtdm_printk("PWM: pinconf mapping failed\n");
    return 0;
  }
  // set lower 16 pins to GPIO bank5
  // (EN | PTD | M4) is 0x0C (0b01100)
  iowrite16((EN | PTD | M4), pinconf + 0x2190);
  iounmap(pinconf);

  rtdm_printk("PWM: configuring GPIO bank 5\n");

  gpio =
    ioremap(0x49050000, 0x05cc);
  if(!gpio)
    {
      rtdm_printk("PWM: GPIO mapping failed\n");
      return 0;
    }

  // 0x4000 0000 - bit 30 is set
  // 0x8000 0000 - bit 31 is set

  // First set all output on bank5 to high
  // (set_data_out has offset 0x94)
  iowrite32(0xFFFFFFFF, gpio + 0x6094);

  // Configure low 16 GPIO pins on bank 5 as output.
  // GPIO 5 is at physical address 0x49056000 = 0x49050000+0x6000
  // GPIO Output enable (GPIO_OE) is offset by 0x34 for each bank
  // (set low for output)
  iowrite32(0x00000000, gpio + 0x6034);
  // Also disable the wakeupenable and irqenable intertupts
  // GPIO clear_Wakeupenable is offset by 0x80 for each bank
  iowrite32(0x0000FFFF, gpio + 0x6080);

  // GPIO clear_irqenable1 is offset by 0x60 for each bank
  iowrite32(0x0000FFFF, gpio + 0x6060);
  // GPIO clear_irqenable2 is offset by 0x70 for each bank
  iowrite32(0x0000FFFF, gpio + 0x6070);

  rtdm_printk("PWM: Starting PWM generation timers.\n");

  retval = rtdm_timer_init(&up_timer, pwm_up, "up timer");
  if(retval)
    {
      rtdm_printk("PWM: error initializing up-timer: %i\n", retval);
      return retval;
    }

  for(i = 0; i < RC_NUM; i++)
    {
      retval = rtdm_timer_init(&down_timer[i], pwm_down, "down timer");
      if(retval)
	{
	  rtdm_printk("PWM: error initializing down-timer #%i: %i\n", i, retval);
	  return retval;
	}
    }

  retval = rtdm_timer_start(&up_timer, 
			    20000000, // we will use periodic timer
			    20000000, // 20ms period
			    RTDM_TIMERMODE_RELATIVE);
  if(retval)
    {
      rtdm_printk("PWM: error starting up-timer: %i\n", retval);
      return retval;
    }
  rtdm_printk("PWM: timers created\n");

  return 0;
}


void 
cleanuppwm(void)
{
  int i = 0;
  rtdm_timer_destroy(&up_timer);
  for(; i < RC_NUM; ++i)
    rtdm_timer_destroy(&down_timer[i]);
  iounmap(gpio);
}
