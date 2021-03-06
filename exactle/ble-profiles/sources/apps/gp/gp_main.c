/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Tag sample application for the following profiles:
 *            Proximity profile reporter
 *            Find Me profile target
 *            Find Me profile locator
 *
 *  Copyright (c) 2011-2019 Arm Ltd.
 *
 *  Copyright (c) 2019 Packetcraft, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/

#include <string.h>
#include "wsf_types.h"
#include "util/bstream.h"
#include "wsf_msg.h"
#include "wsf_trace.h"
#include "wsf_assert.h"
#include "hci_api.h"
#include "dm_api.h"
#include "att_api.h"
#include "smp_api.h"
#include "app_cfg.h"
#include "app_api.h"
#include "app_main.h"
#include "app_db.h"
#include "app_ui.h"
#include "svc_core.h"
#include "svc_gp.h"
#include "svc_ch.h"
#include "gatt/gatt_api.h"
#include "gap/gap_api.h"
#include "fmpl/fmpl_api.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! WSF message event starting value */
#define TAG_MSG_START               0xA0

/*! WSF message event enumeration */
enum
{
  TAG_RSSI_TIMER_IND = TAG_MSG_START,     /*! Read RSSI value timer expired */
};

/*! Read RSSI interval in seconds */
#define TAG_READ_RSSI_INTERVAL      3

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! proximity reporter control block */
static struct
{
  uint16_t          hdlList[APP_DB_HDL_LIST_LEN];
  wsfHandlerId_t    handlerId;
  uint8_t           discState;
  wsfTimer_t        rssiTimer;                    /* Read RSSI value timer */
  bool_t            inProgress;                   /* Read RSSI value in progress */
  bdAddr_t          peerAddr;                     /* Peer address */
  uint8_t           addrType;                     /* Peer address type */
  appDbHdl_t        dbHdl;                        /* Peer device database record handle */
} tagCb;

/**************************************************************************************************
  Configurable Parameters
**************************************************************************************************/

static void (*g_usrCb)(dmConnId_t, uint8_t *, uint16_t) = NULL;

/*! configurable parameters for advertising */
static const appAdvCfg_t tagAdvCfg =
{
  {60000,     0,     0},                  /*! Advertising durations in ms */
  {  800,   800,     0}                   /*! Advertising intervals in 0.625 ms units */
#if 0
  {15000, 45000,     0},                  /*! Advertising durations in ms */
  {   56,   640,  1824}                   /*! Advertising intervals in 0.625 ms units */
#endif
};

/*! configurable parameters for slave */
static const appSlaveCfg_t tagSlaveCfg =
{
  42, //1,                                      /*! Maximum connections */
};

/*! configurable parameters for security */
static const appSecCfg_t tagSecCfg =
{
  DM_AUTH_BOND_FLAG | DM_AUTH_SC_FLAG,                      /*! Authentication and bonding flags */
  DM_KEY_DIST_IRK,                        /*! Initiator key distribution flags */
  DM_KEY_DIST_LTK | DM_KEY_DIST_IRK,      /*! Responder key distribution flags */
  FALSE,                                  /*! TRUE if Out-of-band pairing data is present */
  FALSE                                   /*! TRUE to initiate security upon connection */
};

/*! configurable parameters for connection parameter update */
static const appUpdateCfg_t tagUpdateCfg =
{
  6000,                                   /*! Connection idle period in ms before attempting
                                              connection parameter update; set to zero to disable */
  6, //640,                                    /*! Minimum connection interval in 1.25ms units */
  16, //800,                                    /*! Maximum connection interval in 1.25ms units */
  50,                                      /*! Connection latency */
  6000,                                    /*! Supervision timeout in 10ms units */
  16                                       /*! Number of update attempts before giving up */
};

/*! Configurable parameters for service and characteristic discovery */
static const appDiscCfg_t tagDiscCfg =
{
  FALSE,                                  /*! TRUE to wait for a secure connection before initiating discovery */
  FALSE                                   /*! TRUE to fall back on database hash to verify handles when no bond exists. */
};

/*! SMP security parameter configuration */
static const smpCfg_t tagSmpCfg =
{
  500,                                    /*! 'Repeated attempts' timeout in msec */
  SMP_IO_NO_IN_NO_OUT,                    /*! I/O Capability */
  7,                                      /*! Minimum encryption key length */
  16,                                     /*! Maximum encryption key length */
  1,                                      /*! Attempts to trigger 'repeated attempts' timeout */
  0,                                      /*! Device authentication requirements */
  64000,                                  /*! Maximum repeated attempts timeout in msec */
  64000,                                  /*! Time msec before attemptExp decreases */
  2                                       /*! Repeated attempts multiplier exponent */
};

