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

// #define ATECC_EEPROM_CONFIG
// #define GENERATE_NEW_PASSWORD

#ifndef ATECC_EEPROM_CONFIG
#include "sha256.h"
#endif

#define CONN_KEY_ADDR			EEPROM_DATA_ADDRESS(2, 0, 0)
#define CONN_CONFIG_ADDR		EEPROM_DATA_ADDRESS(2, 1, 0)
#define CONN_CONFIGURED_MAGIC	0x42
#define CONN_CONECTED_MAGIC		0x43

#define PASSWORD_LENGTH 16
#define MAX_REQ_SIZE 240
#define MAX_RSP_SIZE 240

#define REQ_SIZE_CMD	1
#define REQ_OFFSET_CMD	0

#define RSP_SIZE_CMD	1
#define RSP_OFFSET_CMD	0
#define RSP_SIZE_SERIAL	SERIAL_NUMBER_SIZE
#define RSP_OFFSET_SERIAL	(RSP_OFFSET_CMD + RSP_SIZE_CMD)

/* All fields are 1 bytes size to avoid and endianess confusion */
#define BLE_PKT_FIELD(pkt, FIELD)	*((uint8_t *)pkt + (FIELD))
#define BLE_PKT_FIELD_PTR(pkt, FIELD)	((uint8_t *)pkt + (FIELD))

ATECCX08A atecc;

enum req_e
{
	E_REQ_SERIAL
};

enum rsp_e
{
	E_RSP_SERIAL
};

struct bleRequest
{
	enum req_e cmd;
};

struct bleResponse
{
	enum rsp_e cmd;

	union
	{
		struct
		{
			uint8_t serial_number[SERIAL_NUMBER_SIZE];
		} serial;
	} data;
};

static struct conn_config_s
{
	uint32_t configured;
	uint8_t password[PASSWORD_LENGTH + 1];
	uint8_t aes_key[AES256_KEY_SIZE];
	uint8_t aes_iv[AES_BLOCK_SIZE];
} conn_config;

#define CONN_CONFIG_EEPROM_ADDR 0

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

		/* Hash the password */
		if (!atecc.sha256(conn_config.password, PASSWORD_LENGTH, hash))
		{
			uprintf("Failed on SHA256.\r\n");

			goto error;
		}
#else
		sha256_ctx_t hash_ctx;

		randomSeed(analogRead(0));

		for (i = 0; i < PASSWORD_LENGTH; ++i)
		{
			conn_config.password[i] = charMap[random(255) % (sizeof(charMap) - 1)];
		}

		conn_config.password[PASSWORD_LENGTH] = '\0';

		sha256_init(&hash_ctx);
		sha256_update(&hash_ctx, conn_config.password, PASSWORD_LENGTH);
		sha256_final(&hash_ctx, hash);
#endif

		memcpy(conn_config.aes_key, hash, AES256_KEY_SIZE);

		/* Mark as configured */
		conn_config.configured = CONN_CONFIGURED_MAGIC;

#ifdef ATECC_EEPROM_CONFIG
		if (!atecc.configLockStatus)
		{
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

		if (memcmp(conn_config.aes_key, aes_key_write, AES256_KEY_SIZE) != 0)
		{
			uprintf("ERROR: Connection key was not written properly to crypto chip!\r\n");
		}

		/* Remember that we're already configured */
		writeBlockToEEPROM(CONN_CONFIG_EEPROM_ADDR, (uint8_t *)&conn_config.configured, sizeof(conn_config.configured));

		/* Write password until first connection - Erase afterwards! */
		writeBlockToEEPROM(CONN_CONFIG_EEPROM_ADDR + sizeof(conn_config.configured), (uint8_t *)conn_config.password, sizeof(conn_config.password));
#else
		/* Write to device storage */
		writeBlockToEEPROM(CONN_CONFIG_EEPROM_ADDR, (uint8_t *)&conn_config, sizeof(conn_config));
#endif
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

error:
	return;
}

