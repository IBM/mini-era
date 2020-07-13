#ifndef _GET_INIT_TIME_H_
#define _GET_INIT_TIME_H_

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#include <stdint.h>
#ifndef __user
#define __user
#endif
#endif /* __KERNEL__ */

#include <esp.h>
#include <esp_accelerator.h>

/* <<--params-def-->> */

/* <<--params-->> */

struct accelHW_access {
	struct esp_access esp;
	/* <<--regs-->> */
        unsigned mem_size;
};

#define ACCELHW_IOC_ACCESS	_IOW ('S', 0, struct accelHW_access)


#endif /* _GET_INIT_TIME_H_ */
