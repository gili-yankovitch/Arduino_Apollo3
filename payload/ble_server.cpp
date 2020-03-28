#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>
#include <SparkFun_ATECCX08a_Arduino_Library.h> // Click here to get the library: http://librarymanager/All#SparkFun_ATECCX08a
#include <am_util_debug.h>
#include <dm_api.h>
#include <gp/gp_api.h>
#include <aes.h>
#include <uECC.h>
#include "aes_cbc.h"
#include "ble_usr.h"
#include "ble_device_init.h"
#include "ble_server.h"
#include "ble_app_protocol.h"

#define uprintf am_util_debug_printf

// #define ATECC_EEPROM_CONFIG
// #define GENERATE_NEW_PASSWORD

#ifndef ATECC_EEPROM_CONFIG
#include "sha256.h"
#endif

#define PASSWORD_LENGTH			16
#define CONN_KEY_ADDR			EEPROM_DATA_ADDRESS(2, 0, 0)
#define CONN_CONFIG_ADDR		EEPROM_DATA_ADDRESS(2, 1, 0)
#define CONN_CONFIGURED_MAGIC	0x42
#define CONN_CONECTED_MAGIC		0x43

ATECCX08A atecc;

static struct conn_config_s
{
	uint32_t configured;
	uint8_t password[PASSWORD_LENGTH + 1];
#ifndef ATECC_EEPROM_CONFIG
	uint8_t aes_key[AES256_KEY_SIZE];
	uint8_t aes_iv[AES_BLOCK_SIZE];
#endif
	struct  app_config_s app_config;
} conn_config;

#define CONN_CONFIG_EEPROM_ADDR 0

struct app_config_s * bleGetAppConfig()
{
	return &conn_config.app_config;
}

void bleFlushConfig()
{
	/* Write to flash */
	writeBlockToEEPROM(CONN_CONFIG_EEPROM_ADDR, (uint8_t *)&conn_config, sizeof(conn_config));
}

void bleSHA256(uint8_t * data, size_t len, uint8_t * hash)
{
#ifdef ATECC_EEPROM_CONFIG
	if (!atecc.sha256(data, len, hash))
	{
		uprintf("Failed on SHA256.\r\n");
	}
#else
	sha256_ctx_t hash_ctx;

	sha256_init(&hash_ctx);
	sha256_update(&hash_ctx, data, len);
	sha256_final(&hash_ctx, hash);
#endif
}

bool_t bleSendEncrypted(dmConnId_t connId, uint8_t * pkt, uint16_t len)
{
	bool_t err = false;
	uint8_t padded_res[MAX_MSG_SIZE];
	uint8_t encrypted_res[MAX_MSG_SIZE];
	uint8_t pad = 0;

	/* Reset buffer */
	memset(padded_res, 0, MAX_MSG_SIZE);

	/* Add padding field */
	len++;

	/* Make sure packet is padded */
	if (len % AES_BLOCK_SIZE)
		pad = AES_BLOCK_SIZE - (len % AES_BLOCK_SIZE);

	padded_res[0] = pad;
	memcpy(&padded_res[1], pkt, len - 1);

	if (!aes256_cbc_encrypt(conn_config.aes_key, conn_config.aes_iv, padded_res, len, encrypted_res))
		goto error;

	/* Send the encrypted packet */
	bleSendFragmented(connId, encrypted_res, len + pad);

	err = true;
error:
	return err;
}

void bleServer(dmConnId_t connId, uint8_t * pkt, uint16_t len)
{
	uint8_t plain_req[MAX_MSG_SIZE];

	memset(plain_req, 0, MAX_MSG_SIZE);

	/* Decrypt the packet */
	if (!aes256_cbc_decrypt(conn_config.aes_key, conn_config.aes_iv, pkt, len, plain_req))
		goto error;

	// uprintf("Decryption done.\r\n");

	/* Call upper layer */
	bleProtocolHandler(connId, plain_req + 1, len - plain_req[0] /* Padding field */);

error:
	return;
}

