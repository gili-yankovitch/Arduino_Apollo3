#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>

//*****************************************************************************
//
// Sleep function called from FreeRTOS IDLE task.
// Do necessary application specific Power down operations here
// Return 0 if this function also incorporates the WFI, else return value same
// as idleTime
//
//*****************************************************************************
extern "C" uint32_t am_freertos_sleep(uint32_t idleTime)
{
	am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
	return 0;
}

//*****************************************************************************
//
// Recovery function called from FreeRTOS IDLE task, after waking up from Sleep
// Do necessary 'wakeup' operations here, e.g. to power up/enable peripherals etc.
//
//*****************************************************************************
extern "C"  void am_freertos_wakeup(uint32_t idleTime)
{
	return;
}

//*****************************************************************************
//
// FreeRTOS debugging functions.
//
//*****************************************************************************
extern "C" void vApplicationMallocFailedHook()
{
	Serial.println("Something failed...\n");

	while (1) ;
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    (void) pcTaskName;
    (void) pxTask;

    //
    // Run time stack overflow checking is performed if
    // configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    // function is called if a stack overflow is detected.
    //
    while (1)
    {
        __asm("BKPT #0\n") ; // Break into the debugger
    }
}
