/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    cts.c
  * @author  MCD Application Team
  * @brief   Current Time Service Application
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019-2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_common.h"

#include "dbg_trace.h"
#include "ble.h"
#include "app_ble.h"
#include "app_conf.h"
#include "stm32_seq.h"
#include "cts.h"
#include "cts_app.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private defines ------------------------------------------------------------*/

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private typedef -----------------------------------------------------------*/
typedef struct{
	CTS_Ch_t cts_ch;

	uint8_t notify_status;
	uint32_t StartTick;
} CTSAPP_Context_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private macros -------------------------------------------------------------*/
#define DEFAULT_CTS_DATE_TIME_YEAR                                         2022
#define DEFAULT_CTS_DATE_TIME_MONTH                                           7
#define DEFAULT_CTS_DATE_TIME_DAY                                            29
#define DEFAULT_CTS_DATE_TIME_HOURS                                           0
#define DEFAULT_CTS_DATE_TIME_MINUTES                                         0
#define DEFAULT_CTS_DATE_TIME_SECONDS                                         0
#define DEFAULT_CTS_DAY_OF_WEEK                                               5 /* Friday */
#define DEFAULT_CTS_FRACTION256                                               0
#define DEFAULT_ADJUST_REASON                        CTS_ADJUST_REASON_RESERVED

/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
static CTSAPP_Context_t CTSAPP_Context;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void cts_notify(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
/* Static functions ----------------------------------------------------------*/
static void cts_notify(void){
	uint32_t seconds;
	uint8_t minutes;
	uint8_t hours;

	APP_DBG_MSG("CTS Notify (Tick = %ld, enabled = %d)\n\r", HAL_GetTick(), CTSAPP_Context.notify_status );

	if(CTSAPP_Context.notify_status == 0){
		return;
	}

	/* day date time */
	/*   we keep content of year, month, date, day of week in this application demo */
	seconds = (HAL_GetTick() - CTSAPP_Context.StartTick) / 1000;
	CTSAPP_Context.cts_ch.exact_time_256.day_date_time.date_time.Seconds = seconds % 60;
	minutes = seconds / 60;
	CTSAPP_Context.cts_ch.exact_time_256.day_date_time.date_time.Minutes = minutes % 60;
	hours = minutes / 24;
	CTSAPP_Context.cts_ch.exact_time_256.day_date_time.date_time.Minutes = hours % 24;

	/* adjust reason */
	CTSAPP_Context.cts_ch.adjust_reason = CTS_ADJUST_MANUAL_TIME_UPDATE;

	CTS_Update_Char(CURRENT_TIME_CHAR_UUID, (uint8_t*)&(CTSAPP_Context.cts_ch));
}

/* Public functions ----------------------------------------------------------*/

void CTSAPP_Reset(void){
	APP_DBG_MSG("CTSAPP_Reset\n\r");

	/* Reset Context */
	CTSAPP_Context.notify_status = 0; /* disable */
}

void CTSAPP_Init(void)
{
	APP_DBG_MSG("CTSAPP_Init\n\r");

	/* initialize Context */
	CTSAPP_Context.cts_ch.exact_time_256.day_date_time.date_time.Year = DEFAULT_CTS_DATE_TIME_YEAR;
	CTSAPP_Context.cts_ch.exact_time_256.day_date_time.date_time.Month = DEFAULT_CTS_DATE_TIME_MONTH;
	CTSAPP_Context.cts_ch.exact_time_256.day_date_time.date_time.Day = DEFAULT_CTS_DATE_TIME_DAY;
	CTSAPP_Context.cts_ch.exact_time_256.day_date_time.date_time.Hours = DEFAULT_CTS_DATE_TIME_HOURS;
	CTSAPP_Context.cts_ch.exact_time_256.day_date_time.date_time.Minutes = DEFAULT_CTS_DATE_TIME_MINUTES;
	CTSAPP_Context.cts_ch.exact_time_256.day_date_time.date_time.Seconds = DEFAULT_CTS_DATE_TIME_SECONDS;

	CTSAPP_Context.cts_ch.exact_time_256.day_date_time.day_of_week = DEFAULT_CTS_DAY_OF_WEEK;

	CTSAPP_Context.cts_ch.exact_time_256.fractions256 = DEFAULT_CTS_FRACTION256; /* The number of 1/256 fractions of a second. Valid range 0–255 */

	CTSAPP_Context.cts_ch.adjust_reason = DEFAULT_ADJUST_REASON;

	CTSAPP_Context.notify_status = 0; /* disable */

	CTSAPP_Context.StartTick = HAL_GetTick();

	/*
	* Register task for Current Time Notify
	*/
	UTIL_SEQ_RegTask( 1<< CFG_TASK_CTS_NOTIFY_ID, UTIL_SEQ_RFU, cts_notify );
}

void CTS_App_Notification(CTS_App_Notification_evt_t * pNotification){
	switch(pNotification->CTS_Evt_Opcode){
	case CTS_NOTIFY_DISABLED_EVT:
		CTSAPP_Context.notify_status = 0; /* disable */
		break;
	case CTS_NOTIFY_ENABLED_EVT:
		CTSAPP_Context.notify_status = 1; /* enable */
		break;
	default:
		/* do nothing */
		break;
	}
}
/* USER CODE BEGIN FD */

/* USER CODE END FD */