void bleServerInit()
{
	int i;
	uint8_t charMap[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	uint8_t hash[SHA256_SIZE];

#ifdef ATECC_EEPROM_CONFIG
	Wire.begin();

	if (!atecc.begin())
		uprintf("Failed initializing I2C\r\n");

	uprintf("Successful wakeUp(). I2C connections are good.\r\n");

	/* Generate an encryption key */
	if (!atecc.updateRandom32Bytes(false))
	{
		uprintf("Error updating random\r\n");

		goto error;
	}

	if (!atecc.readConfigZone(false))
	{
		uprintf("Error reading config zone.\r\n");

		goto error;
	}

	uprintf("Serial: ")
	for (i = 0; i < sizeof(atecc.serialNumber); ++i)
	{
		if (atecc.serialNumber[i] >> 4 == 0)
			uprintf("0");

		uprintf("%x", atecc.serialNumber[i]);
	}
	uprintf("\r\n");

	uprintf("configLockStatus: %s\r\n", atecc.configLockStatus ? "True" : "False");
	uprintf("dataOTPLockStatus: %s\r\n", atecc.dataOTPLockStatus ? "True" : "False");
	uprintf("slotsLockStatus: 0x%x%x\r\n", atecc.configZone[CONFIG_ZONE_SLOTS_LOCK1], atecc.configZone[CONFIG_ZONE_SLOTS_LOCK0]);

	i = 0;
	{
		uprintf("Slot %d KeyConfig: 0x%x\r\n", i, atecc.KeyConfig[i]);
		uprintf("Slot %d config: 0x%x\r\n", i, atecc.SlotConfig[i]);
		uprintf("\tWrite Config: 0x%x\r\n", WRITE_CONFIG(atecc.SlotConfig[i]));
		uprintf("\tWrite Key: 0x%x\r\n", WRITE_KEY(atecc.SlotConfig[i]));
		uprintf("\tIs Secret: 0x%x\r\n", IS_SECRET(atecc.SlotConfig[i]));
		uprintf("\tEncrypt Read: 0x%x\r\n", ENCRYPT_READ(atecc.SlotConfig[i]));
		uprintf("\tLimited Use: 0x%x\r\n", LIMITED_USE(atecc.SlotConfig[i]));
		uprintf("\tNo Mac: 0x%x\r\n", NO_MAC(atecc.SlotConfig[i]));
		uprintf("\tRead Key: 0x%x\r\n", READ_KEY(atecc.SlotConfig[i]));
	}
#endif

	memset(&conn_config, 0, sizeof(conn_config));

	/* Read Conn Encryption configuration */
#ifdef ATECC_EEPROM_CONFIG
	/* Read configure status */
	for (i = 0; i < sizeof(conn_config.configured); ++i)
		*((uint8_t *)&conn_config.configured + i) = eeprom_read(CONN_CONFIG_EEPROM_ADDR + i);
#else
	/* Read config */
	for (i = 0; i < sizeof(conn_config); ++i)
		*((uint8_t *)&conn_config + i) = eeprom_read(CONN_CONFIG_EEPROM_ADDR + i);
#endif

#ifndef GENERATE_NEW_PASSWORD
	/* Read data */
	if ((conn_config.configured == CONN_CONFIGURED_MAGIC) || (conn_config.configured == CONN_CONECTED_MAGIC))
	{
#ifdef ATECC_EEPROM_CONFIG
		uprintf("Reading from Crypto chip EEPROM\r\n");

		/* If only configured, read password from EEPROM */
		if (conn_config.configured == CONN_CONFIGURED_MAGIC)
		{
			for (i = 0; i < sizeof(conn_config.password); ++i)
				conn_config.password[i] = eeprom_read(CONN_CONFIG_EEPROM_ADDR + sizeof(conn_config.configured) + i);
		}

		/* Read conn key */
		if (!atecc.read_output(ZONE_DATA, CONN_KEY_ADDR, sizeof(conn_config.aes_key), conn_config.aes_key, false))
		{
			uprintf("Error reading from Crypto Chip EEPROM: Connection Key\r\n");

			goto error;
		}
#endif
	}
	else
#endif
	{
#ifdef ATECC_EEPROM_CONFIG
		uint8_t aes_key_write[AES256_KEY_SIZE];

		for (i = 0; i < PASSWORD_LENGTH; ++i)
		{
			conn_config.password[i] = charMap[atecc.random32Bytes[i] % (sizeof(charMap) - 1)];
		}

		conn_config.password[PASSWORD_LENGTH] = '\0';

#else
		randomSeed(analogRead(0));

		for (i = 0; i < PASSWORD_LENGTH; ++i)
		{
			conn_config.password[i] = charMap[random(255) % (sizeof(charMap) - 1)];
		}

		conn_config.password[PASSWORD_LENGTH] = '\0';
#endif
		/* Hash the password */
		bleSHA256(conn_config.password, PASSWORD_LENGTH, conn_config.aes_key);

		/* Mark as configured */
		conn_config.configured = CONN_CONFIGURED_MAGIC;

#ifdef ATECC_EEPROM_CONFIG
		if (!atecc.configLockStatus)
		{
			if (!initAteccEEPROM())
			{
				goto error;
			}
		}

		uprintf("Writing connection key to crypto chip\r\n");

		/* Write key */
		if (!atecc.write(ZONE_DATA, CONN_KEY_ADDR, conn_config.aes_key, sizeof(conn_config.aes_key)))
		{
			uprintf("Failed writing encryption key to EEPROM\r\n");

			goto error;
		}

		/* Read conn key to verify write*/
		if (!atecc.read_output(ZONE_DATA, CONN_KEY_ADDR, sizeof(aes_key_write), aes_key_write, false))
		{
			uprintf("Error reading from Crypto Chip EEPROM: Connection Key\r\n");

			goto error;
		}

		/* This is impossible because slots need to be locked before READ is capable from DATA slots. */
		/* Slots will be locked only after key generation */
#if 0
		/* Verify write was successful*/
		if (memcmp(conn_config.aes_key, aes_key_write, AES256_KEY_SIZE) != 0)
		{
			uprintf("ERROR: Connection key was not written properly to crypto chip!\r\n");
		}
#endif
#endif
		/* Write to device storage */
		/* Erase password on first connection! */
		bleFlushConfig();
	}

	/* Before connected, print the password */
	if (conn_config.configured == CONN_CONFIGURED_MAGIC)
	{
		uprintf("Password: %s\r\n", conn_config.password);
	}

	uprintf("AES Key: ");
	for (i = 0; i < AES256_KEY_SIZE; ++i)
	{
		if (conn_config.aes_key[i] >> 4 == 0)
			uprintf("0");

		uprintf("%x ", conn_config.aes_key[i]);
	}
	uprintf("\r\n");

	/* Call application init */
	bleAppInit();
error:
	return;
}
