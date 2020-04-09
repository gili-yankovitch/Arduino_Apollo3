#ifndef __BLE_APP_CONFIG_H__
#define __BLE_APP_CONFIG_H__

//#include <stdbool.h>
#include <stdint.h>

typedef unsigned char bool_t;

#define PRIVATE_KEY_SIZE 32
#define PUBLIC_KEY_SIZE 64
#define SIGNATURE_SIZE 64

struct app_config_s
{
	bool key_created;
#ifndef ATECC_EEPROM_CONFIG
	/* Store private key locally */
	uint8_t encrypted_private_key[PRIVATE_KEY_SIZE];
	uint8_t public_key[PUBLIC_KEY_SIZE];
#endif
};

#endif
