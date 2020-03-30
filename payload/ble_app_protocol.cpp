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
#include "ble_config.h"
#include "ble_usr.h"
#include "ble_device_init.h"
#include "ble_server.h"
#include "ble_app_protocol.h"

#define uprintf am_util_debug_printf

#define PIN_SALT "salt"
#define PIN_SIZE 6
#define MAX_REQ_SIZE MAX_MSG_SIZE
#define MAX_RSP_SIZE MAX_MSG_SIZE

/* Offsets */
#define REQ_SIZE_CMD	1
#define REQ_OFFSET_CMD	0
#define REQ_SIZE_PIN	PIN_SIZE
#define REQ_OFFSET_PIN	(REQ_OFFSET_CMD + REQ_SIZE_CMD)

#define RSP_SIZE_CMD	1
#define RSP_OFFSET_CMD	0
#define RSP_SIZE_KEY	PUBLIC_KEY_SIZE
#define RSP_OFFSET_KEY	(RSP_OFFSET_CMD + RSP_SIZE_CMD)

/* All fields are 1 bytes size to avoid and endianess confusion */
#define BLE_PKT_FIELD(pkt, FIELD)	*((uint8_t *)pkt + (FIELD))
#define BLE_PKT_FIELD_PTR(pkt, FIELD)	((uint8_t *)pkt + (FIELD))

enum req_e
{
	E_REQ_KEY
};

enum rsp_e
{
	E_RSP_KEY,
	E_RSP_ERR
};

struct bleRequest
{
	enum req_e cmd;
	uint8_t pin[PIN_SIZE];
};

struct bleResponse
{
	enum rsp_e cmd;

	union
	{
		struct
		{
			uint8_t public_key[PUBLIC_KEY_SIZE];
		} key;
	} data;
};

static bool_t bleRequestParse(uint8_t * pkt, uint16_t len, struct bleRequest * r)
{
	bool_t err = false;
	uint8_t field;

	/* Is it at least CMD size? */
	if (len < REQ_SIZE_CMD)
		goto error;

	/* Get command */
	r->cmd = (enum req_e) BLE_PKT_FIELD(pkt, REQ_OFFSET_CMD);

	/* Copy the pin */
	memcpy(r->pin, BLE_PKT_FIELD_PTR(pkt, REQ_OFFSET_PIN), REQ_SIZE_PIN);

	/* First, read the command */
	switch (r->cmd)
	{
		case E_REQ_KEY:
		{
			/* No additional processing */

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
		case E_REQ_KEY:
		{
			memcpy(BLE_PKT_FIELD_PTR(buf, RSP_OFFSET_KEY), r->data.key.public_key, RSP_SIZE_KEY);
			size += RSP_SIZE_KEY;

			if (size > max_len)
				goto error;

			break;
		}

		default:
		{
			/* No additional parsing? */
			break;
		}
	}

	ret_size = size;
error:
	return ret_size;
}

/* Large buffers - ruining stack :( */
static uint8_t private_key[PRIVATE_KEY_SIZE];
static uint8_t pin_iv_buf[PIN_SIZE + sizeof(PIN_SALT) - 1];
static uint8_t pin_key[AES256_KEY_SIZE];
static uint8_t pin_iv[AES_BLOCK_SIZE];
static int bleWalletCreateNewKey(struct bleRequest * req)
{
	int i;
	int err = 0;
	struct app_config_s * app_config;

	uprintf("KEY CREATED ######################\r\n");

	/* Get the config */
	app_config = bleGetAppConfig();

	/* Create a new ECC secp256k1 Key */
	if (!uECC_make_key(app_config->public_key, private_key, uECC_secp256k1()))
	{
		uprintf("Error creating public key\r\n");

		goto error;
	}

	uprintf("Public key created: ");
	for (i = 0; i < PUBLIC_KEY_SIZE; ++i)
	{
		if (app_config->public_key[i] >> 4 == 0)
			uprintf("0");

		uprintf("%x ", app_config->public_key[i]);
	}
	uprintf("\r\n");

	/* Create the key encryption using the pin */
	bleSHA256(req->pin, PIN_SIZE, pin_key);

	/* Create the IV */
	memcpy(pin_iv_buf, req->pin, PIN_SIZE);
	memcpy(pin_iv_buf + PIN_SIZE, PIN_SALT, sizeof(PIN_SALT) - 1);
	bleSHA256(pin_iv_buf, sizeof(pin_iv_buf), pin_iv);

	/* Encrypt the private key */
	if (!aes256_cbc_encrypt(pin_key, pin_iv, private_key, PRIVATE_KEY_SIZE, app_config->encrypted_private_key))
		goto error;

	memset(private_key, 0, PRIVATE_KEY_SIZE);

	/* Mark key as created */
	app_config->key_created = true;

	// bleFlushConfig();

	err = 1;
error:
	return err;
}

void bleProtocolHandler(dmConnId_t connId, uint8_t * pkt, uint16_t len)
{
	struct app_config_s * app_config;
	struct bleRequest req;
	struct bleResponse rsp;
	uint8_t raw_rsp[MAX_RSP_SIZE];
	size_t raw_rsp_size;

	/* Parse the packet */
	if (!bleRequestParse(pkt, len, &req))
	{
		uprintf("Error parsing.\r\n");

		goto error;
	}

	memset(&rsp, 0, sizeof(struct bleResponse));

	switch (req.cmd)
	{
		case E_REQ_KEY:
		{
			uprintf("Got key request\r\n");

			/* Get the config */
			app_config = bleGetAppConfig();

			/* Is it configured already? */
			if (!app_config->key_created)
			{
				if (!bleWalletCreateNewKey(&req))
				{
					goto error;
				}
			}

			rsp.cmd = E_RSP_KEY;

			/* Copy public key to the response buffer */
			memcpy(rsp.data.key.public_key, app_config->public_key, PUBLIC_KEY_SIZE);

			break;
		}

		default:
		{
			goto error;
		}
	}


send:
	/* Always reset request - residue of PIN */
	memset(&req, 0, sizeof(req));

	// memset(pkt, 0, len);

	/* Serialize the response */
	if (!(raw_rsp_size = bleSerialize(&rsp, raw_rsp, MAX_RSP_SIZE)))
		goto error;

	/* Send encrypted result */
	bleSendEncrypted(connId, raw_rsp, raw_rsp_size);

	return;

error:
	rsp.cmd = E_RSP_ERR;

	goto send;
}

void bleAppInit()
{
	/* Initialize the application struct */
}
