/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    bcs_app.c
  * @author  MCD Application Team
  * @brief   Body Composition Service Application
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
#include "stm32_seq.h"
#include "bcs.h"
#include "bcs_app.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef struct{
  BCS_MeasurementValue_t MeasurementChar;
  BCS_FeatureValue_t FeatureChar;
  
  uint8_t Indication_Status;
  uint8_t TimerMeasurement_Id;
  uint32_t StartTick;
} BCSAPP_Context_t;

typedef enum {
  MeasurementUnits_SI = 0,   /* Weight and Mass in units of kilogram (kg) and Height in units of meter */
  MeasurementUnits_Imperial  /* Weight and Mass in units of pound (lb) and Height in units of inch (in) */
} Measurement_Unit_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines ------------------------------------------------------------*/
#define DEFAULT_WEIGHT_IN_KG                                                  70
#define DEFAULT_HEIGHT_IN_METERS                                             1.7f
#define DEFAULT_BCS_TIME_STAMP_YEAR                                         2022
#define DEFAULT_BCS_TIME_STAMP_MONTH                                           7
#define DEFAULT_BCS_TIME_STAMP_DAY                                             5
#define DEFAULT_BCS_TIME_STAMP_HOURS                                           0
#define DEFAULT_BCS_TIME_STAMP_MINUTES                                         0
#define DEFAULT_BCS_TIME_STAMP_SECONDS                                         0
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros -------------------------------------------------------------*/
#define BCS_MEASUREMENT_INTERVAL   (1000000/CFG_TS_TICK_VAL)  /**< 1s */

/* Field Weight of Body Composition Measurement */
#define POUNDS_TO_KILOGRAMS        0.4536
#define INCHES_TO_METERS           0.3048

