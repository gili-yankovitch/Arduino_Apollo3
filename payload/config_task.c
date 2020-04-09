#include <stdint.h>
#include <am_util_delay.h>
#include "ble_freertos.h"
//#include "wsf_types.h"
//#include "wsf_trace.h"
//#include "wsf_buf.h"
//#include "wsf_timer.h"
#include "ble_config.h"
#include "ble_deepsleep.h"

#define EPOCH_DEEPSLEEP 1
#define EPOCH_CONFIG	5
static uint32_t jiffies = 0;

TaskHandle_t config_task_handle;

void ConfigTaskSetup()
{
	bleConfigInit();
}

void ConfigTask(void * param)
{
	while (true)
	{
		if (jiffies % EPOCH_CONFIG == 0)
		{
			bleConfigTask();
		}
		else if (jiffies % EPOCH_DEEPSLEEP == 0)
		{
			bleDeepSleepC();
		}

		/* Wait at least a second */
		am_util_delay_ms(1000);

		++jiffies;
	}
}
