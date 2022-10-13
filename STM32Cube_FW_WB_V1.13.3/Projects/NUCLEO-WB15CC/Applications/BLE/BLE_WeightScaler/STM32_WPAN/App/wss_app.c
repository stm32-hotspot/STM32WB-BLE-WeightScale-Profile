/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    wss_app.c
  * @author  MCD Application Team
  * @brief   Weight Scale Service Application
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
#include "wss.h"
#include "wss_app.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef struct{
  WSS_MeasurementValue_t MeasurementChar;
  WSS_FeatureValue_t FeatureChar;
  
  uint8_t Indication_Status;
  uint8_t TimerMeasurement_Id;
  uint32_t StartTick;
} WSSAPP_Context_t;

typedef enum {
  MeasurementUnits_SI = 0,   /* Weight and Mass in units of kilogram (kg) and Height in units of meter */
  MeasurementUnits_Imperial  /* Weight and Mass in units of pound (lb) and Height in units of inch (in) */
} Measurement_Unit_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines ------------------------------------------------------------*/
#define DEFAULT_WEIGHT_IN_KG                                                  70
#define DEFAULT_HEIGHT_IN_METERS                                             1.7f
#define DEFAULT_WSS_TIME_STAMP_YEAR                                         2022
#define DEFAULT_WSS_TIME_STAMP_MONTH                                           7
#define DEFAULT_WSS_TIME_STAMP_DAY                                             5
#define DEFAULT_WSS_TIME_STAMP_HOURS                                           0
#define DEFAULT_WSS_TIME_STAMP_MINUTES                                         0
#define DEFAULT_WSS_TIME_STAMP_SECONDS                                         0
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros -------------------------------------------------------------*/
#define WSS_MEASUREMENT_INTERVAL   (1000000/CFG_TS_TICK_VAL)  /**< 1s */

/* Field Weight of Weight Scale Measurement */
#define POUNDS_TO_KILOGRAMS        0.4536
#define INCHES_TO_METERS           0.3048

