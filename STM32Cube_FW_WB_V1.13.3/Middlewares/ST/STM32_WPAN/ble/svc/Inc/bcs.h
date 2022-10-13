/**
  ******************************************************************************
  * @file    bcs.h
  * @author  GPM Application Team
  * @brief   Header for bcs.c module
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2018-2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */



/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BCS_H
#define __BCS_H

#ifdef __cplusplus
extern "C" 
{
#endif

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef enum
{
 /*
  * bit 0
  * Time Stamp Supported Supported
  * 0 = False 1 = True
  */
 BCS_FEATURE_SUPPORTED_TIME_STAMP           = 1<<0,
 /*
  * bit 1
  * Multiple Users Supported
  * 0 = False 1 = True
  */
 BCS_FEATURE_SUPPORTED_MULTIPLE_USERS       = 1<<1,
 /*
  * bit 2
  * Basal Metabolism Supported
  * 0 = False 1 = True
  */
 BCS_FEATURE_SUPPORTED_BASAL_METABOLISM     = 1<<2,
 /*
  * bit 3
  * Muscle Percentage Supported
  * 0 = False 1 = True
  */
 BCS_FEATURE_SUPPORTED_MUSCLE_PERCENTAGE    = 1<<3,
 /*
  * bit 4
  * Muscle Mass Supported
  * 0 = False 1 = True
  */
 BCS_FEATURE_SUPPORTED_MUSCLE_MASS          = 1<<4,
 /*
  * bit 5
  * Fat Free Mass Supported
  * 0 = False 1 = True
  */
 BCS_FEATURE_SUPPORTED_FAT_FREE_MASS        = 1<<5,
 /*
  * bit 6
  * Soft Lean Mass Supported
  * 0 = False 1 = True
  */
 BCS_FEATURE_SUPPORTED_SOFT_LEAN_MASS       = 1<<6,
 /*
  * bit 7
  * Body Water Mass Supported
  * 0 = False 1 = True
  */
 BCS_FEATURE_SUPPORTED_BODY_WATER_MASS      = 1<<7,
 /*
  * bit 8
  * Impedance Supported
  * 0 = False 1 = True
  */
 BCS_FEATURE_SUPPORTED_IMPEDANCE            = 1<<8,
 /*
  * bit 9
  * Weight Supported
  * 0 = False 1 = True
  */
 BCS_FEATURE_WEIGHT               = 1<<9,
 /*
  * bit 10
  * Height Supported
  * 0 = False 1 = True
  */
 BCS_FEATURE_HEIGHT               = 1<<10,
 /*
  * bit 11 - 14
  * Weight Measurement Resolution
  */
 BCS_FEATURE_WEIGHT_MEASUREMENT_RESOLUTION_0_5KG   =       1<<11,
 BCS_FEATURE_WEIGHT_MEASUREMENT_RESOLUTION_0_2KG   =       2<<11,
 BCS_FEATURE_WEIGHT_MEASUREMENT_RESOLUTION_0_1KG   =       3<<11,
 BCS_FEATURE_WEIGHT_MEASUREMENT_RESOLUTION_0_05KG  =       4<<11,
 BCS_FEATURE_WEIGHT_MEASUREMENT_RESOLUTION_0_02KG  =       5<<11,
 BCS_FEATURE_WEIGHT_MEASUREMENT_RESOLUTION_0_01KG  =       6<<11,
 BCS_FEATURE_WEIGHT_MEASUREMENT_RESOLUTION_0_005KG =       7<<11,
 /*
  * bit 15 - 17
  * Height Measurement Resolution
  */
 BCS_FEATURE_HEIGHT_MEASUREMENT_RESOLUTION_0_01M   =       1<<15,
 BCS_FEATURE_HEIGHT_MEASUREMENT_RESOLUTION_0_005M  =       2<<15,
 BCS_FEATURE_HEIGHT_MEASUREMENT_RESOLUTION_0_001M  =       3<<15,
} BCS_FeatureBCS_FEATURE_SUPPORTED_t;

typedef enum
{
  BCS_MEASUREMENT_IND_DISABLED_EVT=0,
  BCS_MEASUREMENT_IND_ENABLED_EVT
} BCS_App_Opcode_Notification_evt_t;

typedef enum
{
  BCS_FLAG_MEASUREMENT_UNITS_SI       = 0,
  BCS_FLAG_MEASUREMENT_UNITS_IMPERIAL = 1<<0,
  BCS_FLAG_TIME_STAMP_PRESENT         = 1<<1,  /* 0 = False, 1 = True */
  BCS_FLAG_USER_ID_PRESENT            = 1<<2,  /* 0 = False, 1 = True */
  BCS_FLAG_BASAL_METABOLISM_PRESENT   = 1<<3,  /* 0 = False, 1 = True */
  BCS_FLAG_MUSCLE_PERCENTAGE_PRESENT  = 1<<4,  /* 0 = False, 1 = True */
  BCS_FLAG_MUSCLE_MASS_PRESENT        = 1<<5,  /* 0 = False, 1 = True */
  BCS_FLAG_FAT_FREE_MASS_PRESENT      = 1<<6,  /* 0 = False, 1 = True */
  BCS_FLAG_SOFT_LEAN_MASS_PRESENT     = 1<<7,  /* 0 = False, 1 = True */
  BCS_FLAG_BODY_WATER_MASS_PRESENT    = 1<<8,  /* 0 = False, 1 = True */
  BCS_FLAG_IMPEDANCE_PRESENT          = 1<<9,  /* 0 = False, 1 = True */
  BCS_FLAG_WEIGHT_PRESENT             = 1<<10, /* 0 = False, 1 = True */
  BCS_FLAG_HEIGHT_PRESENT             = 1<<11, /* 0 = False, 1 = True */
  BCS_FLAG_MULTIPLE_PACKET_MEASUREMENT = 1<<12,/* 0 = False, 1 = True */
} BCS_MeasurementFlags_t;

typedef struct
{
  uint32_t Value;
  
} BCS_FeatureValue_t;

typedef struct{
  BCS_App_Opcode_Notification_evt_t  BCS_Evt_Opcode;
}BCS_App_Notification_evt_t;

typedef struct{
  uint16_t  Year;
  uint8_t   Month;
  uint8_t   Day;
  uint8_t   Hours;
  uint8_t   Minutes;
  uint8_t   Seconds;
}BCS_TimeStamp_t;  /* Size (in octets) = 7 */

typedef struct {
  uint16_t Flags;
  uint16_t BodyFatPercentage;
  BCS_TimeStamp_t TimeStamp;
  uint8_t UserID;
  uint16_t BasalMetabolism;
  uint16_t MusclePercentage;
  uint16_t MuscleMass;
  uint16_t FatFreeMass;
  uint16_t SoftLeanMass;
  uint16_t BodyWaterMass;
  uint16_t Impedance;
  uint16_t Weight;
  uint16_t Height;
} BCS_MeasurementValue_t;

/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void BCS_Init(void);
void BCS_App_Notification(BCS_App_Notification_evt_t * pNotification);
void BCS_Update_Char(uint16_t UUID, uint8_t *pPayload);

#ifdef __cplusplus
}
#endif

#endif /*__BCS_H */


