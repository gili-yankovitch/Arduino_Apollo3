#ifndef __AES_CBC_H__
#define __AES_CBC_H__

#include <stdint.h>
#include <stdbool.h>
// #include <stdlib.h>
#include <string.h>

#define AES256_KEY_SIZE 32
#define AES_BLOCK_SIZE  16

bool aes256_cbc_encrypt(uint8_t key[AES256_KEY_SIZE], uint8_t iv[AES_BLOCK_SIZE], uint8_t * plaintext, size_t len, uint8_t * ciphertext);
bool aes256_cbc_decrypt(uint8_t key[AES256_KEY_SIZE], uint8_t iv[AES_BLOCK_SIZE], uint8_t * ciphertext, size_t len, uint8_t * plaintext);

#endif
