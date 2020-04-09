#ifndef __BLE_CONFIG_H__
#define __BLE_CONFIG_H__
#include "ble_app_config.h"
#include "aes_cbc.h"

#define PASSWORD_LENGTH			16
#define CONN_CONFIGURED_MAGIC	0x42
#define CONN_CONECTED_MAGIC		0x43

#ifdef __cplusplus
extern "C"
{
#endif

struct conn_config_s
{
	uint32_t configured;
	uint8_t password[PASSWORD_LENGTH + 1];
#ifndef ATECC_EEPROM_CONFIG
	uint8_t aes_key[AES256_KEY_SIZE];
	uint8_t aes_iv[AES_BLOCK_SIZE];
#endif
	struct app_config_s app_config;
};

struct conn_config_s * bleGetConfig();
struct app_config_s * bleGetAppConfig();
void bleFlushConfig();
void bleConfigInit();
void bleConfigTask();

#ifdef __cplusplus
}
#endif

#endif
