#include <Arduino.h>
#include <Wire.h>
#include <am_util_debug.h>
#include <dm_api.h>
#include <gp/gp_api.h>
#include "ble_usr.h"
#include "ble_phy.h"

#define uprintf am_util_debug_printf

//#include <SparkFun_ATECCX08a_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_ATECCX08a
// ATECCX08A atecc;

#define MAX_FRAG_SIZE 8
#define MAX_FRAGMENTS 16

enum fragment_action_e
{
	E_MORE_FRAGMENTS_WAITING = 0,
	E_DONE_FRAGMENTS_WAITING = 1,
	E_REQUEST_MORE_FRAGMENTS = 2,
	E_FRAGMENTS_ERROR = 3
};

struct frag_s
{
	uint8_t pkt_id;
	uint8_t frag_id;
	uint8_t total_len;
	uint8_t action;
	uint8_t frag[MAX_FRAG_SIZE];
};

struct frag_raw_s
{
	dmConnId_t connId;
	uint8_t fragmentSize;
	uint8_t data[sizeof(struct frag_s)];
};

static struct frag_s		in_frags[MAX_FRAGMENTS];
static struct frag_raw_s	out_frags[MAX_FRAGMENTS];
static uint8_t sent_frag_idx;

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
#define FRAG_OFFSET_ACTION	3
#define FRAG_OFFSET_DATA	4

#define FRAG_HEADER_SIZE 4

#define FRAG_PKT_FIELD(p, FIELD)	*((uint8_t *)p + (FIELD))
#define FRAG_PKT_DATA(p)			((uint8_t *)p + FRAG_OFFSET_DATA)

/* For some fucked up reason the MSB of each byte cannot be set to 1. Make sure that doesn't happen. */
static void bleSendNextFragment()
{
	/* Send */
	blePhyEncode(out_frags[sent_frag_idx].connId, out_frags[sent_frag_idx].data, out_frags[sent_frag_idx].fragmentSize);

	uprintf("%s::%d\r\n", __FILE__, __LINE__);

	/* Advance fragment index */
	sent_frag_idx++;
}

static void bleRequestNextFragment(dmConnId_t connId)
{
	/* Build the request */
	uint8_t request[FRAG_HEADER_SIZE];
	FRAG_PKT_FIELD(request, FRAG_OFFSET_PKT_ID) = ++pkt_ids;
	FRAG_PKT_FIELD(request, FRAG_OFFSET_FRAG_ID) = 0;
	FRAG_PKT_FIELD(request, FRAG_OFFSET_TOT_LEN) = 0;
	FRAG_PKT_FIELD(request, FRAG_OFFSET_ACTION) = E_REQUEST_MORE_FRAGMENTS;

	/* Request */
	blePhyEncode(connId, request, FRAG_HEADER_SIZE);

	uprintf("%s::%d\r\n", __FILE__, __LINE__);
}

