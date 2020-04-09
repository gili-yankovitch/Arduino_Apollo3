#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>
#include <am_util_debug.h>
#include <aes.h>
#include <uECC.h>
#include "ble_config.h"
//#include "aes_cbc.h"
//#include "ble_device_init.h"
//#include "ble_server.h"
//#include "ble_app_protocol.h"

#define uprintf am_util_debug_printf

#define CONN_CONFIG_EEPROM_ADDR 0
#define NUM_OF_BANKS 3
#define NEXT_BANK(bank)	(((bank) + 1) % NUM_OF_BANKS)
#define PREV_BANK(bank)	(((bank) + NUM_OF_BANKS - 1) % NUM_OF_BANKS)

static int flush_bank = 0;
static int modifiable_bank = 1;
static struct conn_config_s conn_config[NUM_OF_BANKS]; /* Always have two banks. One for reading and one for writing! */

struct conn_config_s * bleGetConfig()
{
	return &conn_config[modifiable_bank];
}

struct app_config_s * bleGetAppConfig()
{
	return &conn_config[modifiable_bank].app_config;
}

void bleFlushConfig()
{
	/* Transfer for one moment to the old version */
	modifiable_bank = PREV_BANK(modifiable_bank);

	/* MODIFIABLE_BANK now points to an OLD COPY */
	flush_bank = NEXT_BANK(flush_bank);

	/* Copy flushable data to working copy */
	memcpy(&conn_config[NEXT_BANK(flush_bank)], &conn_config[flush_bank], sizeof(struct conn_config_s));

	/* Now modifies will work on the new data */
	modifiable_bank = NEXT_BANK(flush_bank);

	/* Fix the last one too */
	memcpy(&conn_config[NEXT_BANK(modifiable_bank)], &conn_config[flush_bank], sizeof(struct conn_config_s));
}

void bleConfigInit()
{
	int i;

	for (i = 0; i < sizeof(struct conn_config_s); ++i)
		*((uint8_t *)&conn_config[0] + i) = eeprom_read(CONN_CONFIG_EEPROM_ADDR + i);

	for (i = 1; i < NUM_OF_BANKS; ++i)
	{
		memcpy(&conn_config[i], &conn_config[0], sizeof(struct conn_config_s));
	}
}

extern "C" void bleConfigTask()
{
	/* Write to flash */
	writeBlockToEEPROM(CONN_CONFIG_EEPROM_ADDR, (uint8_t *)&conn_config[flush_bank], sizeof(struct conn_config_s));
}