/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
static WSSAPP_Context_t WSSAPP_Context;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void WsMeas( void );
/* Weight */
static uint8_t WsFeatureField_WMR(void);
static Measurement_Unit_t WsMeasurementUnits_Weight(void);
static float WsMeasurementResolution_Weight(uint8_t feature, Measurement_Unit_t unit);
static uint16_t WsConvert_Weight(Measurement_Unit_t unit_from, Measurement_Unit_t unit_to, float weight);
/* Height */
static uint8_t WsFeatureField_HMR(void);
static Measurement_Unit_t WsMeasurementUnits_Height(void);
static float WsMeasurementResolution_Height(uint8_t feature, Measurement_Unit_t unit);
static uint16_t WsConvert_Height(Measurement_Unit_t unit_from, Measurement_Unit_t unit_to, float height);
/* BMI */
static uint16_t WsCalculate_BMI(float weight, float height);
/* Measurement */
static void WSSAPP_Measurement(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
static void WsMeas( void )
{
  /**
   * The code shall be executed in the background as aci command may be sent
   * The background is the only place where the application can make sure a new aci command
   * is not sent if there is a pending one
   */
  UTIL_SEQ_SetTask( 1<<CFG_TASK_WSS_MEAS_REQ_ID, CFG_SCH_PRIO_0);

  return;
}

/*
 * Get Weight Measurement Resolution from Weight Scale Feature field
 */
static uint8_t WsFeatureField_WMR(void){
  //APP_DBG_MSG("Feature in WsFeatureField_WMR = %X\n\r", WSSAPP_Context.FeatureChar.Value);
  return ((WSSAPP_Context.FeatureChar.Value >> 3) & 0x0F);
}

/*
 * Get Measurement Units from Flags of Weight Scale Measurement
 */
static Measurement_Unit_t WsMeasurementUnits_Weight(void){
  //APP_DBG_MSG("Flags in WsMeasurementUnits_Weight = %X\n\r", WSSAPP_Context.MeasurementChar.Flags);
  return (Measurement_Unit_t)(WSSAPP_Context.MeasurementChar.Flags & 1);
}

/*
 * Weight Measurement Resolution from Weight Scale Feature field
 * Return value is the resolution for kilograms (kg) or resolution for pounds
 */
static float WsMeasurementResolution_Weight(uint8_t feature, Measurement_Unit_t unit){
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
static uint16_t WsConvert_Weight(Measurement_Unit_t unit_from, Measurement_Unit_t unit_to, float weight){
  uint32_t v;
  float resolution = WsMeasurementResolution_Weight(WsFeatureField_WMR(), WsMeasurementUnits_Weight());
  
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
 * Get Weight Measurement Resolution from Height Scale Feature field
 */
static uint8_t WsFeatureField_HMR(void){
  //APP_DBG_MSG("Feature in WsFeatureField_HMR = %X\n\r", WSSAPP_Context.FeatureChar.Value);
  return ((WSSAPP_Context.FeatureChar.Value >> 7) & 0x07);
}


/*
 * Get Measurement Units from Flags of Weight Scale Measurement
 */
static Measurement_Unit_t WsMeasurementUnits_Height(void){
  //APP_DBG_MSG("Flags in WsMeasurementUnits_Height = %X\n\r", WSSAPP_Context.MeasurementChar.Flags);
  return (Measurement_Unit_t)(WSSAPP_Context.MeasurementChar.Flags & 1);
}


/*
 * Height Measurement Resolution from Weight Scale Feature field
 * Return value is the resolution for meters or resolution for inches
 */
static float WsMeasurementResolution_Height(uint8_t feature, Measurement_Unit_t unit){
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
static uint16_t WsConvert_Height(Measurement_Unit_t unit_from, Measurement_Unit_t unit_to, float height){
  uint32_t v;
  float resolution = WsMeasurementResolution_Height(WsFeatureField_HMR(), WsMeasurementUnits_Height());
  
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


/*
 * Calcular: BMI = Weight / (Height ^ 2) with Unit is 0.1 kg/m2
 */
static uint16_t WsCalculate_BMI(float weight, float height){
  float resolution = 0.1f;
  float value = weight / (height * height);
  uint32_t v;
  
  value /= resolution;
  v = (uint32_t)(value * 10);
  return (uint16_t)((v + 5) / 10);
}


static void WSSAPP_Measurement(void)
{
  /*Weight, BMI,  Height Initialization*/
  
  /* E.g. weight = 0x36B0 for 70kg with resolution = 0.0005 */
  float weight_si = (float)(DEFAULT_WEIGHT_IN_KG + (rand() % 10));
  uint16_t weight = WsConvert_Weight(MeasurementUnits_SI,
                                    ((WSSAPP_Context.MeasurementChar.Flags & WSS_FLAGS_VALUE_UNIT_IMPERIAL) ? MeasurementUnits_Imperial : MeasurementUnits_SI),
                                    weight_si);
  float height_si = (float)(DEFAULT_HEIGHT_IN_METERS + (rand() % 10));
  uint16_t height = WsConvert_Height(MeasurementUnits_SI,
                                    ((WSSAPP_Context.MeasurementChar.Flags & WSS_FLAGS_VALUE_UNIT_IMPERIAL) ? MeasurementUnits_Imperial : MeasurementUnits_SI),
                                    height_si);
  uint16_t BMI = WsCalculate_BMI(weight_si, height_si);
  uint32_t ticks = (HAL_GetTick() - WSSAPP_Context.StartTick) / 1000;
  
  //APP_DBG_MSG("weight_si = %f -> weight = %02X, height_si = %f -> height = %02X\n\r", weight_si, weight, height_si, height);
  
  APP_DBG_MSG("WSSAPP_Measurement ticks = %ld (delta = %ld)\n\r", HAL_GetTick(), ticks);
  
  if(WSSAPP_Context.Indication_Status == 0){
    APP_DBG_MSG("Stop WSS Measurement\n\r");
    HW_TS_Stop(WSSAPP_Context.TimerMeasurement_Id);
    return;
  }

  /* update Weight */
  WSSAPP_Context.MeasurementChar.Weight = weight;
  
  /* update Height */
  WSSAPP_Context.MeasurementChar.Height = height;
  
  /* update BMI */
  WSSAPP_Context.MeasurementChar.BMI    = BMI;
  
  /* update User ID */
  WSSAPP_Context.MeasurementChar.UserID = 0x01;
  
  /* update Time Stamp */
  WSSAPP_Context.MeasurementChar.TimeStamp.Seconds = ticks % 60;
  if(ticks >= 60){
    WSSAPP_Context.MeasurementChar.TimeStamp.Minutes += ticks / 60;
  }

  if(WSSAPP_Context.MeasurementChar.TimeStamp.Minutes >= 60){
    WSSAPP_Context.MeasurementChar.TimeStamp.Minutes = (WSSAPP_Context.MeasurementChar.TimeStamp.Minutes % 60);
    
    WSSAPP_Context.MeasurementChar.TimeStamp.Hours += (WSSAPP_Context.MeasurementChar.TimeStamp.Minutes / 60);
  }
  
  if(WSSAPP_Context.MeasurementChar.TimeStamp.Hours >= 24){
    WSSAPP_Context.MeasurementChar.TimeStamp.Hours = (WSSAPP_Context.MeasurementChar.TimeStamp.Hours % 24);
    
    WSSAPP_Context.MeasurementChar.TimeStamp.Day += (WSSAPP_Context.MeasurementChar.TimeStamp.Hours / 24);
  }
  
  /* let's handle date simply without permanment calendar */
  if(WSSAPP_Context.MeasurementChar.TimeStamp.Day >= 30){
    WSSAPP_Context.MeasurementChar.TimeStamp.Day = ((WSSAPP_Context.MeasurementChar.TimeStamp.Day - 1) % 30) + 1;
    
    WSSAPP_Context.MeasurementChar.TimeStamp.Month += (WSSAPP_Context.MeasurementChar.TimeStamp.Day / 30);
  }
  
  if(WSSAPP_Context.MeasurementChar.TimeStamp.Month >= 12){
    WSSAPP_Context.MeasurementChar.TimeStamp.Month = ((WSSAPP_Context.MeasurementChar.TimeStamp.Month - 1) % 12) + 1;
    
    WSSAPP_Context.MeasurementChar.TimeStamp.Year += (WSSAPP_Context.MeasurementChar.TimeStamp.Month / 12);
  }

  if(WSSAPP_Context.Indication_Status){
    WSS_Update_Char(WEIGHT_SCALE_MEASUREMENT_CHAR_UUID, (uint8_t *)&WSSAPP_Context.MeasurementChar);
  }
}

/* Public functions ----------------------------------------------------------*/
void WSS_App_Notification(WSS_App_Notification_evt_t *pNotification)
{
  APP_DBG_MSG("WSS_App_Notification, code = %d (tick = %ld)\n\r", pNotification->WSS_Evt_Opcode, HAL_GetTick());
  
  switch(pNotification->WSS_Evt_Opcode)
  {
    case WSS_MEASUREMENT_IND_ENABLED_EVT:
      WSSAPP_Context.Indication_Status = 1;
      
//      HW_TS_Stop(WSSAPP_Context.TimerMeasurement_Id);
//      HW_TS_Start(WSSAPP_Context.TimerMeasurement_Id, WSS_MEASUREMENT_INTERVAL);
      break;

    case WSS_MEASUREMENT_IND_DISABLED_EVT:
      WSSAPP_Context.Indication_Status = 0;
      
//      HW_TS_Stop(WSSAPP_Context.TimerMeasurement_Id);
      break;

    default:
      break;
  }

  return;
}

void WSSAPP_Init(void)
{
  APP_DBG_MSG("WSSAPP_Init\n\r");
  
  /*
   * No Indication by default
   */
  WSSAPP_Context.Indication_Status                 = 0;
  
  /*
   * Ticks
   */
  WSSAPP_Context.StartTick                         = HAL_GetTick();
  
  /**
   * Initialize Weight Scale Features
   */
  WSSAPP_Context.FeatureChar.Value = (WSS_FEATURE_BMI | WSS_FEATURE_RESOLUTION_0_5KG | WSS_FEATURE_RESOLUTION_0_01m | WSS_FEATURE_TIME_STAMP);
#ifdef SUPPORT_MULTI_USERS
  APP_DBG_MSG("Multi-Users is supported in WSS\n\r");
  WSSAPP_Context.FeatureChar.Value |= WSS_FEATURE_MULTIPLE_USERS;
#endif /* SUPPORT_MULTI_USERS */

  WSS_Update_Char(WEIGHT_SCALE_FEATURE_CHAR_UUID, (uint8_t *)& WSSAPP_Context.FeatureChar);

  /**
   * Initialize Fields of WSS Measurement
   */
  /* Weight Scale Measurement value unit */
  WSSAPP_Context.MeasurementChar.Flags             = WSS_NO_FLAGS; /* unit as SI */
  
  /* Weight */
  WSSAPP_Context.MeasurementChar.Weight            = 0;
  
  /* Add support for Time Stamp */
  WSSAPP_Context.MeasurementChar.Flags            |= WSS_FLAGS_TIME_STAMP_PRESENT;
  WSSAPP_Context.MeasurementChar.TimeStamp.Year    = DEFAULT_WSS_TIME_STAMP_YEAR;
  WSSAPP_Context.MeasurementChar.TimeStamp.Month   = DEFAULT_WSS_TIME_STAMP_MONTH;
  WSSAPP_Context.MeasurementChar.TimeStamp.Day     = DEFAULT_WSS_TIME_STAMP_DAY;
  WSSAPP_Context.MeasurementChar.TimeStamp.Hours   = DEFAULT_WSS_TIME_STAMP_HOURS;
  WSSAPP_Context.MeasurementChar.TimeStamp.Minutes = DEFAULT_WSS_TIME_STAMP_MINUTES;
  WSSAPP_Context.MeasurementChar.TimeStamp.Seconds = DEFAULT_WSS_TIME_STAMP_SECONDS;
  
#ifdef SUPPORT_MULTI_USERS
  /* Add support for User ID */
  WSSAPP_Context.MeasurementChar.Flags            |= WSS_FLAGS_USER_ID_PRESENT;
  WSSAPP_Context.MeasurementChar.UserID            = 0;
#endif /* SUPPORT_MULTI_USERS */
  
  /* Add support for BMI & Height pair */
  WSSAPP_Context.MeasurementChar.Flags            |= WSS_FLAGS_BMI_AND_HEIGHT_PRESENT;
  WSSAPP_Context.MeasurementChar.BMI               = 0;
  WSSAPP_Context.MeasurementChar.Height            = 0;
  
  /*
   * Create timer for Weight Scale Measurement
   */
  HW_TS_Create(CFG_TIM_PROC_ID_ISR, &(WSSAPP_Context.TimerMeasurement_Id), hw_ts_Repeated, WsMeas);
  
  /*
   * Register task for Weight Scale Measurment
   */
  UTIL_SEQ_RegTask( 1<< CFG_TASK_WSS_MEAS_REQ_ID, UTIL_SEQ_RFU, WSSAPP_Measurement );
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */
