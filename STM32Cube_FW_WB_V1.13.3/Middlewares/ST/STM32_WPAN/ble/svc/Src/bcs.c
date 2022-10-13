/**
 ******************************************************************************
 * @file    bcs.c
 * @author  GPM Application Team
 * @brief   Body Composition Service
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


/* Includes ------------------------------------------------------------------*/
#include "common_blesvc.h"
#include "bcs.h"


/* 16-bit UUID */
/*
 * || Allocation type                     | Allocated UUID | Allocated for                 ||
 * |+-------------------------------------+----------------+-------------------------------+|
 * || GATT Service                        | 0x181B         | Body Composition              ||
 * || GATT Characteristic and Object Type | 0x2A9B         | Body Composition Feature      ||
 * || GATT Characteristic and Object Type | 0x2A9C         | Body Composition Measurement  ||
 */


/* Private typedef -----------------------------------------------------------*/
typedef struct {
  uint16_t SvcHdle;                     /**< Service handle, Body Composition Service */
  /* Mandatory Service Characteristics */
  uint16_t FeatureCharHdle;             /**< Service Characteristic handle, Body Composition Feature */
  uint16_t MeasurementCharHdle;         /**< Service Characteristic handle, Body Composition Measurement */
  /* No optional Service Characteristics */
} BCS_Context_t;


/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Store Value into a buffer in Little Endian Format */
#define STORE_LE_16(buf, val)    ( ((buf)[0] =  (uint8_t) (val)    ) , \
                                   ((buf)[1] =  (uint8_t) (val>>8) ) )


/* Private variables ---------------------------------------------------------*/
static BCS_Context_t BCS_Context;


/* Private function prototypes -----------------------------------------------*/
static SVCCTL_EvtAckStatus_t BCS_Event_Handler(void *pckt);
static void Update_Char_Measurement(BCS_MeasurementValue_t *pMeasurement);
static void Update_Char_Feature(BCS_FeatureValue_t *pFeatureValue);

/* Public functions ----------------------------------------------------------*/

/**
 * @brief  Service initialization
 * @param  None
 * @retval None
 */
void BCS_Init(void){
  tBleStatus hciCmdResult = BLE_STATUS_FAILED;
  uint16_t uuid;

  /**
   *  Register the event handler to the BLE controller
   */
  SVCCTL_RegisterSvcHandler(BCS_Event_Handler);

  /**
   *  Add Body Composition Service
   *
   * Max_Attribute_Records = 2*no_of_char + 1
   * service_max_attribute_record = 1 for Body Composition service +
   *                                2 for Body Composition feature characteristic +
   *                                2 for Body Composition measurement characteristic +
   *                                1 for client char configuration descriptor +
   *                                
   */
  uuid = BODY_COMPOSITION_SERVICE_UUID;
  hciCmdResult = aci_gatt_add_service(UUID_TYPE_16,
                    (Service_UUID_t *) &uuid,
                    PRIMARY_SERVICE,
                    6,
                    &(BCS_Context.SvcHdle));

   if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_BCS_MSG ("Body Composition Service is added Successfully %04X\n\r",
                  BCS_Context.SvcHdle);
  }
  else
  {
    BLE_DBG_BCS_MSG ("FAILED to add Body Composition Service: Error: %02X !!\n\r",
                 hciCmdResult);
  }
  
  /**
   *  Add Body Composition Feature Characteristic
   */
  uuid = BODY_COMPOSITION_FEATURE_CHARAC;
  hciCmdResult = aci_gatt_add_char(BCS_Context.SvcHdle,
                    UUID_TYPE_16,
                    (Char_UUID_t *) &uuid ,
                    4,                                   /** 4 octets */
                    CHAR_PROP_READ, 
                    ATTR_PERMISSION_NONE,
                    GATT_DONT_NOTIFY_EVENTS, /* gattEvtMask */
                    10, /* encryKeySize */
                    0, /* isVariable */
                    &(BCS_Context.FeatureCharHdle));

  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_BCS_MSG ("Body Composition Feature Characteristic Added Successfully %04X\n\r",
                 BCS_Context.FeatureCharHdle);
  }
  else
  {
    BLE_DBG_BCS_MSG ("FAILED to add Body Composition Feature Characteristic: Error: %02X !!\n\r",
                 hciCmdResult);
  }

  /**
   *  Add Body Composition Measurement Characteristic
   *  By default all features are supported
   */
  uuid = BODY_COMPOSITION_MEASUREMENT_CHAR_UUID;;
  hciCmdResult = aci_gatt_add_char(BCS_Context.SvcHdle,
                    UUID_TYPE_16,
                    (Char_UUID_t *) &uuid ,
                    2                                         /** Flags, 2 octets */
                   +2                                         /** Body Fat Percentage, 2 octets */
                   +7                                         /** Time Stamp, 7 octets */
                   +1                                         /** User ID, 1 octets */
                   +2                                         /** Basal Metabolism, 2 octets */
                   +2                                         /** Muscle Percentage, 2 octets */
                   +2                                         /** Muscle Mass, 2 octets */
                   +2                                         /** Fat Free Mass, 2 octets */
                   +2                                         /** Soft Lean Mass, 2 octets */
                   +2                                         /** Body Water Mass, 2 octets */
                   +2                                         /** Impedance, 2 octets */
                   +2                                         /** Weight, 2 octets */
                   +2                                         /** Height, 2 octets */
				   +0,                                        /** The empty as the last one */
                    CHAR_PROP_INDICATE,
                    ATTR_PERMISSION_NONE,
                    GATT_DONT_NOTIFY_EVENTS, /* gattEvtMask */
                    10, /* encryKeySize */
                    1, /* isVariable: Variable length */
                    &(BCS_Context.MeasurementCharHdle));

  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_BCS_MSG ("Body Composition Measurement Characteristic Added Successfully %04X\n\r",
                 BCS_Context.MeasurementCharHdle);
  }
  else
  {
    BLE_DBG_BCS_MSG ("FAILED to add Body Composition Measurement Characteristic: Error: %02X !!\n\r",
                 hciCmdResult);
  }
}

