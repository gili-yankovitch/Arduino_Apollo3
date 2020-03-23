#ifndef __SHA256_H__
#define __SHA256_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* SHA256 outputs a 32 byte digest */
#define SHA256_BLOCK_SIZE 32

typedef struct
{
	uint8_t data[64];
	uint32_t datalen;
	uint64_t bitlen;
	uint32_t state[8];
} sha256_ctx_t;

void sha256_init(sha256_ctx_t *ctx);
void sha256_update(sha256_ctx_t *ctx, const uint8_t data[], size_t len);
void sha256_final(sha256_ctx_t *ctx, uint8_t hash[]);

#ifdef __cplusplus
}
#endif

#endif /* __SHA256_H__ */
