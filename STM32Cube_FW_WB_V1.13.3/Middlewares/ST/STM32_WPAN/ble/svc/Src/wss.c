/**
 ******************************************************************************
 * @file    wss.c
 * @author  GPM Application Team
 * @brief   Weight Scale Service
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
#include "wss.h"


/* 16-bit UUID */
/*
 * || Allocation type                     | Allocated UUID | Allocated for         ||
 * |+-------------------------------------+----------------+-----------------------+|
 * || GATT Service                        | 0x181D         | Weight Scale          ||
 * || GATT Characteristic and Object Type | 0x2A9D         | Weight Measurement    ||
 * || GATT Characteristic and Object Type | 0x2A9E         | Weight Scale Feature  ||
 */


/* Private typedef -----------------------------------------------------------*/
typedef struct {
  uint16_t SvcHdle;                     /**< Service handle, Weight Scale Service */
  /* Mandatory Service Characteristics */
  uint16_t FeatureCharHdle;             /**< Service Characteristic handle, Weight Scale Feature */
  uint16_t MeasurementCharHdle;         /**< Service Characteristic handle, Weight Measurement */
  /* No optional Service Characteristics */
} WSS_Context_t;


/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Store Value into a buffer in Little Endian Format */
#define STORE_LE_16(buf, val)    ( ((buf)[0] =  (uint8_t) (val)    ) , \
                                   ((buf)[1] =  (uint8_t) (val>>8) ) )


/* Private variables ---------------------------------------------------------*/
static WSS_Context_t WSS_Context;


/* Private function prototypes -----------------------------------------------*/
static SVCCTL_EvtAckStatus_t WSS_Event_Handler(void *pckt);
static void Update_Char_WeightScaleMeasurement(WSS_MeasurementValue_t *pMeasurement);
static void Update_Char_Feature(WSS_FeatureValue_t *pFeatureValue);

/* Public functions ----------------------------------------------------------*/

/**
 * @brief  Service initialization
 * @param  None
 * @retval None
 */
void WSS_Init(void){
  tBleStatus hciCmdResult = BLE_STATUS_FAILED;
  uint16_t uuid;

  /**
   *  Register the event handler to the BLE controller
   */
  SVCCTL_RegisterSvcHandler(WSS_Event_Handler);

  /**
   *  Add Weight Scale Service
   *
   * Max_Attribute_Records = 2*no_of_char + 1
   * service_max_attribute_record = 1 for weight scale service +
   *                                2 for weight scale feature characteristic +
   *                                2 for weight scale measurement characteristic +
   *                                1 for client char configuration descriptor +
   *                                
   */
  uuid = WEIGHT_SCALE_SERVICE_UUID;
  hciCmdResult = aci_gatt_add_service(UUID_TYPE_16,
                    (Service_UUID_t *) &uuid,
                    PRIMARY_SERVICE,
                    6,
                    &(WSS_Context.SvcHdle));

   if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_WSS_MSG ("Weight Scale Service is added Successfully %04X\n\r",
                  WSS_Context.SvcHdle);
  }
  else
  {
    BLE_DBG_WSS_MSG ("FAILED to add Weight Scale Service: Error: %02X !!\n\r",
                 hciCmdResult);
  }
  
  /**
   *  Add Weight Scale Feature Characteristic
   */
  uuid = WEIGHT_SCALE_FEATURE_CHAR_UUID;
  hciCmdResult = aci_gatt_add_char(WSS_Context.SvcHdle,
                    UUID_TYPE_16,
                    (Char_UUID_t *) &uuid ,
                    4,                                   /** 4 octets */
                    CHAR_PROP_READ, 
                    ATTR_PERMISSION_NONE,
                    GATT_DONT_NOTIFY_EVENTS, /* gattEvtMask */
                    10, /* encryKeySize */
                    0, /* isVariable */
                    &(WSS_Context.FeatureCharHdle));

  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_WSS_MSG ("Weight Scale Feature Characteristic Added Successfully %04X\n\r",
                 WSS_Context.FeatureCharHdle);
  }
  else
  {
    BLE_DBG_WSS_MSG ("FAILED to add Weight Scale Feature Characteristic: Error: %02X !!\n\r",
                 hciCmdResult);
  }

  /**
   *  Add Weight Scale Measurement Characteristic
   */
  uuid = WEIGHT_SCALE_MEASUREMENT_CHAR_UUID;;
  hciCmdResult = aci_gatt_add_char(WSS_Context.SvcHdle,
                    UUID_TYPE_16,
                    (Char_UUID_t *) &uuid ,
                    1                                         /** Flags, 1 octets */
                   +2                                         /** Weight, 2 octets */
                   +7                                         /** Time Stamp, 7 octets */
                   +1                                         /** User ID, 1 octets */
                   +2                                         /** BMI, 2 octets */
                   +2,                                        /** Height, 2 octets */  
                    CHAR_PROP_INDICATE,
                    ATTR_PERMISSION_NONE,
                    GATT_DONT_NOTIFY_EVENTS, /* gattEvtMask */
                    10, /* encryKeySize */
                    1, /* isVariable: 1 */
                    &(WSS_Context.MeasurementCharHdle));

  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_WSS_MSG ("Weight Scale Measurement Characteristic Added Successfully %04X\n\r",
                 WSS_Context.MeasurementCharHdle);
  }
  else
  {
    BLE_DBG_WSS_MSG ("FAILED to add Weight Scale Measurement Characteristic: Error: %02X !!\n\r",
                 hciCmdResult);
  }
}

void WSS_Update_Char(uint16_t UUID, uint8_t *pPayload){
  switch (UUID){
  case WEIGHT_SCALE_MEASUREMENT_CHAR_UUID:
    Update_Char_WeightScaleMeasurement((WSS_MeasurementValue_t*)pPayload);
    break;
  case WEIGHT_SCALE_FEATURE_CHAR_UUID:
    Update_Char_Feature((WSS_FeatureValue_t*)pPayload);
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
static SVCCTL_EvtAckStatus_t WSS_Event_Handler(void *Event)
{
  SVCCTL_EvtAckStatus_t return_value;
  hci_event_pckt *event_pckt;
  evt_blue_aci *blue_evt;
  aci_gatt_attribute_modified_event_rp0    * attribute_modified;
  WSS_App_Notification_evt_t Notification;

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
          if(attribute_modified->Attr_Handle == (WSS_Context.MeasurementCharHdle + 2))
          {
            return_value = SVCCTL_EvtAckFlowEnable;
            /**
             * Notify to application to start measurement
             */
            if(attribute_modified->Attr_Data[0] & COMSVC_Indication)
            {
              Notification.WSS_Evt_Opcode = WSS_MEASUREMENT_IND_ENABLED_EVT;
              WSS_App_Notification(&Notification);
            }
            else
            {
              Notification.WSS_Evt_Opcode = WSS_MEASUREMENT_IND_DISABLED_EVT;
              WSS_App_Notification(&Notification);
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
}/* end WSS_Event_Handler */

/**
 * @brief  Weight Scale Measurement Characteristic update
 * @param  Service_Instance: Instance of the service to which the characteristic belongs
 * @param  pMeasurement: The address of the new value to be written
 * @retval None
 */
static void Update_Char_WeightScaleMeasurement(WSS_MeasurementValue_t *pMeasurement){
  uint8_t wsm_value [
                     1 +  /* Flags */
                     2 +  /* Weight */
                     7 +  /* Time Stamp */
                     1 +  /* User ID */
                     2 +  /* BMI */
                     2 +  /* Height */
                     0    /* The last one which means nothing */
                    ];
  uint8_t length = 0;
  
  /*
   * Flags update
   */
  wsm_value[length++] = pMeasurement->Flags;
  
  /*
   * Weight
   */
  STORE_LE_16(wsm_value + length, pMeasurement->Weight);
  length += 2;
  
  /*
   * Time Stamp
   */
  if (pMeasurement->Flags & WSS_FLAGS_TIME_STAMP_PRESENT){
    STORE_LE_16(wsm_value + length, pMeasurement->TimeStamp.Year);
    length += 2;
    wsm_value[length++] = pMeasurement->TimeStamp.Month;
    wsm_value[length++] = pMeasurement->TimeStamp.Day;
    wsm_value[length++] = pMeasurement->TimeStamp.Hours;
    wsm_value[length++] = pMeasurement->TimeStamp.Minutes;
    wsm_value[length++] = pMeasurement->TimeStamp.Seconds;
  }
  
  /*
   * User ID
   */
  if (pMeasurement->Flags & WSS_FLAGS_USER_ID_PRESENT){
    wsm_value[length++] = pMeasurement->UserID;
  }
  
  /*
   * BMI & Weight Pair
   */
  if (pMeasurement->Flags & WSS_FLAGS_BMI_AND_HEIGHT_PRESENT){
    STORE_LE_16(wsm_value + length, pMeasurement->BMI);
    length += 2;
    STORE_LE_16(wsm_value + length, pMeasurement->Height);
    length += 2;
  }
  
  aci_gatt_update_char_value(WSS_Context.SvcHdle,
                             WSS_Context.MeasurementCharHdle,
                             0,               /* charValOffset */
                             length,          /* charValLength */
                             wsm_value);
}

/**
 * @brief  Feature Characteristic update
 * @param  Service_Instance: Instance of the service to which the characteristic belongs
 * @param  pFeatureValue: The address of the new value to be written
 * @retval None
 */
static void Update_Char_Feature(WSS_FeatureValue_t *pFeatureValue)
{
  uint8_t wsf_value[4];
  /**
   *  Temperature Measurement Value
   */
  wsf_value[0] = (uint8_t)(pFeatureValue->Value);
  wsf_value[1] = (uint8_t)(pFeatureValue->Value >> 8);
  wsf_value[2] = (uint8_t)(pFeatureValue->Value >> 16);
  wsf_value[3] = (uint8_t)(pFeatureValue->Value >> 24);
  
  aci_gatt_update_char_value(WSS_Context.SvcHdle,
                             WSS_Context.FeatureCharHdle,
                             0, /* charValOffset */
                             4, /* charValueLen */
                             (uint8_t *)  &wsf_value[0]);
}/* end Update_Char_Feature() */

