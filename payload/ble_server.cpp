#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_ATECCX08a_Arduino_Library.h> // Click here to get the library: http://librarymanager/All#SparkFun_ATECCX08a
#include <am_util_debug.h>
#include <dm_api.h>
#include <gp/gp_api.h>
#include <aes.h>
#include "aes_cbc.h"
#include "ble_usr.h"

#define uprintf am_util_debug_printf

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

static uint8_t aes_key[AES256_KEY_SIZE];
static uint8_t aes_iv[AES_BLOCK_SIZE] = { 0 };

void bleServerInit()
{
	int i;
	uint8_t charMap[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	uint8_t password[PASSWORD_LENGTH + 1];
	uint8_t hash[SHA256_SIZE];

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

	for (i = 0; i < PASSWORD_LENGTH; ++i)
	{
		password[i] = charMap[atecc.random32Bytes[i] % sizeof(charMap)];
	}

	password[PASSWORD_LENGTH] = '\0';

	uprintf("Password: %s\r\n", password);

	/* Hash the password */
	if (!atecc.sha256(password, PASSWORD_LENGTH, hash))
	{
		uprintf("Failed on SHA256.\r\n");

		goto error;
	}

	memcpy(aes_key, hash, AES256_KEY_SIZE);
#if 0
	// uprintf("SHA256: ");

	for (i = 0; i < SHA256_SIZE; ++i)
	{
		aes_iv[i % AES_BLOCK_SIZE] ^= hash[i];

		// if (hash[i] >> 4 == 0)
		// 	uprintf("0");
		// uprintf("%x", hash[i]);
	}

	// uprintf("\r\n");
#endif
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

	if (!aes256_cbc_encrypt(aes_key, aes_iv, padded_res, len, encrypted_res))
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
	memset(aes_iv, 0, AES_BLOCK_SIZE);

	/* Decrypt the packet */
	if (!aes256_cbc_decrypt(aes_key, aes_iv, pkt, len, plain_req))
		goto error;

	uprintf("Decryption done.\r\n");

	/* Call upper layer */
	bleProtocolHandler(connId, plain_req + 1, len - plain_req[0] /* Padding field */);

error:
	return;
}