/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
static BCSAPP_Context_t BCSAPP_Context;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void BcMeas( void );
/* Weight */
static uint8_t BcFeatureField_WMR(void);
static Measurement_Unit_t BcMeasurementUnits_Weight(void);
static float BcMeasurementResolution_Weight(uint8_t feature, Measurement_Unit_t unit);
static uint16_t BcConvert_Weight(Measurement_Unit_t unit_from, Measurement_Unit_t unit_to, float weight);
/* Height */
static uint8_t BcFeatureField_HMR(void);
static Measurement_Unit_t BcMeasurementUnits_Height(void);
static float BcMeasurementResolution_Height(uint8_t feature, Measurement_Unit_t unit);
static uint16_t BcConvert_Height(Measurement_Unit_t unit_from, Measurement_Unit_t unit_to, float height);
/* Measurement */
static void BCSAPP_Measurement(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
static void BcMeas( void )
{
  /**
   * The code shall be executed in the background as aci command may be sent
   * The background is the only place where the application can make sure a new aci command
   * is not sent if there is a pending one
   */
  UTIL_SEQ_SetTask( 1<<CFG_TASK_BCS_MEAS_REQ_ID, CFG_SCH_PRIO_0);

  return;
}

/*
 * Get Weight Measurement Resolution from Body Composition Feature field
 */
static uint8_t BcFeatureField_WMR(void){
  //APP_DBG_MSG("Feature in BcFeatureField_WMR = %X\n\r", BCSAPP_Context.FeatureChar.Value);
  return ((BCSAPP_Context.FeatureChar.Value >> 3) & 0x0F);
}

/*
 * Get Measurement Units from Flags of Body Composition Measurement
 */
static Measurement_Unit_t BcMeasurementUnits_Weight(void){
  //APP_DBG_MSG("Flags in BcMeasurementUnits_Weight = %X\n\r", BCSAPP_Context.MeasurementChar.Flags);
  return (Measurement_Unit_t)(BCSAPP_Context.MeasurementChar.Flags & 1);
}

/*
 * Weight Measurement Resolution from Body Composition Feature field
 * Return value is the resolution for kilograms (kg) or resolution for pounds
 */
static float BcMeasurementResolution_Weight(uint8_t feature, Measurement_Unit_t unit){
  float resolution = 1.0f; /* the default is Not specified */
  
  switch (feature){
  case 1:
    /* Resolution of 0.5 kg or 1 lb */
    resolution = (unit == MeasurementUnits_SI) ? 0.5f : 1.0f;
    break;
  case 2:
    /* Resolution of 0.2 kg or 0.5 lb */
    resolution = (unit == MeasurementUnits_SI) ? 0.2f : 0.5f;
    break;
  case 3:
    /* Resolution of 0.1 kg or 0.2 lb */
    resolution = (unit == MeasurementUnits_SI) ? 0.1f : 0.2f;
    break;
  case 4:
    /* Resolution of 0.05 kg or 0.1 lb */
    resolution = (unit == MeasurementUnits_SI) ? 0.05f : 0.1f;
    break;
  case 5:
    /* Resolution of 0.02 kg or 0.05 lb */
    resolution = (unit == MeasurementUnits_SI) ? 0.02f : 0.05f;
    break;
  case 6:
    /* Resolution of 0.01 kg or 0.02 lb */
    resolution = (unit == MeasurementUnits_SI) ? 0.01f : 0.02f;
    break;
  case 7:
    /* Resolution of 0.005 kg or 0.01 lb */
    resolution = (unit == MeasurementUnits_SI) ? 0.005f : 0.01f;
    break;
  default:
    /* do nothing */
    break;
  }
  
  //APP_DBG_MSG("Get Weight resolution = %f with feature = %X and unit = %d\n\r", resolution, feature, unit);
  
  return resolution;
}

/*
 * This function will transfer weight value with appoint unit
 * The formular: 1 pound = 0.4536 kilogram
 */
static uint16_t BcConvert_Weight(Measurement_Unit_t unit_from, Measurement_Unit_t unit_to, float weight){
  uint32_t v;
  float resolution = BcMeasurementResolution_Weight(BcFeatureField_WMR(), BcMeasurementUnits_Weight());
  
  //APP_DBG_MSG("Convert Weight %f with resolution = %f, unit %d -> %d\n\r", weight, resolution, unit_from, unit_to);
  
  if((unit_from == MeasurementUnits_SI) && (unit_to == MeasurementUnits_Imperial)){
    /* kilograms (kg) -> pounds */
    weight /= POUNDS_TO_KILOGRAMS;
  } else if ((unit_from == MeasurementUnits_Imperial) && (unit_to == MeasurementUnits_SI)){
    /* pounds -> kilograms (kg) */
    weight *= POUNDS_TO_KILOGRAMS;
  }
  
  weight /= resolution;
  /* floor value */
  v = (uint32_t)(weight * 10);
  return (uint16_t)((v + 5) / 10);
}


/*
 * Get Weight Measurement Resolution from Body Composition Feature field
 */
static uint8_t BcFeatureField_HMR(void){
  //APP_DBG_MSG("Feature in BcFeatureField_HMR = %X\n\r", BCSAPP_Context.FeatureChar.Value);
  return ((BCSAPP_Context.FeatureChar.Value >> 7) & 0x07);
}


/*
 * Get Measurement Units from Flags of Body Composition Measurement
 */
static Measurement_Unit_t BcMeasurementUnits_Height(void){
  //APP_DBG_MSG("Flags in BcMeasurementUnits_Height = %X\n\r", BCSAPP_Context.MeasurementChar.Flags);
  return (Measurement_Unit_t)(BCSAPP_Context.MeasurementChar.Flags & 1);
}


/*
 * Height Measurement Resolution from Body Composition Feature field
 * Return value is the resolution for meters or resolution for inches
 */
static float BcMeasurementResolution_Height(uint8_t feature, Measurement_Unit_t unit){
  float resolution = 1.0f; /* the default is Not specified */
  
  switch (feature){
  case 1:
    /* Resolution of 0.01 meter or 1 inch */
    resolution = (unit == MeasurementUnits_SI) ? 0.01f : 1.0f;
    break;
  case 2:
    /* Resolution of 0.005 meter or 0.5 inch */
    resolution = (unit == MeasurementUnits_SI) ? 0.005f : 0.5f;
    break;
  case 3:
    /* Resolution of 0.001 meter or 0.1 inch */
    resolution = (unit == MeasurementUnits_SI) ? 0.001f : 0.1f;
    break;
  default:
    /* do nothing */
    break;
  }
  
  //APP_DBG_MSG("Get Height resolution = %f with feature = %X and unit = %d\n\r", resolution, feature, unit);
  
  return resolution;
}


/*
 * This function will transfer weight value with appoint unit
 * The formular: 1 pound = 0.4536 kilogram
 */
static uint16_t BcConvert_Height(Measurement_Unit_t unit_from, Measurement_Unit_t unit_to, float height){
  uint32_t v;
  float resolution = BcMeasurementResolution_Height(BcFeatureField_HMR(), BcMeasurementUnits_Height());
  
  //APP_DBG_MSG("Convert Height %f with resolution = %f, unit %d -> %d\n\r", height, resolution, unit_from, unit_to);
  
  if((unit_from == MeasurementUnits_SI) && (unit_to == MeasurementUnits_Imperial)){
    /* meters -> inches */
    height /= INCHES_TO_METERS;
  } else if ((unit_from == MeasurementUnits_Imperial) && (unit_to == MeasurementUnits_SI)){
    /* inches -> meters */
    height *= INCHES_TO_METERS;
  }
  
  height /= resolution;
  /* floor value */
  v = (uint32_t)(height * 10);
  return (uint16_t)((v + 5) / 10);
}


static void BCSAPP_Measurement(void)
{
  /*Weight, BMI,  Height Initialization*/
  
  /* E.g. weight = 0x36B0 for 70kg with resolution = 0.0005 */
  float weight_si = (float)(DEFAULT_WEIGHT_IN_KG + (rand() % 10));
  uint16_t weight = BcConvert_Weight(MeasurementUnits_SI,
                                    ((BCSAPP_Context.MeasurementChar.Flags & BCS_FLAG_MEASUREMENT_UNITS_IMPERIAL) ? MeasurementUnits_Imperial : MeasurementUnits_SI),
                                    weight_si);
  float height_si = (float)(DEFAULT_HEIGHT_IN_METERS + (rand() % 10));
  uint16_t height = BcConvert_Height(MeasurementUnits_SI,
                                    ((BCSAPP_Context.MeasurementChar.Flags & BCS_FLAG_MEASUREMENT_UNITS_IMPERIAL) ? MeasurementUnits_Imperial : MeasurementUnits_SI),
                                    height_si);
  uint32_t ticks = (HAL_GetTick() - BCSAPP_Context.StartTick) / 1000;
  
  //APP_DBG_MSG("weight_si = %f -> weight = %02X, height_si = %f -> height = %02X\n\r", weight_si, weight, height_si, height);
  
  APP_DBG_MSG("BCSAPP_Measurement ticks = %ld (delta = %ld)\n\r", HAL_GetTick(), ticks);
  
  if(BCSAPP_Context.Indication_Status == 0){
    APP_DBG_MSG("Stop BCS Measurement\n\r");
    HW_TS_Stop(BCSAPP_Context.TimerMeasurement_Id);
    return;
  }
  
  /* update Fat Percentage */
  BCSAPP_Context.MeasurementChar.BodyFatPercentage = 10;

#ifdef SUPPORT_MULTI_USERS
  /* update User ID */
  BCSAPP_Context.MeasurementChar.UserID = 1;
#endif /* SUPPORT_MULTI_USERS */

  /* update Weight */
  BCSAPP_Context.MeasurementChar.Weight = weight;
  
  /* update Height */
  BCSAPP_Context.MeasurementChar.Height = height;

  /* update Time Stamp */
  BCSAPP_Context.MeasurementChar.TimeStamp.Seconds = ticks % 60;
  if(ticks >= 60){
    BCSAPP_Context.MeasurementChar.TimeStamp.Minutes += ticks / 60;
  }

  if(BCSAPP_Context.MeasurementChar.TimeStamp.Minutes >= 60){
    BCSAPP_Context.MeasurementChar.TimeStamp.Minutes = (BCSAPP_Context.MeasurementChar.TimeStamp.Minutes % 60);
    
    BCSAPP_Context.MeasurementChar.TimeStamp.Hours += (BCSAPP_Context.MeasurementChar.TimeStamp.Minutes / 60);
  }
  
  if(BCSAPP_Context.MeasurementChar.TimeStamp.Hours >= 24){
    BCSAPP_Context.MeasurementChar.TimeStamp.Hours = (BCSAPP_Context.MeasurementChar.TimeStamp.Hours % 24);
    
    BCSAPP_Context.MeasurementChar.TimeStamp.Day += (BCSAPP_Context.MeasurementChar.TimeStamp.Hours / 24);
  }
  
  /* let's handle date simply without permanment calendar */
  if(BCSAPP_Context.MeasurementChar.TimeStamp.Day >= 30){
    BCSAPP_Context.MeasurementChar.TimeStamp.Day = ((BCSAPP_Context.MeasurementChar.TimeStamp.Day - 1) % 30) + 1;
    
    BCSAPP_Context.MeasurementChar.TimeStamp.Month += (BCSAPP_Context.MeasurementChar.TimeStamp.Day / 30);
  }
  
  if(BCSAPP_Context.MeasurementChar.TimeStamp.Month >= 12){
    BCSAPP_Context.MeasurementChar.TimeStamp.Month = ((BCSAPP_Context.MeasurementChar.TimeStamp.Month - 1) % 12) + 1;
    
    BCSAPP_Context.MeasurementChar.TimeStamp.Year += (BCSAPP_Context.MeasurementChar.TimeStamp.Month / 12);
  }

  if(BCSAPP_Context.Indication_Status){
    BCS_Update_Char(BODY_COMPOSITION_MEASUREMENT_CHAR_UUID, (uint8_t *)&BCSAPP_Context.MeasurementChar);
  }
}

/* Public functions ----------------------------------------------------------*/
void BCS_App_Notification(BCS_App_Notification_evt_t *pNotification)
{
  switch(pNotification->BCS_Evt_Opcode)
  {
    case BCS_MEASUREMENT_IND_ENABLED_EVT:
      BCSAPP_Context.Indication_Status = 1;
      break;
    case BCS_MEASUREMENT_IND_DISABLED_EVT:
      BCSAPP_Context.Indication_Status = 0;
      break;
    default:
      break;
  }

  APP_DBG_MSG("BCS_App_Notification, code = %d (tick = %ld), Indication_Status = %d\n\r", pNotification->BCS_Evt_Opcode, HAL_GetTick(), BCSAPP_Context.Indication_Status);

  return;
}

void BCSAPP_RemoveFeature(uint16_t feature){
	if(feature & BCS_FEATURE_WEIGHT){
		BCSAPP_Context.FeatureChar.Value &= (uint16_t)((~BCS_FEATURE_WEIGHT) & 0xFFFF);

		BCSAPP_Context.MeasurementChar.Flags &= (uint16_t)((~BCS_FLAG_WEIGHT_PRESENT) & 0xFFFF);
	}
	if(feature & BCS_FEATURE_HEIGHT){
		BCSAPP_Context.FeatureChar.Value &= (uint16_t)((~BCS_FEATURE_HEIGHT) & 0xFFFF);

		BCSAPP_Context.MeasurementChar.Flags &= (uint16_t)((~BCS_FLAG_HEIGHT_PRESENT) & 0xFFFF);
	}

	BCS_Update_Char(BODY_COMPOSITION_FEATURE_CHARAC, (uint8_t *)& BCSAPP_Context.FeatureChar);
}

void BCSAPP_Reset(void){
	APP_DBG_MSG("BCSAPP_Reset\n\r");

#ifndef UDS_SINGLE_TRUSTED_COLLECTOR
	/*
	* Reset Application Context
	*/
	BCSAPP_Context.Indication_Status = 0;  /* disable */
#endif /* ! UDS_SINGLE_TRUSTED_COLLECTOR */
}

void BCSAPP_Init(void)
{
  APP_DBG_MSG("BCSAPP_Init\n\r");
  
  /*
   * No Indication by default
   */
  BCSAPP_Context.Indication_Status                 = 0;
  
  /*
   * Ticks
   */
  BCSAPP_Context.StartTick                         = HAL_GetTick();
  
  /**
   * Initialize Body Composition Features
   */
  BCSAPP_Context.FeatureChar.Value = (BCS_FEATURE_SUPPORTED_TIME_STAMP |
		  BCS_FEATURE_WEIGHT |
		  BCS_FEATURE_HEIGHT |
		  BCS_FEATURE_WEIGHT_MEASUREMENT_RESOLUTION_0_005KG |
		  BCS_FEATURE_HEIGHT_MEASUREMENT_RESOLUTION_0_001M);
#ifdef SUPPORT_MULTI_USERS
  APP_DBG_MSG("Multi-Users is supported in BCS\n\r");
  BCSAPP_Context.FeatureChar.Value |= BCS_FEATURE_SUPPORTED_MULTIPLE_USERS;
#endif
  
  BCS_Update_Char(BODY_COMPOSITION_FEATURE_CHARAC, (uint8_t *)& BCSAPP_Context.FeatureChar);

  /**
   * Initialize Fields of BCS Measurement
   */
  /* Body Composition Measurement value unit */
  BCSAPP_Context.MeasurementChar.Flags             = BCS_FLAG_MEASUREMENT_UNITS_SI;
  
#ifdef SUPPORT_MULTI_USERS
  BCSAPP_Context.MeasurementChar.Flags            |= BCS_FLAG_USER_ID_PRESENT;
#endif /* SUPPORT_MULTI_USERS */

  /* Add support for Weight */
  BCSAPP_Context.MeasurementChar.Flags            |= BCS_FLAG_WEIGHT_PRESENT;
  BCSAPP_Context.MeasurementChar.Weight            = 0;
  
  /* Add support for Time Stamp */
  BCSAPP_Context.MeasurementChar.Flags            |= BCS_FLAG_TIME_STAMP_PRESENT;
  BCSAPP_Context.MeasurementChar.TimeStamp.Year    = DEFAULT_BCS_TIME_STAMP_YEAR;
  BCSAPP_Context.MeasurementChar.TimeStamp.Month   = DEFAULT_BCS_TIME_STAMP_MONTH;
  BCSAPP_Context.MeasurementChar.TimeStamp.Day     = DEFAULT_BCS_TIME_STAMP_DAY;
  BCSAPP_Context.MeasurementChar.TimeStamp.Hours   = DEFAULT_BCS_TIME_STAMP_HOURS;
  BCSAPP_Context.MeasurementChar.TimeStamp.Minutes = DEFAULT_BCS_TIME_STAMP_MINUTES;
  BCSAPP_Context.MeasurementChar.TimeStamp.Seconds = DEFAULT_BCS_TIME_STAMP_SECONDS;

  /* Add support for Height */
  BCSAPP_Context.MeasurementChar.Flags            |= BCS_FLAG_HEIGHT_PRESENT;
  BCSAPP_Context.MeasurementChar.Height            = 0;
  
  /*
   * Create timer for Body Composition Measurement
   */
  HW_TS_Create(CFG_TIM_PROC_ID_ISR, &(BCSAPP_Context.TimerMeasurement_Id), hw_ts_Repeated, BcMeas);
  
  /*
   * Register task for Body Composition Measurment
   */
  UTIL_SEQ_RegTask( 1<< CFG_TASK_BCS_MEAS_REQ_ID, UTIL_SEQ_RFU, BCSAPP_Measurement );
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */
