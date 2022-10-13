/**
  ******************************************************************************
  * @file    wss.h
  * @author  GPM Application Team
  * @brief   Header for wss.c module
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
#ifndef __WSS_H
#define __WSS_H

#ifdef __cplusplus
extern "C" 
{
#endif

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
 typedef enum
{
  WSS_FEATURE_TIME_STAMP           = 1<<0,
  WSS_FEATURE_MULTIPLE_USERS       = 1<<1,
  WSS_FEATURE_BMI                  = 1<<2,
  WSS_FEATURE_RESOLUTION_0_5KG     = 1<<3,
  WSS_FEATURE_RESOLUTION_0_2KG     = 2<<3,
  WSS_FEATURE_RESOLUTION_0_1KG     = 3<<3,
  WSS_FEATURE_RESOLUTION_0_05KG    = 4<<3,
  WSS_FEATURE_RESOLUTION_0_02KG    = 5<<3,
  WSS_FEATURE_RESOLUTION_0_01KG    = 6<<3,
  WSS_FEATURE_RESOLUTION_0_005KG   = 7<<3,
  WSS_FEATURE_RESOLUTION_0_01m     = 1<<7,
  WSS_FEATURE_RESOLUTION_0_005m    = 2<<7,
  WSS_FEATURE_RESOLUTION_0_001m    = 3<<7,
} WSS_WeightScaleFeatureOption_t;

typedef enum
{
  WSS_MEASUREMENT_IND_DISABLED_EVT=0,
  WSS_MEASUREMENT_IND_ENABLED_EVT
} WSS_App_Opcode_Notification_evt_t;

typedef enum
{
  WSS_NO_FLAGS = 0,
  WSS_FLAGS_VALUE_UNIT_IMPERIAL= (1<<0),
  WSS_FLAGS_TIME_STAMP_PRESENT = (1<<1),
  WSS_FLAGS_USER_ID_PRESENT = (1<<2),
  WSS_FLAGS_BMI_AND_HEIGHT_PRESENT = (1<<3),
} WSS_WM_Flags_t;

typedef struct
{
  uint32_t Value;
  
} WSS_FeatureValue_t;

typedef struct{
  WSS_App_Opcode_Notification_evt_t  WSS_Evt_Opcode;
}WSS_App_Notification_evt_t;

typedef struct{
  uint16_t  Year;
  uint8_t   Month;
  uint8_t   Day;
  uint8_t   Hours;
  uint8_t   Minutes;
  uint8_t   Seconds;
}WSS_TimeStamp_t;

typedef struct {
  uint16_t Weight;
  WSS_TimeStamp_t TimeStamp;
  uint8_t UserID;
  uint16_t BMI;
  uint16_t Height;
  uint8_t Flags;
} WSS_MeasurementValue_t;

/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void WSS_Init(void);
void WSS_App_Notification(WSS_App_Notification_evt_t * pNotification);
void WSS_Update_Char(uint16_t UUID, uint8_t *pPayload);

#ifdef __cplusplus
}
#endif

#endif /*__WSS_H */