void BCS_Update_Char(uint16_t UUID, uint8_t *pPayload){
  switch (UUID){
  case BODY_COMPOSITION_MEASUREMENT_CHAR_UUID:
    Update_Char_Measurement((BCS_MeasurementValue_t*)pPayload);
    break;
  case BODY_COMPOSITION_FEATURE_CHARAC:
    Update_Char_Feature((BCS_FeatureValue_t*)pPayload);
    break;
  default:
    /* do nothing */
    break;
  }
}


/* Private functions ---------------------------------------------------------*/
/**
 * @brief  Event handler
 * @param  Event: Address of the buffer holding the Event
 * @retval Ack: Return whether the Event has been managed or not
 */
static SVCCTL_EvtAckStatus_t BCS_Event_Handler(void *Event)
{
  SVCCTL_EvtAckStatus_t return_value;
  hci_event_pckt *event_pckt;
  evt_blue_aci *blue_evt;
  aci_gatt_attribute_modified_event_rp0    * attribute_modified;
  BCS_App_Notification_evt_t Notification;

  return_value = SVCCTL_EvtNotAck;
  event_pckt = (hci_event_pckt *)(((hci_uart_pckt*)Event)->data);

  switch(event_pckt->evt)
  {
    case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE:
    {
      blue_evt = (evt_blue_aci*)event_pckt->data;
      switch(blue_evt->ecode)
      {
        case EVT_BLUE_GATT_ATTRIBUTE_MODIFIED:
        {
          attribute_modified = (aci_gatt_attribute_modified_event_rp0*)blue_evt->data;
          if(attribute_modified->Attr_Handle == (BCS_Context.MeasurementCharHdle + 2))
          {
            return_value = SVCCTL_EvtAckFlowEnable;
            /**
             * Notify to application to start measurement
             */
            if(attribute_modified->Attr_Data[0] & COMSVC_Indication)
            {
              Notification.BCS_Evt_Opcode = BCS_MEASUREMENT_IND_ENABLED_EVT;
              BCS_App_Notification(&Notification);
            }
            else
            {
              Notification.BCS_Evt_Opcode = BCS_MEASUREMENT_IND_DISABLED_EVT;
              BCS_App_Notification(&Notification);
            }
          }
        }
        break;

        default:
          break;
      }
    }
    break; /* HCI_EVT_VENDOR_SPECIFIC */

    default:
      break;
  }

  return(return_value);
}/* end BCS_Event_Handler */

/**
 * @brief  Body Composition Measurement Characteristic update
 * @param  Service_Instance: Instance of the service to which the characteristic belongs
 * @param  pMeasurement: The address of the new value to be written
 * @retval None
 */
