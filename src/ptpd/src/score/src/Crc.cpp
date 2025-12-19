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

/*
 **********************************************************************************************************************
 * Includes
 **********************************************************************************************************************
 */
#include <Crc.h>

/*
 ***********************************************************************************************************************
 * Crc common routines
 ***********************************************************************************************************************
 */

#define CRC_START_SEC_CODE

// clang-format off
/**
 **********************************************************************************************************************
 * Crc_CalculateCRC8
 *
 * \brief Calculates the remainder of a polynomial division: input is divided by the CRC_8_POLYNOMIAL polynomial
 *
 * Crc_CalculateCRC8 calculates the remainder of a polynomial division by the CRC polynomial.
 *
 * CRC calculation starts with an initial value, Crc_StartValue8.
 *
 * The data stream is transmitted via pointer *Crc_DataPtr and is of length Crc_Length.
 * The crc remainder is modified by a XOR operation using the value CRC_XOR_VALUE8.
 *
 * \param    Crc_DataPtr           Pointer to start address of data block
 * \param    Crc_Length            Length of data block to be calculated in bytes
 * \param    Crc_StartValue8       Start value when the algorithm starts
 *
 * \param    Crc_IsFirstCall       Set to TRUE when this is the first call in a sequence or an individual CRC calculation. 
 *                                 Calculation starts from CRC_INITIAL_VALUE8, the parameter Crc_StartValue8 is ignored.
 *                                 Set to FALSE when this is a subsequent call in a call sequence, Crc_StartValue8 is 
 *                                 taken as the unmodified return value of the previous call to this function.
 *
 * \return                         Remainder after polynomial division *const Crc_DataPtr / CRC8
 **********************************************************************************************************************
 */
// clang-format on
/* RULECHECKER_comment(1,1, check_max_parameters, "AUTOSAR API definition", true_no_defect); */
uint8 Crc_CalculateCRC8(const uint8 Crc_DataPtr[], uint32 Crc_Length, uint8 Crc_StartValue8, boolean Crc_IsFirstCall)
{
    uint8 result {Crc_StartValue8};
    if (Crc_DataPtr != NULL_PTR)
    {
        uint8 crcTemp { static_cast<uint8>((Crc_IsFirstCall != FALSE) ? (static_cast<uint8>(CRC_INITIAL_VALUE8))
                                                                      : (Crc_StartValue8 ^ CRC_XOR_VALUE8)) };

        for (uint32 index { 0U }; index < Crc_Length; ++index)
        {
            /* Impact of temporary rest on next crc rest */
            crcTemp ^= (static_cast<uint8>(Crc_DataPtr[index]));
            /* Next temporary crc rest */
            crcTemp = CRC_8_Tbl[crcTemp];
        }
        result = crcTemp ^ CRC_XOR_VALUE8;
    }

    return (result);
}

// clang-format off
/**
 **********************************************************************************************************************
 * Crc_CalculateCRC8H2F
 *
 * \brief Calculates the remainder of a polynomial division: input is divided by the CRC_8H2F_POLYNOMIAL polynomial
 *
 * Crc_CalculateCRC8 calculates the remainder of a polynomial division by the CRC polynomial.
 *
 * CRC calculation starts with an initial value, Crc_StartValue8.
 *
 * The data stream is transmitted via pointer *Crc_DataPtr and is of length Crc_Length.
 * The crc remainder is modified by a XOR operation using the value CRC_XOR_VALUE8H2F.
 *
 * \param    Crc_DataPtr           Pointer to start address of data block
 * \param    Crc_Length            Length of data block to be calculated in bytes
 * \param    Crc_StartValue8       Start value when the algorithm starts
 *
 * \param    Crc_IsFirstCall       Set to TRUE when this is the first call in a sequence or an individual CRC calculation.
 *                                 Calculation starts from CRC_INITIAL_VALUE8, the parameter Crc_StartValue8 is ignored.
 *                                 Set to FALSE when this is a subsequent call in a call sequence, Crc_StartValue8 is 
 *                                 taken as the unmodified return value of the previous call to this function.
 *
 * \return                         Remainder after polynomial division *const Crc_DataPtr / CRC8
 **********************************************************************************************************************
 */
