/*
 * Core Source for:
 *
 * Copyright (C) 2012 Pantech, Inc.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/version.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>

#include "tspdrv.h"

extern VibeStatus ImmVibeSPI_ForceOut_SetSamples(VibeUInt8 nActuatorIndex, VibeUInt16 nOutputSignalBitDepth, VibeUInt16 nBufferSizeInBytes, VibeInt8* pForceOutputBuffer);
extern VibeStatus ImmVibeSPI_ForceOut_AmpDisable(VibeUInt8 nActuatorIndex);

static int Atoi(const char Str[])
{
     int i ,n=0;

     for(i=0; Str[i] >= '0' && Str[i] <= '9'; ++i)
          n = 10*n + (Str[i]-'0');

     return n;
}


static long touch_fops_ioctl(struct file *filp,unsigned int cmd, unsigned long arg)
{
	int return_to_user = 0;
	return return_to_user;
}

static int motor_index = 0;
static int force_mode = false;

#define MAX_FORCE_TIME		500

static ssize_t touch_fops_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	int nBufSize=0, i=0;

	if ((size_t)(*ppos) > 0)
		return 0;

	if (buf != NULL) {
		nBufSize=strlen(buf);

		if (force_mode == true) {
			int force = 0;
			VibeUInt8 cZero[1] = {0};

			if (strncmp(buf, "force_disable", 13)==0) {
				printk("IMM force mode disable\n");
				force_mode = false;
				return nBufSize;
			}

			force = Atoi(buf);
			cZero[0] = force;

			printk("IMM force : %d\n", force);

			for (i = 0; i  <MAX_FORCE_TIME; i += 5) {
				/* Nothing to play for all actuators, turn off the timer when we reach the watchdog tick count limit */
				ImmVibeSPI_ForceOut_SetSamples(motor_index, 8, 1, cZero);
				mdelay(5);
			}
			ImmVibeSPI_ForceOut_AmpDisable(motor_index);
		}

		if (strncmp(buf, "debugon", 5) == 0)
			IMMR_DEB = true;
		else if (strncmp(buf, "index0", 6) == 0)
			motor_index = 0;
		else if (strncmp(buf, "index1", 6) == 0)
			motor_index = 1;
		else if(strncmp(buf, "force_enable", 12) == 0)
			force_mode = true;
	}

	*ppos +=nBufSize;
	return nBufSize;
}

static struct file_operations touch_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = touch_fops_ioctl,
	.write = touch_fops_write,
};

static struct miscdevice touch_fops_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "touch_fops",
	.fops = &touch_fops,
};

// call in driver init function
void touch_fops_init(void) {
	int rc;
	rc = misc_register(&touch_fops_dev);
	if (rc)
		pr_err("::::::::: can''t register touch_fops\n");
}

// call in driver remove function
void touch_fops_exit(void) {
	misc_deregister(&touch_fops_dev);
}