static const appCfg_t tagAppCfg =
{
  TRUE,                                   /*! TRUE to abort service discovery if service not found */
  TRUE                                    /*! TRUE to disconnect if ATT transaction times out */
};

/*! local IRK */
static uint8_t localIrk[] =
{
  0x95, 0xC8, 0xEE, 0x6F, 0xC5, 0x0D, 0xEF, 0x93, 0x35, 0x4E, 0x7C, 0x57, 0x08, 0xE2, 0xA3, 0x85
};

/**************************************************************************************************
  Advertising Data
**************************************************************************************************/

/*! advertising data, discoverable mode */
static const uint8_t gpAdvDataDisc[] =
{
  /*! device name */
  10,                                      /*! length */
  DM_ADV_TYPE_LOCAL_NAME,                 /*! AD type */
  'C',
  'r',
  'y',
  'p',
  't',
  'o',
  'K',
  'i',
  't'
};

/*! scan data */
static const uint8_t gpScanData[] =
{
  /*! service UUID list */
  1,                                      /*! length */
  DM_ADV_TYPE_16_UUID_PART,                    /*! AD type */

  //UINT16_TO_BYTES(ATT_UUID_LINK_LOSS_SERVICE),
  //UINT16_TO_BYTES(ATT_UUID_IMMEDIATE_ALERT_SERVICE),
  //UINT16_TO_BYTES(ATT_UUID_TX_POWER_SERVICE)
};

/**************************************************************************************************
  ATT Client Discovery Data
**************************************************************************************************/

/*! Discovery states:  enumeration of services to be discovered */
enum
{
  TAG_DISC_IAS_SVC,       /* Immediate Alert service */
  TAG_DISC_GATT_SVC,      /* GATT service */
  TAG_DISC_GAP_SVC,       /* GAP service */
  TAG_DISC_SVC_MAX        /* Discovery complete */
};

/*! the Client handle list, tagCb.hdlList[], is set as follows:
 *
 *  ------------------------------- <- TAG_DISC_IAS_START
 *  | IAS alert level handle      |
 *  ------------------------------- <- TAG_DISC_GATT_START
 *  | GATT svc changed handle     |
 *  -------------------------------
 *  | GATT svc changed ccc handle |
 *  ------------------------------- <- TAG_DISC_GAP_START
 *  | GAP central addr res handle |
 *  -------------------------------
 *  | GAP RPA Only handle         |
 *  -------------------------------
 */

/*! Start of each service's handles in the the handle list */
#define TAG_DISC_IAS_START        0
#define TAG_DISC_GATT_START       (TAG_DISC_IAS_START + FMPL_IAS_HDL_LIST_LEN)
#define TAG_DISC_GAP_START        (TAG_DISC_GATT_START + GATT_HDL_LIST_LEN)
#define TAG_DISC_HDL_LIST_LEN     (TAG_DISC_GAP_START + GAP_HDL_LIST_LEN)

/*! Pointers into handle list for each service's handles */
static uint16_t *pTagIasHdlList  = &tagCb.hdlList[TAG_DISC_IAS_START];
static uint16_t *pTagGattHdlList = &tagCb.hdlList[TAG_DISC_GATT_START];
static uint16_t *pTagGapHdlList  = &tagCb.hdlList[TAG_DISC_GAP_START];

/* sanity check:  make sure handle list length is <= app db handle list length */
WSF_CT_ASSERT(TAG_DISC_HDL_LIST_LEN <= APP_DB_HDL_LIST_LEN);

/**************************************************************************************************
  ATT Client Data
**************************************************************************************************/

/* Default value for GATT service changed ccc descriptor */
static const uint8_t tagGattScCccVal[] = {UINT16_TO_BYTES(ATT_CLIENT_CFG_INDICATE)};

/* Default value for Client Supported Features (enable Robust Caching) */
const uint8_t tagCsfVal[1] = { ATTS_CSF_ROBUST_CACHING };

