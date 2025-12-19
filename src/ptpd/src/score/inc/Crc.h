/*
********************************************************************************************************************** 
*
* COPYRIGHT RESERVED, 2022 Robert Bosch GmbH. All rights reserved.
* The reproduction, distribution and utilization of this document as well as the communication of its contents to
* others without explicit authorization is prohibited. Offenders will be held liable for the payment of damages.
* All rights reserved in the event of the grant of a patent, utility model or design.
*
********************************************************************************************************************** 
*/


#ifndef CRC_H
#define CRC_H

/*
**********************************************************************************************************************
* Includes
**********************************************************************************************************************
*/
#include "Std_Types.h"             /* AUTOSAR standard type definitions */
#include "Crc_Cfg.h"               /* Configuration file */
#include <_export.h>

/*
**********************************************************************************************************************
* Prototypes
**********************************************************************************************************************
*/

#ifdef __cplusplus
extern "C"
{
#endif

#define CRC_START_SEC_CODE
SCORE_CRC_EXPORT extern uint8 Crc_CalculateCRC8(const uint8 Crc_DataPtr[], uint32 Crc_Length, uint8 Crc_StartValue8, boolean Crc_IsFirstCall);
SCORE_CRC_EXPORT extern uint8 Crc_CalculateCRC8H2F(const uint8 Crc_DataPtr[], uint32 Crc_Length, uint8 Crc_StartValue8, boolean Crc_IsFirstCall);
SCORE_CRC_EXPORT extern uint16 Crc_CalculateCRC16(const uint8 Crc_DataPtr[], uint32 Crc_Length, uint16 Crc_StartValue16, boolean Crc_IsFirstCall);
SCORE_CRC_EXPORT extern uint32 Crc_CalculateCRC32(const uint8 Crc_DataPtr[], uint32 Crc_Length, uint32 Crc_StartValue32, boolean Crc_IsFirstCall);
SCORE_CRC_EXPORT extern uint32 Crc_CalculateCRC32P4(const uint8 Crc_DataPtr[], uint32 Crc_Length, uint32 Crc_StartValue32, boolean Crc_IsFirstCall);
SCORE_CRC_EXPORT extern uint64 Crc_CalculateCRC64(const uint8 Crc_DataPtr[], uint32 Crc_Length, uint64 Crc_StartValue64, boolean Crc_IsFirstCall);
#define CRC_STOP_SEC_CODE

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* CRC_H */