void bleSendFragmented(dmConnId_t connId, uint8_t * packet_data, uint16_t len)
{
	int j;
	uint8_t i;
	uint8_t pkt_id = ++pkt_ids;
	uint8_t total_fragments = len / MAX_FRAG_SIZE + !!(len % MAX_FRAG_SIZE);
	// uint8_t fragment[sizeof(struct frag_s)];
	uint8_t data_size;

	memset(out_frags, 0, sizeof(out_frags));

	/* Prepare fragments */
	for (i = 0; i < total_fragments; ++i)
	{
		data_size = MAX_FRAG_SIZE;

		if ((len % MAX_FRAG_SIZE) && (i + 1 == total_fragments))
			data_size = len % MAX_FRAG_SIZE;

		out_frags[i].connId = connId;
		out_frags[i].fragmentSize = FRAG_HEADER_SIZE + data_size;
		FRAG_PKT_FIELD(out_frags[i].data, FRAG_OFFSET_PKT_ID) = pkt_id;
		FRAG_PKT_FIELD(out_frags[i].data, FRAG_OFFSET_FRAG_ID) = i;
		FRAG_PKT_FIELD(out_frags[i].data, FRAG_OFFSET_TOT_LEN) = len;
		FRAG_PKT_FIELD(out_frags[i].data, FRAG_OFFSET_ACTION) = ((i + 1) < total_fragments) ? E_MORE_FRAGMENTS_WAITING : E_DONE_FRAGMENTS_WAITING;
		memcpy(FRAG_PKT_DATA(out_frags[i].data), &packet_data[i * MAX_FRAG_SIZE], data_size);
#if 0
		FRAG_PKT_FIELD(fragment, FRAG_OFFSET_PKT_ID) = pkt_id;
		FRAG_PKT_FIELD(fragment, FRAG_OFFSET_FRAG_ID) = i;
		FRAG_PKT_FIELD(fragment, FRAG_OFFSET_TOT_LEN) =  len;
		memcpy(FRAG_PKT_DATA(fragment), &packet_data[i * MAX_FRAG_SIZE], data_size);

		uprintf("Fragment data %d: ", i);
		for (j = 0; j < data_size; ++j)
		{
			if (fragment[FRAG_OFFSET_DATA + j] >> 4 == 0)
				uprintf("0");

			uprintf("%x ", fragment[FRAG_OFFSET_DATA + j]);
		}
		uprintf("\r\n");
#endif
	}

	/* Reset fragment index */
 	sent_frag_idx = 0;

 	/* Send first fragment */
 	bleSendNextFragment();

	uprintf("%s::%d\r\n", __FILE__, __LINE__);
}

void bleUsrCallback(dmConnId_t connId, uint8_t * pkt, uint16_t len)
{
	/* Parse metadata */
	uint8_t i;
	uint8_t total_fragments;
	uint8_t pkt_id = FRAG_PKT_FIELD(pkt, FRAG_OFFSET_PKT_ID);
	uint8_t frag_id = FRAG_PKT_FIELD(pkt, FRAG_OFFSET_FRAG_ID);
	uint8_t tot_len = FRAG_PKT_FIELD(pkt, FRAG_OFFSET_TOT_LEN);
	uint8_t action = FRAG_PKT_FIELD(pkt, FRAG_OFFSET_ACTION);
	uint8_t * data = FRAG_PKT_DATA(pkt);
	uint8_t data_size = MAX_FRAG_SIZE;

	/* Verify ilegal fragment id */
	if (frag_id >= MAX_FRAGMENTS)
		goto error;

	/* Verify packet size */
	if (len < FRAG_HEADER_SIZE) //sizeof(struct frag_s))
		goto error;

	/* Is it a request for waiting fragments? */
	if (action == E_REQUEST_MORE_FRAGMENTS)
	{
		// uprintf("Sending next fragment (%d)...\r\n", sent_frag_idx);

		/* Send the next one */
		bleSendNextFragment();

		goto end;
	}

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

	if (action == E_MORE_FRAGMENTS_WAITING)
	{
		//uprintf("Requesting the next fragment...\r\n");

		/* Request another fragment */
		bleRequestNextFragment(connId);

		goto end;
	}

	/* Check if fragments finished yet */
	if (handler.recvd_len != handler.total_len)
	{
		goto error;
	}

	for (i = 0; i < total_fragments; ++i)
	{
		data_size = MAX_FRAG_SIZE;

		if ((handler.total_len % MAX_FRAG_SIZE) && (i + 1 == total_fragments))
			data_size = handler.total_len % MAX_FRAG_SIZE;

		memcpy(handler.packet + (i * MAX_FRAG_SIZE), in_frags[i].frag, data_size);
	}

	/* Finally done. Reassemble... */
	uprintf("[FRAG] Defragmentation done. Message size: %u\r\n", handler.total_len);
	// uprintf("\t%s\r\n", handler.packet);

	/* Call server function */
	bleServer(connId, handler.packet, handler.total_len);

	uprintf("%s::%d\r\n", __FILE__, __LINE__);

error:
end:
	return;
}