/* List of characteristics to configure */
static const attcDiscCfg_t tagDiscCfgList[] =
{
  /* Write:  GATT service changed ccc descriptor */
  {tagGattScCccVal, sizeof(tagGattScCccVal), (GATT_SC_CCC_HDL_IDX + TAG_DISC_GATT_START)},

  /* Write:  GATT client supported features */
  {tagCsfVal, sizeof(tagCsfVal), (GATT_CSF_HDL_IDX + TAG_DISC_GATT_START) },

  /* Read: GAP central address resolution attribute */
  {NULL, 0, (GAP_CAR_HDL_IDX + TAG_DISC_GAP_START)},
};

/* Characteristic configuration list length */
#define TAG_DISC_CFG_LIST_LEN   (sizeof(tagDiscCfgList) / sizeof(attcDiscCfg_t))

/* sanity check:  make sure configuration list length is <= handle list length */
WSF_CT_ASSERT(TAG_DISC_CFG_LIST_LEN <= TAG_DISC_HDL_LIST_LEN);

/**************************************************************************************************
  ATT Server Data
**************************************************************************************************/

/*! enumeration of client characteristic configuration descriptors used in local ATT server */
enum
{
  TAG_GATT_SC_CCC_IDX,        /*! GATT service, service changed characteristic */
  TAG_NUM_CCC_IDX             /*! Number of ccc's */
};

/*! client characteristic configuration descriptors settings, indexed by ccc enumeration */
static const attsCccSet_t tagCccSet[TAG_NUM_CCC_IDX] =
{
  /* cccd handle         value range                 security level */
  {GATT_SC_CH_CCC_HDL,   ATT_CLIENT_CFG_INDICATE,    DM_SEC_LEVEL_ENC}    /* TAG_GATT_SC_CCC_IDX */
};

/*************************************************************************************************/
/*!
 *  \brief  Application DM callback.
 *
 *  \param  pDmEvt  DM callback event
 *
 *  \return None.
 */
