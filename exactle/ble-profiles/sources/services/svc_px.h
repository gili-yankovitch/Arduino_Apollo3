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
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/

#ifndef SVC_PX_H
#define SVC_PX_H

#include "att_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! Base UUID:  00002760-08C2-11E1-9073-0E8AC72EXXXX */
#define ATT_UUID_GP_BASE             0x2E, 0xC7, 0x8A, 0x0E, 0x73, 0x90, \
                                            0xE1, 0x11, 0xC2, 0x08, 0x60, 0x27, 0x00, 0x00

/*! Macro for building GP UUIDs */
#define ATT_UUID_GP_BUILD(part)      UINT16_TO_BYTES(part), ATT_UUID_GP_BASE

/*! Partial GP service UUIDs */
#define ATT_UUID_GP_SERVICE_PART     0x1011

/*! Partial GP rx characteristic UUIDs */
#define ATT_UUID_GP_RX_PART          0x0011

/*! Partial GP tx characteristic UUIDs */
#define ATT_UUID_GP_TX_PART          0x0012

/*! Partial GP ack characteristic UUIDs */
#define ATT_UUID_GP_ACK_PART         0x0013

/* GP services */
#define ATT_UUID_GP_SERVICE          ATT_UUID_GP_BUILD(ATT_UUID_GP_SERVICE_PART)

/* GP characteristics */
#define ATT_UUID_GP_RX               ATT_UUID_GP_BUILD(ATT_UUID_GP_RX_PART)
#define ATT_UUID_GP_TX               ATT_UUID_GP_BUILD(ATT_UUID_GP_TX_PART)

/*! \addtogroup PROXIMITY_SERVICE
 *  \{ */

/**************************************************************************************************
 Handle Ranges
**************************************************************************************************/

/** \name Proximity Service Handles
 *
 */
/**@{*/
#define PX_START_HDL                      0x50             /*!< \brief Start handle. */
#define PX_END_HDL                        (PX_MAX_HDL - 1) /*!< \brief End handle. */

/**************************************************************************************************
 Handles
**************************************************************************************************/

/*! \brief Service Handles */
enum
{
#if 0
  LLS_SVC_HDL = PX_START_HDL,       /*!< \brief Link loss service declaration */
  LLS_AL_CH_HDL,                    /*!< \brief Alert level characteristic */
  LLS_AL_HDL,                       /*!< \brief Alert level */
#endif
#if 0
  IAS_SVC_HDL = PX_START_HDL,                      /*!< \brief Immediate alert service declaration */
  IAS_AL_CH_HDL,                    /*!< \brief Alert level characteristic */
  IAS_AL_HDL,                       /*!< \brief Alert level */
#else
  GP_SVC_HDL = PX_START_HDL,
  GP_CH_HDL,
  GP_HDL,

  GP_TX_CH_HDL,                     /* GP notify characteristic */
  GP_TX_HDL,                        /* GP notify data */
  GP_TX_CH_CCC_HDL,                 /* GP notify client characteristic configuration */
#endif
  TXS_SVC_HDL,                      /*!< \brief TX power service declaration */
  TXS_TX_CH_HDL,                    /*!< \brief TX power level characteristic */
  TXS_TX_HDL,                       /*!< \brief TX power level */
  PX_MAX_HDL                        /*!< \brief Maximum handle. */
};
/**@}*/

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Add the services to the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcPxAddGroup(void);

/*************************************************************************************************/
/*!
 *  \brief  Remove the services from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcPxRemoveGroup(void);

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
void SvcPxCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback);

/*! \} */    /* PROXIMITY_SERVICE */

#ifdef __cplusplus
};
#endif

#endif /* SVC_PX_H */

