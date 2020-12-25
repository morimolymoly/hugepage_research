#ifndef HUGE_CMD_H_
#define HUGE_CMD_H_

#include <linux/ioctl.h>

struct value {
	unsigned long addr;
	unsigned int size;
};

#define HUGE_IOC_TYPE 'M'
#define HUGE_SET_VALUE _IOW(HUGE_IOC_TYPE, 1, struct value)
#define HUGE_TIME1GB _IO(HUGE_IOC_TYPE, 2)
#define HUGE_TIME2MB _IO(HUGE_IOC_TYPE, 3)
#define HUGE_TIME4KB _IO(HUGE_IOC_TYPE, 4)
#define HUGE_WALK _IO(HUGE_IOC_TYPE, 5)
#endif
