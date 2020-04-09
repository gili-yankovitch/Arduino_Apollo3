#include <Arduino.h>
#include <am_util_debug.h>
#define uprintf am_util_debug_printf

#define SECONDS_TO_SLEEP 120
static uint32_t jiffies = 0;

void bleKickDeepSleep()
{
	jiffies = 0;
}

/* Taken from here https://github.com/sparkfun/Arduino_Apollo3/blob/master/libraries/Examples/examples/Advanced/LowPower/LowPower.ino */
void bleDeepSleep()
{
  // Stop the XTAL.
  am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_XTAL_STOP, 0);

  // Disable the RTC.
  am_hal_rtc_osc_disable();

  // Disabling the debugger GPIOs saves about 1.2 uA total:
  am_hal_gpio_pinconfig(20 /* SWDCLK */, g_AM_HAL_GPIO_DISABLE);
  am_hal_gpio_pinconfig(21 /* SWDIO */, g_AM_HAL_GPIO_DISABLE);

  // These two GPIOs are critical: the TX/RX connections between the Artemis module and the CH340S on the Blackboard
  // are prone to backfeeding each other. To stop this from happening, we must reconfigure those pins as GPIOs
  // and then disable them completely:
  am_hal_gpio_pinconfig(48 /* TXO-0 */, g_AM_HAL_GPIO_DISABLE);
  am_hal_gpio_pinconfig(49 /* RXI-0 */, g_AM_HAL_GPIO_DISABLE);

  // The default Arduino environment runs the System Timer (STIMER) off the 48 MHZ HFRC clock source.
  // The HFRC appears to take over 60 uA when it is running, so this is a big source of extra
  // current consumption in deep sleep.
  // For systems that might want to use the STIMER to generate a periodic wakeup, it needs to be left running.
  // However, it does not have to run at 48 MHz. If we reconfigure STIMER (system timer) to use the 32768 Hz
  // XTAL clock source instead the measured deepsleep power drops by about 64 uA.
  am_hal_stimer_config(AM_HAL_STIMER_CFG_CLEAR | AM_HAL_STIMER_CFG_FREEZE);

  // This option selects 32768 Hz via crystal osc. This appears to cost about 0.1 uA versus selecting "no clock"
  am_hal_stimer_config(AM_HAL_STIMER_XTAL_32KHZ);

  // This option would be available to systems that don't care about passing time, but might be set
  // to wake up on a GPIO transition interrupt.
  // am_hal_stimer_config(AM_HAL_STIMER_NO_CLK);

  // Turn OFF Flash1
  if (am_hal_pwrctrl_memory_enable(AM_HAL_PWRCTRL_MEM_FLASH_512K))
  {
    while (1)
      ;
  }

  // Power down SRAM
  PWRCTRL->MEMPWDINSLEEP_b.SRAMPWDSLP = PWRCTRL_MEMPWDINSLEEP_SRAMPWDSLP_ALLBUTLOWER32K;

  // am_hal_gpio_pinconfig(19, g_AM_HAL_GPIO_OUTPUT_WITH_READ_12);
  // am_hal_gpio_output_set(19);

  // Serial.println("Going to sleep...");
  
  // delay(100); //Wait for print to complete

  // Serial.end(); //Disable Serial

  am_hal_gpio_output_clear(19);

  am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
}

extern "C" void bleDeepSleepC()
{
	if (jiffies++ >= SECONDS_TO_SLEEP)
	{
		bleDeepSleep();
	}
}
