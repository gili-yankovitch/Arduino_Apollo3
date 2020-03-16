#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_ATECCX08a_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_ATECCX08a
#include <am_util_debug.h>

ATECCX08A atecc;

extern "C" void serial_print()
{
	Wire.begin();

	if (!atecc.begin())
		am_util_debug_printf("Failed initializing I2C\r\n");

	am_util_debug_printf("Successful wakeUp(). I2C connections are good.\r\n");

	atecc.readConfigZone(false); // Debug argument false (OFF)

	am_util_debug_printf("Serial: ");

	for (int i = 0 ; i < 9 ; i++)
	{
		if ((atecc.serialNumber[i] >> 4) == 0)
			am_util_debug_printf("0");

		am_util_debug_printf("%x", atecc.serialNumber[i]);
	}

	am_util_debug_printf("\r\n");
}
