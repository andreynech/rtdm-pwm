#ifndef __PWM_TASK_PROC_H
#define __PWM_TASK_PROC_H

typedef struct pwm_desc_s
{
  int channel;
  int pwmMinWidth;
  int pwmMaxWidth;
} pwm_desc_t;

/**
 * Initialize PWM generation infrastructure.
 *
 * @param nchannels - number of required channels. The value must be
 * between 1 and 8.
 *
 * @return 0 on success or error code in case of failure.
 */
int initpwm(pwm_desc_t *channels, int nchannels);


/**
 * Set the width of the pulse for certain channel.
 *
 * @param channel - specify the output channel to work with.
 *
 * @param percentage - pulse width specified in percents of the
 * maximal allowed width. Should be in range between 0 and 100. The
 * actual width which will be set is implementation specific and may
 * vary depending on devices to be controlled. Current implementation
 * will set the width in range between 600 and 2000 usec which is the
 * typical range for model servos such as for example Futaba servos.
 */
void setpwmwidth(int channel, int percentage);


/**
 * Return current PWM width in percents of the maximum width.
 *
 * @param channel - specify the output channel of interest.
 *
 * @return PWM width withing 0 to 100 range
 */
nanosecs_rel_t getpwmwidth(int channel);


/**
 * Release acquired resources and stops real-time threads started for
 * PWM generation.
 */
void cleanuppwm(void);

#endif