/*************************************************************************************************/
static void tagDmCback(dmEvt_t *pDmEvt)
{
  dmEvt_t *pMsg;
  uint16_t  len;

  if (pDmEvt->hdr.event == DM_SEC_ECC_KEY_IND)
  {
    DmSecSetEccKey(&pDmEvt->eccMsg.data.key);
  }
  else
  {
    len = DmSizeOfEvt(pDmEvt);

    if ((pMsg = WsfMsgAlloc(len)) != NULL)
    {
      memcpy(pMsg, pDmEvt, len);
      WsfMsgSend(tagCb.handlerId, pMsg);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Application ATT callback.
 *
 *  \param  pEvt    ATT callback event
 *
 *  \return None.
 */
/*************************************************************************************************/
static void tagAttCback(attEvt_t *pEvt)
{
  attEvt_t *pMsg;

  if ((pMsg = WsfMsgAlloc(sizeof(attEvt_t) + pEvt->valueLen)) != NULL)
  {
    memcpy(pMsg, pEvt, sizeof(attEvt_t));
    pMsg->pValue = (uint8_t *) (pMsg + 1);
    pMsg->valueLen = pEvt->valueLen;
    memcpy(pMsg->pValue, pEvt->pValue, pEvt->valueLen);
    WsfMsgSend(tagCb.handlerId, pMsg);
  }

  return;
}

/*************************************************************************************************/
/*!
 *  \brief  Application ATTS client characteristic configuration callback.
 *
 *  \param  pDmEvt  DM callback event
 *
 *  \return None.
 */
/*************************************************************************************************/
static void tagCccCback(attsCccEvt_t *pEvt)
{
  attsCccEvt_t  *pMsg;
  appDbHdl_t    dbHdl;

  /* If CCC not set from initialization and there's a device record and currently bonded */
  if ((pEvt->handle != ATT_HANDLE_NONE) &&
      ((dbHdl = AppDbGetHdl((dmConnId_t) pEvt->hdr.param)) != APP_DB_HDL_NONE) &&
      AppCheckBonded((dmConnId_t)pEvt->hdr.param))
  {
    /* Store value in device database. */
    AppDbSetCccTblValue(dbHdl, pEvt->idx, pEvt->value);
  }

  if ((pMsg = WsfMsgAlloc(sizeof(attsCccEvt_t))) != NULL)
  {
    memcpy(pMsg, pEvt, sizeof(attsCccEvt_t));
    WsfMsgSend(tagCb.handlerId, pMsg);
  }
}

/* Set user callback */
void gpSetUserCallback(void (*usrCb)(dmConnId_t, uint8_t *, uint16_t))
{
	g_usrCb = usrCb;
}

#include "am_util_debug.h"

void gpSendResponse(dmConnId_t connId, uint8_t * val, uint16_t len)
{
	AttsHandleValueNtf(connId, GP_TX_HDL, len, val);
}

/*************************************************************************************************/
/*!
 *  \brief  ATTS write callback for Immediate Alert service.
 *
 *  \return ATT status.
 */
/*************************************************************************************************/
static uint8_t tagIasWriteCback(dmConnId_t connId, uint16_t handle, uint8_t operation,
                                uint16_t offset, uint16_t len, uint8_t *pValue,
                                attsAttr_t *pAttr)
{
  // pValue[len] = '\0';
  // am_util_debug_printf(">>> %s\r\n", pValue);
  // am_util_debug_printf("\t\t\tlen: %u connId: %d handle: 0x%04x op: 0x%02x\r\n", len, connId, handle, operation);

  /* Call user callback */
  if (g_usrCb)
    g_usrCb(connId, pValue, len);

  /* Pong */
  // AttsHandleValueNtf(connId, GP_TX_HDL, len, pValue);

  return ATT_SUCCESS;
}

/*************************************************************************************************/
/*!
*  \brief  Perform actions on connection open.
*
*  \param  pMsg    Pointer to DM callback event message.
*
*  \return None.
*/
/*************************************************************************************************/
static void tagOpen(dmEvt_t *pMsg)
{
  /* Update peer address info */
  tagCb.addrType = DmHostAddrType(pMsg->connOpen.addrType);
  BdaCpy(tagCb.peerAddr, pMsg->connOpen.peerAddr);
}

/*************************************************************************************************/
/*!
 *  \brief  Perform UI actions on connection close.
 *
 *  \param  pMsg    Pointer to DM callback event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void tagClose(dmEvt_t *pMsg)
{
  uint8_t   *pVal;
  uint16_t  len;

  /* if read RSSI in progress, stop timer */
  if (tagCb.inProgress)
  {
    tagCb.inProgress = FALSE;

    /* stop timer */
    WsfTimerStop(&tagCb.rssiTimer);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Get peer key from a device database record.
 *
 *  \param  dbHdl   Device database record handle.
 *
 *  \return Pointer to peer key if key is valid or NULL if not valid.
 */
/*************************************************************************************************/
static dmSecKey_t *tagGetPeerKey(appDbHdl_t dbHdl)
{
  /* if database record handle valid */
  if (dbHdl != APP_DB_HDL_NONE)
  {
    return AppDbGetKey(dbHdl, DM_KEY_IRK, NULL);
  }

  return NULL;
}

/*************************************************************************************************/
/*!
 *  \brief  Handle pairing complete.
 *
 *  \param  pMsg    Pointer to DM callback event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void tagSecPairCmpl(dmEvt_t *pMsg)
{
  /* store database record handle for peer device */
  tagCb.dbHdl = AppDbGetHdl((dmConnId_t) pMsg->hdr.param);
}

/*************************************************************************************************/
/*!
 *  \brief  Handle add device to resolving list indication.
 *
 *  \param  pMsg    Pointer to DM callback event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void tagPrivAddDevToResListInd(dmEvt_t *pMsg)
{
  dmSecKey_t *pPeerKey;

  /* if peer IRK present */
  if ((pPeerKey = tagGetPeerKey(AppDbGetHdl((dmConnId_t) pMsg->hdr.param))) != NULL)
  {
    /* set advertising peer address */
    AppSetAdvPeerAddr(pPeerKey->irk.addrType, pPeerKey->irk.bdAddr);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Handle remove device from resolving list indication.
 *
 *  \param  pMsg    Pointer to DM callback event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void tagPrivRemDevFromResListInd(dmEvt_t *pMsg)
{
  if (pMsg->hdr.status == HCI_SUCCESS)
  {
    if (AppDbGetHdl((dmConnId_t) pMsg->hdr.param) != APP_DB_HDL_NONE)
    {
      uint8_t addrZeros[BDA_ADDR_LEN] = { 0 };

      /* clear advertising peer address and its type */
      AppSetAdvPeerAddr(HCI_ADDR_TYPE_PUBLIC, addrZeros);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Set up advertising and other procedures that need to be performed after
 *          device reset.
 *
 *  \param  pMsg    Pointer to DM callback event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void tagSetup(dmEvt_t *pMsg)
{
  /* set advertising and scan response data for discoverable mode */
  AppAdvSetData(APP_ADV_DATA_DISCOVERABLE, sizeof(gpAdvDataDisc), (uint8_t *) gpAdvDataDisc);
  AppAdvSetData(APP_SCAN_DATA_DISCOVERABLE, sizeof(gpScanData), (uint8_t *) gpScanData);

  /* set advertising and scan response data for connectable mode */
  AppAdvSetData(APP_ADV_DATA_CONNECTABLE, 0, NULL);
  AppAdvSetData(APP_SCAN_DATA_CONNECTABLE, sizeof(gpScanData), (uint8_t *) gpScanData);

  /* start advertising; automatically set connectable/discoverable mode and bondable mode */
  AppAdvStart(APP_MODE_AUTO_INIT);
}

/*************************************************************************************************/
/*!
 *  \brief  Process a received ATT indication.
 *
 *  \param  pMsg    Pointer to ATT callback event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void tagValueUpdate(attEvt_t *pMsg)
{
  if (pMsg->hdr.status == ATT_SUCCESS)
  {
    /* determine which profile the handle belongs to */

    /* GATT */
    if (GattValueUpdate(pTagGattHdlList, pMsg) == ATT_SUCCESS)
    {
      return;
    }

    /* GAP */
    if (GapValueUpdate(pTagGapHdlList, pMsg) == ATT_SUCCESS)
    {
      return;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief  GAP service discovery has completed.
 *
 *  \param  connId    Connection identifier.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void tagDiscGapCmpl(dmConnId_t connId)
{
  appDbHdl_t dbHdl;

  /* if RPA Only attribute found on peer device */
  if ((pTagGapHdlList[GAP_RPAO_HDL_IDX] != ATT_HANDLE_NONE) &&
      ((dbHdl = AppDbGetHdl(connId)) != APP_DB_HDL_NONE))
  {
    /* update DB */
    AppDbSetPeerRpao(dbHdl, TRUE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  This function is called when the periodic read RSSI timer expires.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void tagProcRssiTimer(dmEvt_t *pMsg)
{
  dmConnId_t  connId;

  /* if still connected */
  if ((connId = AppConnIsOpen()) != DM_CONN_ID_NONE)
  {
    /* read RSSI value for the active connection */
    DmConnReadRssi(connId);

    /* restart timer */
    WsfTimerStartSec(&tagCb.rssiTimer, TAG_READ_RSSI_INTERVAL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Discovery callback.
 *
 *  \param  connId    Connection identifier.
 *  \param  status    Service or configuration status.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void tagDiscCback(dmConnId_t connId, uint8_t status)
{
  switch(status)
  {
    case APP_DISC_INIT: /* 0 */
      /* set handle list when initialization requested */
      AppDiscSetHdlList(connId, TAG_DISC_HDL_LIST_LEN, tagCb.hdlList);
      break;

    case APP_DISC_READ_DATABASE_HASH: /* 1 */
      /* Read peer's database hash */
      AppDiscReadDatabaseHash(connId);
      break;

    case APP_DISC_SEC_REQUIRED: /* 2 */
      /* request security */
      AppSlaveSecurityReq(connId);
      break;

    case APP_DISC_START: /* 3 */
      /* initialize discovery state */
      tagCb.discState = TAG_DISC_IAS_SVC;

      /* discover immediate alert service */
      FmplIasDiscover(connId, pTagIasHdlList);
      break;

    case APP_DISC_FAILED: /* 5 */
      if (pAppCfg->abortDisc)
      {
        /* if immediate alert service not found */
        if (tagCb.discState == TAG_DISC_IAS_SVC)
        {
          /* discovery failed */
          AppDiscComplete(connId, APP_DISC_FAILED);
          break;
        }
      }
      /* Else falls through. */

    case APP_DISC_CMPL: /* 4 */
      /* next discovery state */
      tagCb.discState++;

      if (tagCb.discState == TAG_DISC_GATT_SVC)
      {
        /* discover GATT service */
        GattDiscover(connId, pTagGattHdlList);
      }
      else if (tagCb.discState == TAG_DISC_GAP_SVC)
      {
        /* discover GAP service */
        GapDiscover(connId, pTagGapHdlList);
      }
      else
      {
        /* discovery complete */
        AppDiscComplete(connId, APP_DISC_CMPL);

        /* GAP service discovery completed */
        tagDiscGapCmpl(connId);

        /* start configuration */
        AppDiscConfigure(connId, APP_DISC_CFG_START, TAG_DISC_CFG_LIST_LEN,
                         (attcDiscCfg_t *) tagDiscCfgList, TAG_DISC_HDL_LIST_LEN, tagCb.hdlList);
      }
      break;

    case APP_DISC_CFG_START:
      /* start configuration */
      AppDiscConfigure(connId, APP_DISC_CFG_START, TAG_DISC_CFG_LIST_LEN,
                       (attcDiscCfg_t *) tagDiscCfgList, TAG_DISC_HDL_LIST_LEN, tagCb.hdlList);
      break;

    case APP_DISC_CFG_CMPL:
      AppDiscComplete(connId, APP_DISC_CFG_CMPL);
      break;

    case APP_DISC_CFG_CONN_START:
      /* no connection setup configuration for this application */
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Process messages from the event handler.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void tagProcMsg(dmEvt_t *pMsg)
{
  uint8_t uiEvent = APP_UI_NONE;

  switch(pMsg->hdr.event)
  {
    case ATTC_READ_RSP:
    case ATTC_HANDLE_VALUE_IND:
      tagValueUpdate((attEvt_t *) pMsg);
      break;

    case ATT_MTU_UPDATE_IND:
      APP_TRACE_INFO1("Negotiated MTU %d", ((attEvt_t *)pMsg)->mtu);
      break;  

    case DM_RESET_CMPL_IND:
      AttsCalculateDbHash();
      DmSecGenerateEccKeyReq();
      tagSetup(pMsg);
      uiEvent = APP_UI_RESET_CMPL;
      break;

    case DM_ADV_START_IND:
      uiEvent = APP_UI_ADV_START;
      break;

    case DM_ADV_STOP_IND:
      uiEvent = APP_UI_ADV_STOP;
      break;

    case DM_CONN_OPEN_IND:
      tagOpen(pMsg);
      uiEvent = APP_UI_CONN_OPEN;
      break;

    case DM_CONN_CLOSE_IND:
      tagClose(pMsg);
      uiEvent = APP_UI_CONN_CLOSE;
      break;

    case DM_SEC_PAIR_CMPL_IND:
      tagSecPairCmpl(pMsg);
      DmSecGenerateEccKeyReq();
      uiEvent = APP_UI_SEC_PAIR_CMPL;
      break;

    case DM_SEC_PAIR_FAIL_IND:
      DmSecGenerateEccKeyReq();
      uiEvent = APP_UI_SEC_PAIR_FAIL;
      break;

    case DM_SEC_ENCRYPT_IND:
      uiEvent = APP_UI_SEC_ENCRYPT;
      break;

    case DM_SEC_ENCRYPT_FAIL_IND:
      uiEvent = APP_UI_SEC_ENCRYPT_FAIL;
      break;

    case DM_SEC_AUTH_REQ_IND:
      AppHandlePasskey(&pMsg->authReq);
      break;

    case DM_SEC_COMPARE_IND:
      AppHandleNumericComparison(&pMsg->cnfInd);
      break;

    case DM_PRIV_ADD_DEV_TO_RES_LIST_IND:
      tagPrivAddDevToResListInd(pMsg);
      break;

    case DM_PRIV_REM_DEV_FROM_RES_LIST_IND:
      tagPrivRemDevFromResListInd(pMsg);
      break;

    case DM_ADV_NEW_ADDR_IND:
      break;

    case TAG_RSSI_TIMER_IND:
      tagProcRssiTimer(pMsg);
      break;

    case DM_CONN_READ_RSSI_IND:
      /* if successful */
      if (pMsg->hdr.status == HCI_SUCCESS)
      {
        /* display RSSI value */
        AppUiDisplayRssi(pMsg->readRssi.rssi);
      }
      break;

    case DM_PRIV_CLEAR_RES_LIST_IND:
      APP_TRACE_INFO1("Clear resolving list status 0x%02x", pMsg->hdr.status);
      break;

    case DM_VENDOR_SPEC_CMD_CMPL_IND:
      {
        #if defined(AM_PART_APOLLO) || defined(AM_PART_APOLLO2)
       
          uint8_t *param_ptr = &pMsg->vendorSpecCmdCmpl.param[0];
        
          switch (pMsg->vendorSpecCmdCmpl.opcode)
          {
            case 0xFC20: //read at address
            {
              uint32_t read_value;

              BSTREAM_TO_UINT32(read_value, param_ptr);

              APP_TRACE_INFO3("VSC 0x%0x complete status %x param %x", 
                pMsg->vendorSpecCmdCmpl.opcode, 
                pMsg->hdr.status,
                read_value);
            }

            break;
            default:
                APP_TRACE_INFO2("VSC 0x%0x complete status %x",
                    pMsg->vendorSpecCmdCmpl.opcode,
                    pMsg->hdr.status);
            break;
          }

        #endif
      }
      break;

    default:
      break;
  }

  if (uiEvent != APP_UI_NONE)
  {
    AppUiAction(uiEvent);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Proximity reporter handler init function called during system initialization.
 *
 *  \param  handlerID  WSF handler ID.
 *
 *  \return None.
 */
/*************************************************************************************************/
void GPHandlerInit(wsfHandlerId_t handlerId)
{
  // APP_TRACE_INFO0("TagHandlerInit");

  /* store handler ID */
  tagCb.handlerId = handlerId;

  /* initialize control block */
  tagCb.rssiTimer.handlerId = handlerId;
  tagCb.rssiTimer.msg.event = TAG_RSSI_TIMER_IND;
  tagCb.inProgress = FALSE;

  /* Set configuration pointers */
  pAppSlaveCfg = (appSlaveCfg_t *) &tagSlaveCfg;
  pAppAdvCfg = (appAdvCfg_t *) &tagAdvCfg;
  pAppSecCfg = (appSecCfg_t *) &tagSecCfg;
  pAppUpdateCfg = (appUpdateCfg_t *) &tagUpdateCfg;
  pAppDiscCfg = (appDiscCfg_t *) &tagDiscCfg;
  pAppCfg = (appCfg_t *) &tagAppCfg;

  /* Set stack configuration pointers */
  pSmpCfg = (smpCfg_t *)&tagSmpCfg;

  /* Initialize application framework */
  AppSlaveInit();
  AppDiscInit();
  AppServerInit();

  /* Set IRK for the local device */
  DmSecSetLocalIrk(localIrk);
}

/*************************************************************************************************/
/*!
 *  \brief  WSF event handler for proximity reporter.
 *
 *  \param  event   WSF event mask.
 *  \param  pMsg    WSF message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void GPHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
  if (pMsg != NULL)
  {
    /* process ATT messages */
    if (pMsg->event <= ATT_CBACK_END)
    {
      /* process discovery-related ATT messages */
      AppDiscProcAttMsg((attEvt_t *) pMsg);

      /* process server-related ATT messages */
      AppServerProcAttMsg(pMsg);
    }
    /* process DM messages */
    else if (pMsg->event <= DM_CBACK_END)
    {
      /* process advertising and connection-related messages */
      AppSlaveProcDmMsg((dmEvt_t *) pMsg);

      /* process security-related messages */
      AppSlaveSecProcDmMsg((dmEvt_t *) pMsg);

      /* process discovery-related messages */
      AppDiscProcDmMsg((dmEvt_t *) pMsg);
    }

    /* perform profile and user interface-related operations */
    tagProcMsg((dmEvt_t *) pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Start the proximity profile reporter application.
 *
 *  \return None.
 */
/*************************************************************************************************/
void GPStart(void)
{
  /* Register for stack callbacks */
  DmRegister(tagDmCback);
  DmConnRegister(DM_CLIENT_ID_APP, tagDmCback);
  AttRegister(tagAttCback);
  AttConnRegister(AppServerConnCback);
  AttsCccRegister(TAG_NUM_CCC_IDX, (attsCccSet_t *) tagCccSet, tagCccCback);

  /* Register for app framework button callbacks */
  //AppUiBtnRegister(tagBtnCback);

  /* Register for app framework discovery callbacks */
  AppDiscRegister(tagDiscCback);

  /* Initialize attribute server database */
  SvcCoreGattCbackRegister(GattReadCback, GattWriteCback);
  SvcCoreAddGroup();
  SvcGPCbackRegister(NULL, tagIasWriteCback);
  SvcGPAddGroup();

  /* Set Service Changed CCCD index. */
  GattSetSvcChangedIdx(TAG_GATT_SC_CCC_IDX);

  /* Reset the device */
  DmDevReset();
}
