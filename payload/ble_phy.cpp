#include <Arduino.h>
#include <Wire.h>
#include <am_util_debug.h>
#include <dm_api.h>
//#include <att_defs.h>
#include <gp/gp_api.h>
#include "ble_usr.h"

#define uprintf am_util_debug_printf
#define MAX_PDU (8*16)
#define PHY_HDR_SIZE 2
#define BITS_IN_BYTE 7

void bleUsrCallback(dmConnId_t connId, uint8_t * pkt, uint16_t len);

void blePhyDecode(dmConnId_t connId, uint8_t * pkt, uint16_t len)
{
	int i;
	uint8_t decoded_pkt[MAX_PDU];

	if (len < PHY_HDR_SIZE)
		return;

	memcpy(decoded_pkt, pkt + PHY_HDR_SIZE, len - PHY_HDR_SIZE);

	// uprintf("Decoding bytes: %x %x\r\n", pkt[0], pkt[1]);

	/* Decode all MSBs */
	for (i = 0; i < (len - PHY_HDR_SIZE); ++i)
	{
		/* The bit indicates whether the MSB should be 1 or 0 */
		if (pkt[i / BITS_IN_BYTE] & (1 << (i % BITS_IN_BYTE)))
			decoded_pkt[i] |= 0b10000000;
		else
			decoded_pkt[i] &= ~0b10000000;

		// uprintf("%x ", decoded_pkt[i]);
	}

	// uprintf("\r\n");
	// uprintf("Decoded packet (%d).\r\n", len - PHY_HDR_SIZE);

	bleUsrCallback(connId, decoded_pkt, len - PHY_HDR_SIZE);
}

void blePhyEncode(dmConnId_t connId, uint8_t * pkt, uint16_t len)
{
	int i;
	uint8_t encoded_pkt[MAX_PDU];

	//uprintf("Encoding...\r\n");

	memcpy(encoded_pkt + PHY_HDR_SIZE, pkt, len);

	/* Encode all MSBs */
	for (i = 0; i < len; ++i)
	{
		/* Remove MSB */
		encoded_pkt[PHY_HDR_SIZE + i] &= 0b01111111;

		/* Set the LSB if needed */
		if (pkt[i] & 0b10000000)
			encoded_pkt[i / BITS_IN_BYTE] |= (1 << (i % BITS_IN_BYTE));
		else
			encoded_pkt[i / BITS_IN_BYTE] &= ~(1 << (i % BITS_IN_BYTE));

		//uprintf("%x ", encoded_pkt[PHY_HDR_SIZE + i]);

	}

	encoded_pkt[0] &= 0b01111111;
	encoded_pkt[1] &= 0b01111111;
	//uprintf("(%x %x) len = %d\r\n", encoded_pkt[0], encoded_pkt[1], len);

	gpSendResponse(connId, encoded_pkt, PHY_HDR_SIZE + len);

	/* Wait a bit */
	delay(50);
}

extern "C" void gp_setup()
{
	/* Initialize server */
	bleServerInit();

	/* Register callback */
	gpSetUserCallback(blePhyDecode);
}
