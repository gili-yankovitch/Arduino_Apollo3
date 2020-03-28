#ifndef __BLE_APP_PROTOCOL_H__
#define __BLE_APP_PROTOCOL_H__

#include <uECC.h>
#include <stdint.h>

#define PRIVATE_KEY_SIZE 32
#define PUBLIC_KEY_SIZE 64

struct app_config_s
{
	bool_t key_created;
#ifndef ATECC_EEPROM_CONFIG
	/* Store private key locally */
	uint8_t encrypted_private_key[PRIVATE_KEY_SIZE];
	uint8_t public_key[PUBLIC_KEY_SIZE];
#endif
};

void bleAppInit();

void bleProtocolHandler(dmConnId_t connId, uint8_t * pkt, uint16_t len);

#endif