// clang-format on
/* RULECHECKER_comment(1,1, check_max_parameters, "AUTOSAR API definition", true_no_defect); */
uint8 Crc_CalculateCRC8H2F(const uint8 Crc_DataPtr[], uint32 Crc_Length, uint8 Crc_StartValue8, boolean Crc_IsFirstCall)
{
    uint8 result {Crc_StartValue8};
    if (Crc_DataPtr != NULL_PTR)
    {
        uint8 crcTemp { static_cast<uint8>((Crc_IsFirstCall != FALSE) ? (static_cast<uint8>(CRC_INITIAL_VALUE8))
                                                                      : (Crc_StartValue8 ^ CRC_XOR_VALUE8H2F)) };

        for (uint32 index { 0U }; index < Crc_Length; ++index)
        {
            /* Impact of temporary rest on next crc rest */
            crcTemp ^= (static_cast<uint8>(Crc_DataPtr[index]));
            /* Next temporary crc rest */
            crcTemp = CRC_8H2F_Tbl[crcTemp];
        }
        result = crcTemp ^ CRC_XOR_VALUE8H2F;
    }

    return (result);
}

// clang-format off
/**
 **********************************************************************************************************************
 * Crc_CalculateCRC16
 *
 * \brief Calculates the remainder of a polynomial division: input is divided by the CRC_16_POLYNOMIAL polynomial
 *
 * Crc_CalculateCRC16 calculates the remainder of a polynomial division by the CRC polynomial.
 *
 * CRC calculation starts with an initial value, Crc_StartValue16.
 *
 * The data stream is transmitted via pointer *Crc_DataPtr and is of length Crc_Length.
 * The crc remainder is modified by a XOR operation using the value CRC_XOR_VALUE16.
 *
 * \param    Crc_DataPtr           Pointer to start address of data block
 * \param    Crc_Length            Length of data block to be calculated in bytes
 * \param    Crc_StartValue16       Start value when the algorithm starts
 *
 * \param    Crc_IsFirstCall       Set to TRUE when this is the first call in a sequence or an individual CRC calculation.
 *                                 Calculation starts from CRC_INITIAL_VALUE16, the parameter Crc_StartValue16 is ignored.
 *                                 Set to FALSE when this is a subsequent call in a call sequence, Crc_StartValue16 is 
 *                                 taken as the unmodified return value of the previous call to this function.
 *
 * \return                         Remainder after polynomial division *const Crc_DataPtr / CRC16
 **********************************************************************************************************************
 */
// clang-format on
/* RULECHECKER_comment(1,1, check_max_parameters, "AUTOSAR API definition", true_no_defect); */
uint16 Crc_CalculateCRC16(const uint8 Crc_DataPtr[], uint32 Crc_Length, uint16 Crc_StartValue16, boolean Crc_IsFirstCall)
{
    uint16 result {Crc_StartValue16};
    if (Crc_DataPtr != NULL_PTR)
    {
        uint16 crcTemp { static_cast<uint16>((Crc_IsFirstCall != FALSE) ? (static_cast<uint16>(CRC_INITIAL_VALUE16))
                                                                        : (Crc_StartValue16 ^ CRC_XOR_VALUE16)) };

        for (uint32 index { 0U }; index < Crc_Length; ++index)
        {
            /* Impact of temporary rest on next crc rest */
            crcTemp ^= static_cast<uint16>(static_cast<uint16>(Crc_DataPtr[index]) << CRC_16_JUNKREST);
            /* Next temporary crc rest */
            crcTemp
                = static_cast<uint16>(crcTemp << (CRC_JUNK)) ^ CRC_16_Tbl[(crcTemp >> CRC_16_JUNKREST) & CRC_TABLEMASK];
        }
        result = crcTemp ^ CRC_XOR_VALUE16;
    }

    return (result);
}

