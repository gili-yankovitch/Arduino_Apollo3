#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_ATECCX08a_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_ATECCX08a
#include <am_util_debug.h>

ATECCX08A atecc;

extern "C" void serial_print()
{
	Wire.begin();

	am_util_debug_printf("%s::%d\r\n", __FILE__, __LINE__);
	if (atecc.begin())
		am_util_debug_printf("Successful wakeUp(). I2C connections are good.\r\n");
#if 0
	if (atecc.begin() == true)
	{
	}

	atecc.readConfigZone(false); // Debug argument false (OFF)

	for (int i = 0 ; i < 9 ; i++)
	{
		if ((atecc.serialNumber[i] >> 4) == 0)
			am_util_debug_printf("0");

		am_util_debug_printf("%x", atecc.serialNumber[i]);
	}
#endif
}
