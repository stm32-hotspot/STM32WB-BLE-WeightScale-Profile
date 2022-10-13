/**
  ******************************************************************************
  * @file    wss.h
  * @author  GPM Application Team
  * @brief   Header for cts.c module
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
#ifndef __CTS_H
#define __CTS_H

#ifdef __cplusplus
extern "C" 
{
#endif

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct{
  uint16_t  Year;
  uint8_t   Month;
  uint8_t   Day;
  uint8_t   Hours;
  uint8_t   Minutes;
  uint8_t   Seconds;
}CTS_DateTime_t;

typedef struct {
	CTS_DateTime_t date_time;
	uint8_t day_of_week;
} CTS_DayDateTime_t;

typedef struct {
	CTS_DayDateTime_t day_date_time;
	uint8_t           fractions256;
} CTS_ExactTime256_Ch_t;

typedef struct {
	CTS_ExactTime256_Ch_t exact_time_256;
	uint8_t               adjust_reason;
} CTS_Ch_t;

typedef enum
{
  CTS_NOTIFY_DISABLED_EVT=0,
  CTS_NOTIFY_ENABLED_EVT
} CTS_App_Opcode_Notification_evt_t;

typedef struct{
  CTS_App_Opcode_Notification_evt_t  CTS_Evt_Opcode;
}CTS_App_Notification_evt_t;

typedef enum {
	CTS_ADJUST_MANUAL_TIME_UPDATE = (1<<0),
	CTS_ADJUST_EXTERNAL_REFERENCE_TIME_UPDATE = (1<<1),
	CTS_ADJUST_CHANGE_OF_TIME_ZONE = (1<<2),
	CTS_ADJUST_CHANGE_OF_DST = (1<<3),
	CTS_ADJUST_REASON_RESERVED = (1<<4),
} CTS_AdjustReason_t;

typedef enum {
	CTS_ERR_CODE_DATA_FIELD_IGNORED = 0x80,
} CTS_ErrorCode_t;

/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void CTS_Init(void);
void CTS_App_Notification(CTS_App_Notification_evt_t * pNotification);
void CTS_Update_Char(uint16_t UUID, uint8_t *pPayload);

#ifdef __cplusplus
}
#endif

#endif /*__CTS_H */


