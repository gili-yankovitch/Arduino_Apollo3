#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>
#include <SparkFun_ATECCX08a_Arduino_Library.h> // Click here to get the library: http://librarymanager/All#SparkFun_ATECCX08a
#include <am_util_debug.h>
#include <dm_api.h>
#include <gp/gp_api.h>
#include <aes.h>
#include "aes_cbc.h"
#include "ble_usr.h"

#define uprintf am_util_debug_printf

#ifdef ATECC_EEPROM_CONFIG

extern ATECCX08A atecc;

bool_t initAteccEEPROM()
{
	bool_t err = false;

	/* Slot 2 contains the connection key */
	/* Config slots 2 and 3 to be cleartext writeable */
	uint16_t slot2WriteConfig = 0; /* Cleartext possible, no restrictions */
	uint16_t slot2KeyConfig = 0;
	uint16_t slot3WriteConfig = 0;
	uint16_t slot3KeyConfig = 0;

	/* Make sure slot 8 remains clean and writeable for any case of fuck-ups to be used as general purpose storage */
	uint16_t slot8WriteConfig = 0;
	uint16_t slot8KeyConfig = 0;

	/* Slots 9 + 10 contains the ENCRYPTED wallet public(9) + private(10) keys */
	uint16_t slot9WriteConfig = 0;
	uint16_t slot9KeyConfig = 0;
	uint16_t slot10WriteConfig = 0;
	uint16_t slot10KeyConfig = 0;

	/* Slot 10 is written with slot 11, so configure it too */
	uint16_t slot11WriteConfig = 0;
	uint16_t slot11KeyConfig = 0;

	uint32_t slots23WriteConfig = 0;
	uint32_t slots23KeyConfig = 0;
	uint32_t slots89WriteConfig = 0;
	uint32_t slots89KeyConfig = 0;
	uint32_t slots1011WriteConfig = 0;
	uint32_t slots1011KeyConfig = 0;

	uprintf("Writing slot configuration...\r\n");

	slot2KeyConfig |= KEY_CONFIG_SET(1, KEY_CONFIG_OFFSET_LOCKABLE); /* Slot 0 should be lockable using the LOCK command */
	slot9KeyConfig |= KEY_CONFIG_SET(1, KEY_CONFIG_OFFSET_LOCKABLE);
	slot10KeyConfig |= KEY_CONFIG_SET(1, KEY_CONFIG_OFFSET_LOCKABLE);

	/* Attach configurations together, prepare for writes */
	slots23WriteConfig = slot2WriteConfig | (slot3WriteConfig << 16);
	slots23KeyConfig = slot2KeyConfig | (slot3KeyConfig << 16);

	slots89WriteConfig = slot8WriteConfig | (slot9WriteConfig << 16);
	slots89KeyConfig = slot8KeyConfig | (slot9KeyConfig << 16);

	slots1011WriteConfig = slot10WriteConfig | (slot11WriteConfig << 16);
	slots1011KeyConfig = slot10KeyConfig | (slot11KeyConfig << 16);

	/* Run the SparkFun config, making sure there's availability to at least on ECC key */
	if (!atecc.writeConfigSparkFun())
	{
		uprintf("Failed configuring slots 0, 1 for ECC keys\r\n");

		goto error;
	}

	/* Now, configure other slots */
	if (!atecc.write(ZONE_CONFIG, SLOT_CONFIG_ADDRESS(2), (uint8_t *)&slots23WriteConfig, sizeof(slots23WriteConfig)))
	{
		uprintf("Failed modifiying Slot Configs 2, 3\r\n");

		goto error;
	}

	if (!atecc.write(ZONE_CONFIG, KEY_CONFIG_ADDRESS(2), (uint8_t *)&slots23KeyConfig, sizeof(slots23KeyConfig)))
	{
		uprintf("Failed modifiying Key Configs 2, 3\r\n");

		goto error;
	}

	if (!atecc.write(ZONE_CONFIG, SLOT_CONFIG_ADDRESS(8), (uint8_t *)&slots89WriteConfig, sizeof(slots89WriteConfig)))
	{
		uprintf("Failed modifiying Slot Configs 8, 9\r\n");

		goto error;
	}

	if (!atecc.write(ZONE_CONFIG, KEY_CONFIG_ADDRESS(8), (uint8_t *)&slots89KeyConfig, sizeof(slots89KeyConfig)))
	{
		uprintf("Failed modifiying Key Configs 8, 9\r\n");

		goto error;
	}

	if (!atecc.write(ZONE_CONFIG, SLOT_CONFIG_ADDRESS(10), (uint8_t *)&slots1011WriteConfig, sizeof(slots1011WriteConfig)))
	{
		uprintf("Failed modifiying Slot Configs 10, 11\r\n");

		goto error;
	}

	if (!atecc.write(ZONE_CONFIG, KEY_CONFIG_ADDRESS(10), (uint8_t *)&slots1011KeyConfig, sizeof(slots1011KeyConfig)))
	{
		uprintf("Failed modifiying Key Configs 10, 11\r\n");

		goto error;
	}

	uprintf("Locking configuration!\r\n");

	/* Lock configuration */
	if (!atecc.lockConfig())
	{
		uprintf("Failed locking config!\r\n");

		goto error;
	}

	err = true;
error:
	return err;
}

#endif
