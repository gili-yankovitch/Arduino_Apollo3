#ifndef __BLE_USR_H__
#define __BLE_USR_H__

/* Send fragmented packet */
void bleSendFragmented(dmConnId_t connId, uint8_t * pkt, uint16_t len);

/* Server stup */
void bleServerInit();

/* Server logic */
void bleServer(dmConnId_t connId, uint8_t * pkt, uint16_t len);

#endif