static bool_t bleSendEncrypted(dmConnId_t connId, uint8_t * pkt, uint16_t len)
{
	bool_t err = false;
	uint8_t padded_res[MAX_RSP_SIZE];
	uint8_t encrypted_res[MAX_RSP_SIZE];
	uint8_t pad = 0;

	/* Reset buffer */
	memset(padded_res, 0, MAX_RSP_SIZE);

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

static bool_t bleRequestParse(uint8_t * pkt, uint16_t len, struct bleRequest * r)
{
	bool_t err = false;
	uint8_t field;

	/* Is it at least CMD size? */
	if (len < REQ_SIZE_CMD)
		goto error;

	field = BLE_PKT_FIELD(pkt, REQ_OFFSET_CMD);

	/* First, read the command */
	switch (field)
	{
		case E_REQ_SERIAL:
		{
			r->cmd = (enum req_e)field;

			break;
		}

		default:
		{
			goto error;
		}
	}

	err = true;
error:
	return err;
}

static size_t bleSerialize(struct bleResponse * r, uint8_t * buf, size_t max_len)
{
	size_t size = 0;
	size_t ret_size = 0;

	/* First, write the command */
	BLE_PKT_FIELD(buf, RSP_OFFSET_CMD) = r->cmd;
	size += RSP_SIZE_CMD;

	if (size > max_len)
		goto error;

	switch(r->cmd)
	{
		case E_RSP_SERIAL:
		{
			memcpy(BLE_PKT_FIELD_PTR(buf, RSP_OFFSET_SERIAL), r->data.serial.serial_number, RSP_SIZE_SERIAL);
			size += RSP_SIZE_SERIAL;

			if (size > max_len)
				goto error;

			break;
		}

		default:
		{
			/* No such command? */
			break;
		}
	}

	ret_size = size;
error:
	return ret_size;
}

static void bleProtocolHandler(dmConnId_t connId, uint8_t * pkt, uint16_t len)
{
	struct bleRequest req;
	struct bleResponse rsp;
	uint8_t raw_rsp[MAX_RSP_SIZE];
	size_t raw_rsp_size;

	/* Parse the packet */
	if (!bleRequestParse(pkt, len, &req))
		goto error;

	memset(&rsp, 0, sizeof(struct bleResponse));

	switch (req.cmd)
	{
		case E_REQ_SERIAL:
		{
			atecc.readConfigZone(false); // Debug argument false (OFF)

			uprintf("Requested Serial: ");

			for (int i = 0 ; i < SERIAL_NUMBER_SIZE ; i++)
			{
				if ((atecc.serialNumber[i] >> 4) == 0)
					uprintf("0");

				uprintf("%x", atecc.serialNumber[i]);
			}

			uprintf("\r\n");

			/* Create the response packet */
			rsp.cmd = E_RSP_SERIAL;
			memcpy(rsp.data.serial.serial_number, atecc.serialNumber, SERIAL_NUMBER_SIZE);

			break;
		}

		default:
		{
			goto error;
		}
	}

	/* Serialize the response */
	if (!(raw_rsp_size = bleSerialize(&rsp, raw_rsp, MAX_RSP_SIZE)))
		goto error;

	/* Send encrypted result */
	bleSendEncrypted(connId, raw_rsp, raw_rsp_size);

error:
	return;
}

void bleServer(dmConnId_t connId, uint8_t * pkt, uint16_t len)
{
	uint8_t plain_req[MAX_REQ_SIZE];

	memset(plain_req, 0, MAX_REQ_SIZE);

	/* Decrypt the packet */
	if (!aes256_cbc_decrypt(conn_config.aes_key, conn_config.aes_iv, pkt, len, plain_req))
		goto error;

	uprintf("Decryption done.\r\n");

	/* Call upper layer */
	bleProtocolHandler(connId, plain_req + 1, len - plain_req[0] /* Padding field */);

error:
	return;
}