// clang-format off
/**
 **********************************************************************************************************************
 * Crc_CalculateCRC32
 *
 * \brief Calculates the remainder of a polynomial division: input is divided by the CRC_32_REV_POLYNOMIAL polynomial
 *
 * Crc_CalculateCRC32 calculates the remainder of a polynomial division by the CRC polynomial.
 *
 * CRC calculation starts with an initial value, Crc_StartValue32.
 *
 * The data stream is transmitted via pointer *Crc_DataPtr and is of length Crc_Length.
 * The crc remainder is modified by a XOR operation using the value CRC_XOR_VALUE32.
 *
 * To reduce the computation time required to reflect the incoming data stream and the final result, the CRC polynomial
 * is reflected instead.
 * Using the reflected CRC polynomial with unreflected input and output results in the same CRC value
 * as using the truncated polynomial with reflected input and output data.
 *
 * \param    Crc_DataPtr           Pointer to start address of data block
 * \param    Crc_Length            Length of data block to be calculated in bytes
 * \param    Crc_StartValue32       Start value when the algorithm starts
 *
 * \param    Crc_IsFirstCall       Set to TRUE when this is the first call in a sequence or an individual CRC calculation.
 *                                 Calculation starts from CRC_INITIAL_VALUE32, the parameter Crc_StartValue32 is ignored.
 *                                 Set to FALSE when this is a subsequent call in a call sequence, Crc_StartValue32 is 
 *                                 taken as the unmodified return value of the previous call to this function.
 *
 * \return                         Remainder after polynomial division *const Crc_DataPtr / CRC32
 **********************************************************************************************************************
 */
// clang-format on
/* RULECHECKER_comment(1,1, check_max_parameters, "AUTOSAR API definition", true_no_defect); */
uint32 Crc_CalculateCRC32(const uint8 Crc_DataPtr[], uint32 Crc_Length, uint32 Crc_StartValue32, boolean Crc_IsFirstCall)
{
    uint32 result {Crc_StartValue32};
    if (Crc_DataPtr != NULL_PTR)
    {
        uint32 crcTemp { static_cast<uint32>((Crc_IsFirstCall != FALSE)
                                                 ? (static_cast<uint32>(CRC_INITIAL_VALUE32))
                                                 : (Crc_StartValue32 ^ static_cast<uint32>(CRC_XOR_VALUE32))) };

        for (uint32 index { 0U }; index < Crc_Length; ++index)
        {
            /* Impact of temporary rest on next crc rest */
            crcTemp ^= (static_cast<uint32>(Crc_DataPtr[index]));
            /* Next temporary crc rest */
            crcTemp = (crcTemp >> (CRC_JUNK)) ^ CRC_32_REV_Tbl[(crcTemp)&CRC_TABLEMASK];
        }
        result = crcTemp ^ static_cast<uint32>(CRC_XOR_VALUE32);
    }

    return (result);
}

// clang-format off
/**
 **********************************************************************************************************************
 * Crc_CalculateCRC32P4
 *
 * \brief Calculates the remainder of a polynomial division: input is divided by the CRC_32P4_REV_POLYNOMIAL polynomial
 *
 * Crc_CalculateCRC32 calculates the remainder of a polynomial division by the CRC polynomial.
 *
 * CRC calculation starts with an initial value, Crc_StartValue32.
 *
 * The data stream is transmitted via pointer *Crc_DataPtr and is of length Crc_Length.
 * The crc remainder is modified by a XOR operation using the value CRC_XOR_VALUE32P4.
 *
 * To reduce the computation time required to reflect the incoming data stream and the final result, the CRC polynomial
 * is reflected instead.
 * Using the reflected CRC polynomial with unreflected input and output results in the same CRC value
 * as using the truncated polynomial with reflected input and output data.
 *
 * \param    Crc_DataPtr           Pointer to start address of data block
 * \param    Crc_Length            Length of data block to be calculated in bytes
 * \param    Crc_StartValue32       Start value when the algorithm starts
 *
 * \param    Crc_IsFirstCall       Set to TRUE when this is the first call in a sequence or an individual CRC calculation.
 *                                 Calculation starts from CRC_INITIAL_VALUE32, the parameter Crc_StartValue32 is ignored.
 *                                 Set to FALSE when this is a subsequent call in a call sequence, Crc_StartValue32 is 
 *                                 taken as the unmodified return value of the previous call to this function.
 *
 * \return                         Remainder after polynomial division *const Crc_DataPtr / CRC32
 **********************************************************************************************************************
 */
