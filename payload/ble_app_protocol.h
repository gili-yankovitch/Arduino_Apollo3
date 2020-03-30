#ifndef __BLE_APP_PROTOCOL_H__
#define __BLE_APP_PROTOCOL_H__
#include <stdint.h>
#include <uECC.h>

#include "ble_app_config.h"

void bleAppInit();

void bleProtocolHandler(dmConnId_t connId, uint8_t * pkt, uint16_t len);

#endif