static void Update_Char_Measurement(BCS_MeasurementValue_t *pMeasurement){
  uint8_t bcm_value [
					2                                         /** Flags, 2 octets */
				   +2                                         /** Body Fat Percentage, 2 octets */
				   +7                                         /** Time Stamp, 7 octets */
				   +1                                         /** User ID, 1 octets */
				   +2                                         /** Basal Metabolism, 2 octets */
				   +2                                         /** Muscle Percentage, 2 octets */
				   +2                                         /** Muscle Mass, 2 octets */
				   +2                                         /** Fat Free Mass, 2 octets */
				   +2                                         /** Soft Lean Mass, 2 octets */
				   +2                                         /** Body Water Mass, 2 octets */
				   +2                                         /** Impedance, 2 octets */
				   +2                                         /** Weight, 2 octets */
				   +2                                         /** Height, 2 octets */
				   +0                                        /** The empty as the last one */
                    ];
  uint8_t length = 0;
  
  /*
   * Flags update
   */
  STORE_LE_16(bcm_value, pMeasurement->Flags);
  length = 2;
  
  /*
   * Body Fat Percentage
   */
  STORE_LE_16(bcm_value + length, pMeasurement->BodyFatPercentage);
  length += 2;
  
  /*
   * Time Stamp
   */
  if (pMeasurement->Flags & BCS_FLAG_TIME_STAMP_PRESENT){
    STORE_LE_16(bcm_value + length, pMeasurement->TimeStamp.Year);
    length += 2;
    bcm_value[length++] = pMeasurement->TimeStamp.Month;
    bcm_value[length++] = pMeasurement->TimeStamp.Day;
    bcm_value[length++] = pMeasurement->TimeStamp.Hours;
    bcm_value[length++] = pMeasurement->TimeStamp.Minutes;
    bcm_value[length++] = pMeasurement->TimeStamp.Seconds;
  }
  
  /*
   * User ID
   */
  if (pMeasurement->Flags & BCS_FLAG_USER_ID_PRESENT){
    bcm_value[length++] = pMeasurement->UserID;
  }
  
  /*
   * Basal Metabolism
   */
  if (pMeasurement->Flags & BCS_FLAG_BASAL_METABOLISM_PRESENT){
    STORE_LE_16(bcm_value + length, pMeasurement->BasalMetabolism);
    length += 2;
  }

  /*
   * Muscle Percentage
   */
  if (pMeasurement->Flags & BCS_FLAG_MUSCLE_PERCENTAGE_PRESENT){
    STORE_LE_16(bcm_value + length, pMeasurement->MusclePercentage);
    length += 2;
  }

  /*
   * Muscle Mass
   */
  if (pMeasurement->Flags & BCS_FLAG_MUSCLE_MASS_PRESENT){
    STORE_LE_16(bcm_value + length, pMeasurement->MuscleMass);
    length += 2;
  }

  /*
   * Fat Free Mass
   */
  if (pMeasurement->Flags & BCS_FLAG_FAT_FREE_MASS_PRESENT){
    STORE_LE_16(bcm_value + length, pMeasurement->FatFreeMass);
    length += 2;
  }

  /*
   * Soft Lean Mass
   */
  if (pMeasurement->Flags & BCS_FLAG_SOFT_LEAN_MASS_PRESENT){
    STORE_LE_16(bcm_value + length, pMeasurement->SoftLeanMass);
    length += 2;
  }

  /*
   * Body Water Mass
   */
  if (pMeasurement->Flags & BCS_FLAG_BODY_WATER_MASS_PRESENT){
    STORE_LE_16(bcm_value + length, pMeasurement->BodyWaterMass);
    length += 2;
  }

  /*
   * Impedance
   */
  if (pMeasurement->Flags & BCS_FLAG_IMPEDANCE_PRESENT){
    STORE_LE_16(bcm_value + length, pMeasurement->Impedance);
    length += 2;
  }

  /*
   * Weight
   */
  if (pMeasurement->Flags & BCS_FLAG_WEIGHT_PRESENT){
    STORE_LE_16(bcm_value + length, pMeasurement->Weight);
    length += 2;
  }

  /*
   * Height
   */
  if (pMeasurement->Flags & BCS_FLAG_HEIGHT_PRESENT){
    STORE_LE_16(bcm_value + length, pMeasurement->Height);
    length += 2;
  }

  aci_gatt_update_char_value(BCS_Context.SvcHdle,
                             BCS_Context.MeasurementCharHdle,
                             0,               /* charValOffset */
                             length,          /* charValLength */
                             bcm_value);
}

/**
 * @brief  Feature Characteristic update
 * @param  Service_Instance: Instance of the service to which the characteristic belongs
 * @param  pFeatureValue: The address of the new value to be written
 * @retval None
 */
static void Update_Char_Feature(BCS_FeatureValue_t *pFeatureValue)
{
  uint8_t wsf_value[4];
  /**
   *  Temperature Measurement Value
   */
  wsf_value[0] = (uint8_t)(pFeatureValue->Value & 0xFF);
  wsf_value[1] = (uint8_t)((pFeatureValue->Value >> 8) & 0xFF);
  wsf_value[2] = (uint8_t)((pFeatureValue->Value >> 16) & 0xFF);
  wsf_value[3] = (uint8_t)((pFeatureValue->Value >> 24) & 0xFF);
  
  aci_gatt_update_char_value(BCS_Context.SvcHdle,
                             BCS_Context.FeatureCharHdle,
                             0, /* charValOffset */
                             4, /* charValueLen */
                             (uint8_t *)  &wsf_value[0]);
}/* end Update_Char_Feature() */

