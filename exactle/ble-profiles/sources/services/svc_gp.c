/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Example Proximity services implementation.
 *
 *  Copyright (c) 2011-2018 Arm Ltd.
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
 *  d\istributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/

#include "wsf_types.h"
#include "att_api.h"
#include "wsf_trace.h"
#include "util/bstream.h"
#include "svc_gp.h"
#include "svc_cfg.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Characteristic read permissions */
#ifndef GP_SEC_PERMIT_READ
#define GP_SEC_PERMIT_READ SVC_SEC_PERMIT_READ
#endif

/*! Characteristic write permissions */
#ifndef GP_SEC_PERMIT_WRITE
#define GP_SEC_PERMIT_WRITE SVC_SEC_PERMIT_WRITE
#endif

/**************************************************************************************************
 Static Variables
**************************************************************************************************/

/* UUIDs */
static const uint8_t svcRxUuid[] = { ATT_UUID_GP_RX };
static const uint8_t svcTxUuid[] = { ATT_UUID_GP_TX };
static const uint8_t svcTxPwrUuid[ATT_16_UUID_LEN] = {UINT16_TO_BYTES(ATT_UUID_TX_POWER_LEVEL)};

/**************************************************************************************************
 Service variables
**************************************************************************************************/

/* General purpose UUID */
static const uint8_t gpValSvc[] = { ATT_UUID_GP_RX };
static const uint16_t gpLenSvc = sizeof(gpValSvc);

/* General Purpose data characteristic */
static const uint8_t gpValCh[] = {ATT_PROP_WRITE, UINT16_TO_BYTES(GP_HDL), ATT_UUID_GP_RX};
static const uint16_t gpLenCh = sizeof(gpValCh);

/* Data buffer */
static uint8_t gpVal[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static const uint16_t gpLen = sizeof(gpVal);

static const uint8_t gpTxCh[] = {ATT_PROP_NOTIFY, UINT16_TO_BYTES(GP_TX_HDL), ATT_UUID_GP_TX};
static const uint16_t gpLenTxCh = sizeof(gpTxCh);

/* Note these are dummy values */
static const uint8_t gpTx[] = {0};
static const uint16_t gpLenTx = sizeof(gpTx);

/* Proprietary data client characteristic configuration */
static uint8_t gpTxChCcc[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t gpLenTxChCcc = sizeof(gpTxChCcc);

/* Attribute list */
static const attsAttr_t gpList[] =
{
  {
    attPrimSvcUuid,
    (uint8_t *) gpValSvc,
    (uint16_t *) &gpLenSvc,
    sizeof(gpValSvc),
    0,
    ATTS_PERMIT_READ
  },
  {
    attChUuid,
    (uint8_t *) gpValCh,
    (uint16_t *) &gpLenCh,
    sizeof(gpValCh),
    0,
    ATTS_PERMIT_READ
  },
  {
    svcRxUuid,
    (uint8_t *)gpVal,
    (uint16_t *) &gpLen,
    sizeof(gpVal),
    ATTS_SET_UUID_128 | ATTS_SET_VARIABLE_LEN | ATTS_SET_WRITE_CBACK,
    GP_SEC_PERMIT_WRITE
  },
  {
    attChUuid,
    (uint8_t *) gpTxCh,
    (uint16_t *) &gpLenTxCh,
    sizeof(gpTxCh),
    0,
    ATTS_PERMIT_READ
  },
  {
    svcTxUuid,
    (uint8_t *) gpTx,
    (uint16_t *) &gpLenTx,
    sizeof(gpTx), //ATT_VALUE_MAX_LEN,
    0,  //(ATTS_SET_UUID_128 | ATTS_SET_VARIABLE_LEN),
    0,  //ATTS_PERMIT_READ
  },
  {
    attCliChCfgUuid,
    (uint8_t *) gpTxChCcc,
    (uint16_t *) &gpLenTxChCcc,
    sizeof(gpTxChCcc),
    ATTS_SET_CCC,
    (ATTS_PERMIT_READ | ATTS_PERMIT_WRITE)
  }
};

/* Group structure */
static attsGroup_t svcGPGroup =
{
  NULL,
  (attsAttr_t *) gpList,
  NULL,
  NULL,
  GP_START_HDL,
  GP_END_HDL
};

/*************************************************************************************************/
/*!
 *  \brief  Add the services to the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcGPAddGroup(void)
{
  /* add services */
  AttsAddGroup(&svcGPGroup);
}

/*************************************************************************************************/
/*!
 *  \brief  Remove the services from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcGPRemoveGroup(void)
{
  AttsRemoveGroup(GP_START_HDL);
}

/*************************************************************************************************/
/*!
 *  \brief  Register callbacks for the service.
 *
 *  \param  readCback   Read callback function.
 *  \param  writeCback  Write callback function.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcGPCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback)
{
  svcGPGroup.readCback = readCback;
  svcGPGroup.writeCback = writeCback;
}
