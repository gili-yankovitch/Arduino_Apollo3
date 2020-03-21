#include <stdint.h>
#include <stdbool.h>
// #include <stdlib.h>
#include <string.h>
#include <aes.h>
#include "aes_cbc.h"

#include <Arduino.h>
#include <am_util_debug.h>
#define uprintf am_util_debug_printf

bool aes256_cbc_encrypt(uint8_t key[AES256_KEY_SIZE], uint8_t iv[AES_BLOCK_SIZE], uint8_t * plaintext, size_t len, uint8_t * ciphertext)
{
	bool err = false;
	aes_256_context_t context;
	uint8_t cv[AES_BLOCK_SIZE];
	int i;
	int j;

	/* Length must be aligned! */
	if (len % AES_BLOCK_SIZE)
		goto error;

	/* Initialize */
	memcpy(cv, iv, AES_BLOCK_SIZE);
	aes_256_init(&context, key);

	for (i = 0; i < len / AES_BLOCK_SIZE; ++i)
	{
		for (j = 0; j < AES_BLOCK_SIZE; ++j)
			cv[j] ^= plaintext[i * AES_BLOCK_SIZE + j];

		/* Encrypt */
		aes_256_encrypt(&context, cv);

		/* Copy to vector */
		memcpy(&ciphertext[i * AES_BLOCK_SIZE], cv, AES_BLOCK_SIZE);
	}

	err = true;
error:
	return err;
}

bool aes256_cbc_decrypt(uint8_t key[AES256_KEY_SIZE], uint8_t iv[AES_BLOCK_SIZE], uint8_t * ciphertext, size_t len, uint8_t * plaintext)
{
	bool err = false;
	aes_256_context_t context;
	uint8_t cv[AES_BLOCK_SIZE];
	int i;
	int j;

	/* Length must be aligned! */
	if (len % AES_BLOCK_SIZE)
		goto error;

	/* Initialize */
	memcpy(cv, iv, AES_BLOCK_SIZE);
	aes_256_init(&context, key);
#if 0
	uprintf("AES Key: ");
	for (i = 0; i < AES256_KEY_SIZE; ++i)
	{
		if (key[i] >> 4 == 0)
			uprintf("0");

		uprintf("%x ", key[i]);
	}

	uprintf("\r\n");

	uprintf("IV: ");
	for (i = 0; i < AES_BLOCK_SIZE; ++i)
	{
		if (iv[i] >> 4 == 0)
			uprintf("0");

		uprintf("%x ", iv[i]);
	}

	uprintf("\r\n");

	uprintf("Ciphertext len: %u Blocks: %u\r\n", len, len / AES_BLOCK_SIZE);

	uprintf("Ciphertext: ");
	for (i = 0; i < len; ++i)
	{
		if (ciphertext[i] >> 4 == 0)
			uprintf("0");

		uprintf("%x ", ciphertext[i]);
	}

	uprintf("\r\n");
#endif
	for (i = 0; i < len / AES_BLOCK_SIZE; ++i)
	{
		memcpy(&plaintext[i * AES_BLOCK_SIZE], &ciphertext[i * AES_BLOCK_SIZE], AES_BLOCK_SIZE);

		/* Decrypt */
		aes_256_decrypt(&context, &plaintext[i * AES_BLOCK_SIZE]);

#if 0
		uprintf("Plaintext: ");
		for (j = 0; j < AES_BLOCK_SIZE; ++j)
		{
			if (plaintext[j] >> 4 == 0)
				uprintf("0");

			uprintf("%x ", plaintext[j]);
		}

		uprintf("\r\n");
#endif
		for (j = 0; j < AES_BLOCK_SIZE; ++j)
			plaintext[i * AES_BLOCK_SIZE + j] ^= cv[j];

		/* Copy to vector */
		memcpy(cv, &ciphertext[i * AES_BLOCK_SIZE], AES_BLOCK_SIZE);
	}

	err = true;
error:
	return err;
}

