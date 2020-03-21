#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_ATECCX08a_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_ATECCX08a
#include <am_util_debug.h>
#include <dm_api.h>
#include <gp/gp_api.h>
#include "ble_usr.h"
#define uprintf am_util_debug_printf

// ATECCX08A atecc;

#define MAX_FRAG_SIZE 16
#define MAX_FRAGMENTS 16

struct frag_s
{
	uint8_t pkt_id;
	uint8_t frag_id;
	uint8_t total_len;
	uint8_t frag[MAX_FRAG_SIZE];
};

struct frag_raw_s
{
	uint8_t data[sizeof(struct frag_s)];
};

static struct frag_s		in_frags[MAX_FRAGMENTS];
static struct frag_raw_s	out_frags[MAX_FRAGMENTS];

struct reassemble_handler_s
{
	uint8_t pkt_id;
	size_t recvd_len;
	size_t total_len;
	uint8_t packet[MAX_FRAGMENTS * MAX_FRAG_SIZE];
};

static uint8_t pkt_ids = 0;
static struct reassemble_handler_s handler = {0, 0, 0};

/* Packet parser */
#define FRAG_OFFSET_PKT_ID	0
#define FRAG_OFFSET_FRAG_ID 1
#define FRAG_OFFSET_TOT_LEN	2
#define FRAG_OFFSET_DATA	3

#define FRAG_PKT_FIELD(pkt, FIELD)	*((uint8_t *)pkt + (FIELD))
#define FRAG_PKT_DATA(pkt)			((uint8_t *)pkt + FRAG_OFFSET_DATA)

void bleSendFragmented(dmConnId_t connId, uint8_t * pkt, uint16_t len)
{
	uint8_t i;
	uint8_t pkt_id = ++pkt_ids;
	uint8_t total_fragments = len / MAX_FRAG_SIZE + !!(len % MAX_FRAG_SIZE);
	uint8_t fragment[sizeof(struct frag_s)];
	uint8_t data_size;

	memset(out_frags, 0, MAX_FRAGMENTS * sizeof(struct frag_raw_s));

	for (i = 0; i < total_fragments; ++i)
	{
		data_size = MAX_FRAG_SIZE;

		if ((len % MAX_FRAG_SIZE) && (i + 1 == total_fragments))
			data_size = len % MAX_FRAG_SIZE;

		FRAG_PKT_FIELD(out_frags[i].data, FRAG_OFFSET_PKT_ID) = pkt_id;
		FRAG_PKT_FIELD(out_frags[i].data, FRAG_OFFSET_FRAG_ID) = i;
		FRAG_PKT_FIELD(out_frags[i].data, FRAG_OFFSET_TOT_LEN) = len;
		memcpy(FRAG_PKT_DATA(out_frags[i].data), pkt + (i * MAX_FRAG_SIZE), data_size);

		uprintf("Sending fragment: %s (Pkt id: %u Total size: %u Fragment size: %u)...\r\n", FRAG_PKT_DATA(out_frags[i].data), pkt_id, len, data_size)

		gpSendResponse(connId, out_frags[i].data, sizeof(out_frags[i].data));
	}
}

extern "C" void bleUsrCallback(dmConnId_t connId, uint8_t * pkt, uint16_t len)
{
	/* Parse metadata */
	uint8_t i;
	uint8_t total_fragments;
	uint8_t pkt_id = FRAG_PKT_FIELD(pkt, FRAG_OFFSET_PKT_ID);
	uint8_t frag_id = FRAG_PKT_FIELD(pkt, FRAG_OFFSET_FRAG_ID);
	uint8_t tot_len = FRAG_PKT_FIELD(pkt, FRAG_OFFSET_TOT_LEN);
	uint8_t * data = FRAG_PKT_DATA(pkt);
	uint8_t data_size = MAX_FRAG_SIZE;

	/* Verify ilegal fragment id */
	if (frag_id >= MAX_FRAGMENTS)
		goto error;

	/* Verify packet size */
	if (len < sizeof(struct frag_s))
		goto error;

	if (handler.pkt_id != pkt_id)
	{
		/* Erase all fragments and reset meta */
		memset(in_frags, 0, sizeof(struct frag_s) * MAX_FRAGMENTS);

		memset(&handler, 0, sizeof(struct reassemble_handler_s));

		handler.pkt_id = pkt_id;
		handler.total_len = tot_len;
	}

	total_fragments = handler.total_len / MAX_FRAG_SIZE + !!(handler.total_len % MAX_FRAG_SIZE);

	/* Invalid fragment length */
	if (frag_id >= total_fragments)
		goto error;

	/* No fragment rewrite */
	if (in_frags[frag_id].pkt_id == pkt_id)
		goto error;

	/* Verify size - is it the last fragment? */
	if ((handler.total_len % MAX_FRAG_SIZE) && (frag_id + 1 == total_fragments))
	{
		data_size = handler.total_len % MAX_FRAG_SIZE;
	}

	/* Copy data */
	in_frags[frag_id].pkt_id = pkt_id;
	in_frags[frag_id].frag_id = frag_id;
	in_frags[frag_id].total_len = tot_len;
	memcpy(in_frags[frag_id].frag, data, data_size);

	uprintf("[FRAG] Pkt Id: %d Received Fragment Id: %d Total fragments: %d Frag size: %d Total size: %d\r\n", pkt_id, frag_id, total_fragments, data_size, tot_len);

	handler.recvd_len += data_size;

	/* Check if fragments finished yet */
	if (handler.recvd_len != handler.total_len)
		goto error;

	for (i = 0; i < total_fragments; ++i)
	{
		data_size = MAX_FRAG_SIZE;

		if ((handler.total_len % MAX_FRAG_SIZE) && (i + 1 == total_fragments))
			data_size = handler.total_len % MAX_FRAG_SIZE;

		memcpy(handler.packet + (i * MAX_FRAG_SIZE), in_frags[i].frag, data_size);
	}

	/* Finally done. Reassemble... */
	uprintf("[FRAG] Fragmentation done. Message size: %u\r\n", handler.total_len);
	// uprintf("\t%s\r\n", handler.packet);

	/* Call server function */
	bleServer(connId, handler.packet, handler.total_len);

error:
	return;
}

extern "C" void gp_setup()
{
	/* Initialize server */
	bleServerInit();

	/* Register callback */
	gpSetUserCallback(bleUsrCallback);
}
