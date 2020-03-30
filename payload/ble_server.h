#ifndef __BLE_SERVER_H__
#define __BLE_SERVER_H__

#include <stdbool.h>
#include "ble_app_protocol.h"

#define MAX_MSG_SIZE 240

void bleSHA256(uint8_t * data, size_t len, uint8_t * hash);

bool_t bleSendEncrypted(dmConnId_t connId, uint8_t * pkt, uint16_t len);

/* Server logic */
void bleServer(dmConnId_t connId, uint8_t * pkt, uint16_t len);

#endif