// clang-format on
/* RULECHECKER_comment(1,1, check_max_parameters, "AUTOSAR API definition", true_no_defect); */
uint32 Crc_CalculateCRC32P4(const uint8 Crc_DataPtr[], uint32 Crc_Length, uint32 Crc_StartValue32, boolean Crc_IsFirstCall)
{
    uint32 result {Crc_StartValue32};
    if (Crc_DataPtr != NULL_PTR)
    {
        uint32 crcTemp { static_cast<uint32>((Crc_IsFirstCall != FALSE)
                                                 ? (static_cast<uint32>(CRC_INITIAL_VALUE32))
                                                 : (Crc_StartValue32 ^ static_cast<uint32>(CRC_XOR_VALUE32P4))) };

        for (uint32 index { 0U }; index < Crc_Length; ++index)
        {
            /* Impact of temporary rest on next crc rest */
            crcTemp ^= (static_cast<uint32>(Crc_DataPtr[index]));
            /* Next temporary crc rest */
            crcTemp = (crcTemp >> (CRC_JUNK)) ^ CRC_32P4_REV_Tbl[(crcTemp)&CRC_TABLEMASK];
        }
        result = crcTemp ^ static_cast<uint32>(CRC_XOR_VALUE32P4);
    }

    return (result);
}

// clang-format off
/**
 **********************************************************************************************************************
 * Crc_CalculateCRC64
 *
 * \brief Calculates the remainder of a polynomial division: input is divided by the CRC_64_REV_POLYNOMIAL polynomial
 *
 * Crc_CalculateCRC64 calculates the remainder of a polynomial division by the CRC polynomial.
 *
 * CRC calculation starts with an initial value, Crc_StartValue64.
 *
 * The data stream is transmitted via pointer *Crc_DataPtr and is of length Crc_Length.
 * The crc remainder is modified by a XOR operation using the value CRC_XOR_VALUE64.
 *
 * To reduce the computation time required to reflect the incoming data stream and the final result, the CRC polynomial
 * is reflected instead.
 * Using the reflected CRC polynomial with unreflected input and output results in the same CRC value
 * as using the truncated polynomial with reflected input and output data.
 *
 * \param    Crc_DataPtr           Pointer to start address of data block
 * \param    Crc_Length            Length of data block to be calculated in bytes
 * \param    Crc_StartValue64       Start value when the algorithm starts
 *
 * \param    Crc_IsFirstCall       Set to TRUE when this is the first call in a sequence or an individual CRC calculation.
 *                                 Calculation starts from CRC_INITIAL_VALUE64, the parameter Crc_StartValue64 is ignored.
 *                                 Set to FALSE when this is a subsequent call in a call sequence, Crc_StartValue64 is 
 *                                 taken as the unmodified return value of the previous call to this function.
 *
 * \return                         Remainder after polynomial division *const Crc_DataPtr / CRC64
 **********************************************************************************************************************
 */
// clang-format on
/* RULECHECKER_comment(1,1, check_max_parameters, "AUTOSAR API definition", true_no_defect); */
uint64 Crc_CalculateCRC64(const uint8 Crc_DataPtr[], uint32 Crc_Length, uint64 Crc_StartValue64, boolean Crc_IsFirstCall)
{
    uint64 result {Crc_StartValue64};
    if (Crc_DataPtr != NULL_PTR)
    {
        uint64 crcTemp { static_cast<uint64>((Crc_IsFirstCall != FALSE) ? (static_cast<uint64>(CRC_INITIAL_VALUE64))
                                                                        : (Crc_StartValue64 ^ CRC_XOR_VALUE64)) };

        for (uint32 index { 0U }; index < Crc_Length; ++index)
        {
            /* Impact of temporary rest on next crc rest */
            crcTemp ^= (static_cast<uint64>(Crc_DataPtr[index]));
            /* Next temporary crc rest */
            crcTemp = (crcTemp >> (CRC_JUNK)) ^ CRC_64_REV_Tbl[(crcTemp)&CRC_TABLEMASK];
        }
        result = crcTemp ^ CRC_XOR_VALUE64;
    }

    return (result);
}

#define CRC_STOP_SEC_CODE
