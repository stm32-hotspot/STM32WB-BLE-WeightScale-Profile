/**
 ******************************************************************************
 * @file    uds.c
 * @author  GPM Application Team
 * @brief   User Data Service
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
#include "uds.h"


/* Private typedef -----------------------------------------------------------*/
typedef struct {
  uint16_t SvcHdle;                        /**< Service handle, Body Composition Service */
  /* Mandatory Service Characteristics */
  uint16_t UDS_HeightCharHdle;             /**< Service Characteristic handle, UDS Characteristics, Height */
  uint16_t UDS_WeightCharHdle;             /**< Service Characteristic handle, UDS Characteristics, Weight */
  uint16_t DatabaseChangeIncrementCharHdle;/**< Service Characteristic handle, Database Change Increment */
  uint16_t UserIndexCharHdle;              /**< Service Characteristic handle, User Index */
  uint16_t UserControlPointCharHdle;       /**< Service Characteristic handle, Body Composition Feature */
  /* Optional Service Characteristics */
  uint16_t UdsCharHdle;                    /**< Service Characteristic handle, USD Characteristics */
  uint16_t RegisteredUserCharHdle;         /**< Service Characteristic handle, Registered User */
} UDS_Context_t;


/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Store Value into a buffer in Little Endian Format */
#define STORE_LE_16(buf, val)    ( ((buf)[0] =  (uint8_t) (val & 0xFF)    ) , \
                                   ((buf)[1] =  (uint8_t) ((val>>8) & 0xFF) ) )

#define STORE_LE_32(buf, val)    ( ((buf)[0] =  (uint8_t) (val)     ) , \
                                   ((buf)[1] =  (uint8_t) (val>>8)  ) , \
                                   ((buf)[2] =  (uint8_t) (val>>16) ) , \
                                   ((buf)[3] =  (uint8_t) (val>>24) ) )

/* Private variables ---------------------------------------------------------*/
static UDS_Context_t UDS_Context;


/* Private function prototypes -----------------------------------------------*/
static SVCCTL_EvtAckStatus_t UDS_Event_Handler(void *pckt);
static void Populate_Char_User_Index(uint8_t user_index);
static void Populate_Char_Height(uint16_t height);
static void Populate_Char_Weight(uint16_t weight);
static void UDS_UserControlPoint_Message(UDC_ProcedureComplete_t *msg);

/* Public functions ----------------------------------------------------------*/

/**
 * @brief  Service initialization
 * @param  None
 * @retval None
 */
void UDS_Init(void){
  tBleStatus hciCmdResult = BLE_STATUS_FAILED;
  uint16_t uuid;

  /**
   *  Register the event handler to the BLE controller
   */
  SVCCTL_RegisterSvcHandler(UDS_Event_Handler);

  /**
   *  Add User Data Service
   *
   * Max_Attribute_Records = 2*no_of_char + 1
   * service_max_attribute_record = 1 for User Data service +
   *                                2 for UDS Characteristics - Height +
   *                                2 for UDS Characteristics - Weight +
   *                                2 for Database Change Increment characteristic +
   *                                    1 for client char configuration descriptor +
   *                                2 for User Index characteristic +
   *                                2 for User Control Point characteristic +
   *                                    1 for client char configuration descriptor +
   *
   */
  uuid = USER_DATA_SERVICE_UUID;
  hciCmdResult = aci_gatt_add_service(UUID_TYPE_16,
                    (Service_UUID_t *) &uuid,
                    PRIMARY_SERVICE,
                    13,
                    &(UDS_Context.SvcHdle));

   if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_UDS_MSG ("User Data Service is added Successfully %04X\n\r",
                  UDS_Context.SvcHdle);
  }
  else
  {
    BLE_DBG_UDS_MSG ("FAILED to add User Data Service: Error: %02X !!\n\r",
                 hciCmdResult);
  }

   /**
    *  Add UDS Characteristics - Height
    */
   uuid = HEIGHT_CHAR_UUID;
   hciCmdResult = aci_gatt_add_char(UDS_Context.SvcHdle,
                     UUID_TYPE_16,
                     (Char_UUID_t *) &uuid ,
                     2,                        /** 2 octets */
                     CHAR_PROP_READ | CHAR_PROP_WRITE,
                     ATTR_PERMISSION_NONE,
					 GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP | GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP, /* gattEvtMask */
                     10, /* encryKeySize */
                     0, /* isVariable */
                     &(UDS_Context.UDS_HeightCharHdle));

   if (hciCmdResult == BLE_STATUS_SUCCESS)
   {
     BLE_DBG_UDS_MSG ("UDS Characteristic - Height Added Successfully %04X\n\r",
                  UDS_Context.UDS_HeightCharHdle);
   }
   else
   {
     BLE_DBG_UDS_MSG ("FAILED to add UDS Characteristic - Height: Error: %02X !!\n\r",
                  hciCmdResult);
   }


   /**
    *  Add UDS Characteristics - Weight
    */
   uuid = WEIGHT_CHAR_UUID;
   hciCmdResult = aci_gatt_add_char(UDS_Context.SvcHdle,
                     UUID_TYPE_16,
                     (Char_UUID_t *) &uuid ,
                     2,                        /** 2 octets */
                     CHAR_PROP_READ | CHAR_PROP_WRITE,
                     ATTR_PERMISSION_NONE,
					 GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP | GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP, /* gattEvtMask */
                     10, /* encryKeySize */
                     0, /* isVariable */
                     &(UDS_Context.UDS_WeightCharHdle));

   if (hciCmdResult == BLE_STATUS_SUCCESS)
   {
     BLE_DBG_UDS_MSG ("UDS Characteristic - Weight Added Successfully %04X\n\r",
                  UDS_Context.UDS_WeightCharHdle);
   }
   else
   {
     BLE_DBG_UDS_MSG ("FAILED to add UDS Characteristic - Weight: Error: %02X !!\n\r",
                  hciCmdResult);
   }

   /**
   *  Add Database Change Increment Characteristic
   */
  uuid = DATABASE_CHANGE_INCREMENT_CHAR_UUID;
  hciCmdResult = aci_gatt_add_char(UDS_Context.SvcHdle,
                    UUID_TYPE_16,
                    (Char_UUID_t *) &uuid ,
                    4,                        /** 4 octets */
                    CHAR_PROP_READ | CHAR_PROP_WRITE | CHAR_PROP_NOTIFY,
                    ATTR_PERMISSION_NONE,
					GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP, /* gattEvtMask */
                    10, /* encryKeySize */
                    0, /* fixed-length */
                    &(UDS_Context.DatabaseChangeIncrementCharHdle));

  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_UDS_MSG ("Database Change Increment Characteristic Added Successfully %04X\n\r",
                 UDS_Context.DatabaseChangeIncrementCharHdle);
  }
  else
  {
    BLE_DBG_UDS_MSG ("FAILED to add Database Change Increment Characteristic: Error: %02X !!\n\r",
                 hciCmdResult);
  }

  /**
   *  Add User Index Characteristic
   *  By default all features are supported
   */
  uuid = USER_INDEX_CHAR_UUID;;
  hciCmdResult = aci_gatt_add_char(UDS_Context.SvcHdle,
                    UUID_TYPE_16,
                    (Char_UUID_t *) &uuid ,
					1,                       /* unit8 */
                    CHAR_PROP_READ,
                    ATTR_PERMISSION_NONE,
                    GATT_DONT_NOTIFY_EVENTS, /* gattEvtMask */
                    10, /* encryKeySize */
                    0, /* fixed-length */
                    &(UDS_Context.UserIndexCharHdle));

  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_UDS_MSG ("User Index Characteristic Added Successfully %04X\n\r",
                 UDS_Context.UserIndexCharHdle);
  }
  else
  {
    BLE_DBG_UDS_MSG ("FAILED to add User Index Characteristic: Error: %02X !!\n\r",
                 hciCmdResult);
  }

  /**
   *  Add User Control Point Characteristic
   *  By default all features are supported
   */
  uuid = USER_CONTROL_POINT_CHAR_UUID;;
  hciCmdResult = aci_gatt_add_char(UDS_Context.SvcHdle,
                    UUID_TYPE_16,
                    (Char_UUID_t *) &uuid,
                    1 +                      /** Op Code */
					17,                      /** Parameter */
                    CHAR_PROP_WRITE | CHAR_PROP_INDICATE,
                    ATTR_PERMISSION_NONE,
					GATT_NOTIFY_ATTRIBUTE_WRITE, /* gattEvtMask */
                    10, /* encryKeySize */
                    1, /* variable-length */
                    &(UDS_Context.UserControlPointCharHdle));

  if (hciCmdResult == BLE_STATUS_SUCCESS)
  {
    BLE_DBG_UDS_MSG ("User Control Point Characteristic Added Successfully %04X\n\r",
                 UDS_Context.UserControlPointCharHdle);
  }
  else
  {
    BLE_DBG_UDS_MSG ("FAILED to add User Control Point Characteristic: Error: %02X !!\n\r",
                 hciCmdResult);
  }
}

/* Private functions ---------------------------------------------------------*/

void UDS_Update_Char(uint16_t UUID, uint8_t *pPayload){
	switch(UUID){
	case HEIGHT_CHAR_UUID:
		Populate_Char_Height(*((uint16_t*)pPayload));
		break;
	case WEIGHT_CHAR_UUID:
		Populate_Char_Weight(*((uint16_t*)pPayload));
		break;
	case USER_INDEX_CHAR_UUID:
		Populate_Char_User_Index(pPayload[0]);
		break;
	case USER_CONTROL_POINT_CHAR_UUID:
		UDS_UserControlPoint_Message((UDC_ProcedureComplete_t*)pPayload);
		break;
	default:
		/* do nothing */
		break;
	}
}

/**
 * @brief  Event handler
 * @param  Event: Address of the buffer holding the Event
 * @retval Ack: Return whether the Event has been managed or not
 */
static SVCCTL_EvtAckStatus_t UDS_Event_Handler(void *Event)
{
  SVCCTL_EvtAckStatus_t return_value;
  hci_event_pckt *event_pckt;
  evt_blue_aci *blue_evt;
  aci_gatt_attribute_modified_event_rp0    * attribute_modified;
  aci_gatt_read_permit_req_event_rp0    * attribute_read;
  UDS_App_Notification_evt_t Notification;

  return_value = SVCCTL_EvtNotAck;
  event_pckt = (hci_event_pckt *)(((hci_uart_pckt*)Event)->data);

  //BLE_DBG_UDS_MSG("UDS_Event_Handler package event = 0x%X\n\r", event_pckt->evt);
  switch(event_pckt->evt)
  {
    case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE:
    {
      //blecore_evt = (evt_blecore_aci*)event_pckt->data;
      blue_evt = (evt_blue_aci*)event_pckt->data;
      //BLE_DBG_UDS_MSG("UDS_Event_Handler blue event code = 0x%X\n\r", blue_evt->ecode);
      switch(blue_evt->ecode)
      {
        case ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE:
        {
          attribute_modified = (aci_gatt_attribute_modified_event_rp0*)blue_evt->data;

          if(attribute_modified->Attr_Handle == (UDS_Context.UserControlPointCharHdle + 1))
          {
        	uint8_t op_code = attribute_modified->Attr_Data[0];

            if((op_code == 0) || (op_code >= 6)){
            	uint8_t ucp_value[3];

            	BLE_DBG_UDS_MSG("Unsupported operator code %d for User Control Point Characteristic\n\r", op_code);
            	/* operation code NOT supported */
            	ucp_value[0] = UDS_UCP_OPCODE_RESPONSE_CODE;
            	ucp_value[1] = op_code;
            	ucp_value[2] = UDS_RESPONSE_VALUE_OP_CODE_NOT_SUPPORTED;
    			aci_gatt_update_char_value(UDS_Context.SvcHdle,
    						UDS_Context.UserControlPointCharHdle,
    						0, /* charValOffset */
    						3 , /* charValueLen */
    						ucp_value);
    			return SVCCTL_EvtNotAck;
            }

            BLE_DBG_UDS_MSG("ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE UDS_USER_CONTROL_POINT_EVT(op code=%d)\n\r", op_code);

            Notification.UDS_Evt_Opcode = UDS_USER_CONTROL_POINT_EVT;
            Notification.DataTransfered.Length=attribute_modified->Attr_Data_Length;
            Notification.DataTransfered.pPayload=attribute_modified->Attr_Data;

            UDS_App_Notification(&Notification);

            return_value = SVCCTL_EvtAckFlowEnable;
          } else if(attribute_modified->Attr_Handle == (UDS_Context.UserControlPointCharHdle + 2)){
        	  if (attribute_modified->Attr_Data[0] & COMSVC_Indication) {
        		  Notification.UDS_Evt_Opcode = UDS_INDICATION_ENABLED;
        	  } else {
        		  Notification.UDS_Evt_Opcode = UDS_INDICATION_DISABLED;
        	  }

        	  BLE_DBG_UDS_MSG("ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE UDS_INDICATION (%d)\n\r", Notification.UDS_Evt_Opcode);

        	  Notification.DataTransfered.Length = 0;
        	  Notification.DataTransfered.pPayload = NULL;

        	  UDS_App_Notification(&Notification);

              return_value = SVCCTL_EvtAckFlowEnable;
          }
        }
        	break;

        case ACI_GATT_READ_PERMIT_REQ_VSEVT_CODE:
        	attribute_read = (aci_gatt_read_permit_req_event_rp0*)blue_evt->data;
        	if(attribute_read->Attribute_Handle == (UDS_Context.UDS_WeightCharHdle + 1)){
        		if(UDS_App_AccessPermitted()){
        			return_value = SVCCTL_EvtAckFlowEnable;
        			aci_gatt_allow_read(attribute_read->Connection_Handle);
        		} else {
        			aci_gatt_deny_read(attribute_read->Connection_Handle, UDS_ERROR_CODE_UserDataAccessNotPermitted);
        		}
        	} else if(attribute_read->Attribute_Handle == (UDS_Context.UDS_HeightCharHdle + 1)){
        		if(UDS_App_AccessPermitted()){
        			return_value = SVCCTL_EvtAckFlowEnable;
        			aci_gatt_allow_read(attribute_read->Connection_Handle);
        		} else {
        			aci_gatt_deny_read(attribute_read->Connection_Handle, UDS_ERROR_CODE_UserDataAccessNotPermitted);
        		}
        	}
        	break;

        case ACI_GATT_WRITE_PERMIT_REQ_VSEVT_CODE:{
        	aci_gatt_write_permit_req_event_rp0 * write_perm_req;

        	write_perm_req = (aci_gatt_write_permit_req_event_rp0*)blue_evt->data;
        	if(write_perm_req->Attribute_Handle == (UDS_Context.UDS_WeightCharHdle + 1)){
				if(UDS_App_AccessPermitted()){
					return_value = SVCCTL_EvtAckFlowEnable;
					aci_gatt_write_resp(write_perm_req->Connection_Handle,
					                                        write_perm_req->Attribute_Handle,
					                                        0x00, /* write_status = 0 (no error))*/
					                                        0x00, /* err_code */
					                                        write_perm_req->Data_Length,
					                                        (uint8_t *)&(write_perm_req->Data[0]));
				} else {
					aci_gatt_write_resp(write_perm_req->Connection_Handle,
					                                        write_perm_req->Attribute_Handle,
					                                        0x01, /* write_status = 1 (error))*/
															UDS_ERROR_CODE_UserDataAccessNotPermitted, /* err_code */
					                                        write_perm_req->Data_Length,
					                                        (uint8_t *)&(write_perm_req->Data[0]));
				}
			} else if(write_perm_req->Attribute_Handle == (UDS_Context.UDS_HeightCharHdle + 1)){
				if(UDS_App_AccessPermitted()){
					return_value = SVCCTL_EvtAckFlowEnable;
					aci_gatt_write_resp(write_perm_req->Connection_Handle,
					                                        write_perm_req->Attribute_Handle,
					                                        0x00, /* write_status = 0 (no error))*/
					                                        0x00, /* err_code */
					                                        write_perm_req->Data_Length,
					                                        (uint8_t *)&(write_perm_req->Data[0]));
				} else {
					aci_gatt_write_resp(write_perm_req->Connection_Handle,
					                                        write_perm_req->Attribute_Handle,
					                                        0x01, /* write_status = 1 (error))*/
															UDS_ERROR_CODE_UserDataAccessNotPermitted, /* err_code */
					                                        write_perm_req->Data_Length,
					                                        (uint8_t *)&(write_perm_req->Data[0]));
				}
			} else if(write_perm_req->Attribute_Handle == (UDS_Context.DatabaseChangeIncrementCharHdle + 1)){
				if(UDS_App_AccessPermitted()){
					return_value = SVCCTL_EvtAckFlowEnable;
					aci_gatt_write_resp(write_perm_req->Connection_Handle,
					                                        write_perm_req->Attribute_Handle,
					                                        0x00, /* write_status = 0 (no error))*/
					                                        0x00, /* err_code */
					                                        write_perm_req->Data_Length,
					                                        (uint8_t *)&(write_perm_req->Data[0]));
				} else {
					aci_gatt_write_resp(write_perm_req->Connection_Handle,
					                                        write_perm_req->Attribute_Handle,
					                                        0x01, /* write_status = 1 (error))*/
															UDS_ERROR_CODE_UserDataAccessNotPermitted, /* err_code */
					                                        write_perm_req->Data_Length,
					                                        (uint8_t *)&(write_perm_req->Data[0]));
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
}/* end UDS_Event_Handler */


static void Populate_Char_User_Index(uint8_t user_index)
{
  uint8_t value;

  value = user_index;

  aci_gatt_update_char_value(UDS_Context.SvcHdle,
                             UDS_Context.UserIndexCharHdle,
                             0, /* charValOffset */
                             1, /* charValueLen */
                             &value);
}/* end Update_Char_Feature() */


static void Populate_Char_Height(uint16_t height)
{
  uint8_t value[2];

  STORE_LE_16(value, height);

  aci_gatt_update_char_value(UDS_Context.SvcHdle,
                             UDS_Context.UDS_HeightCharHdle,
                             0, /* charValOffset */
                             2, /* charValueLen */
                             value);
}/* end Update_Char_Feature() */


static void Populate_Char_Weight(uint16_t weight)
{
  uint8_t value[2];

  STORE_LE_16(value, weight);

  aci_gatt_update_char_value(UDS_Context.SvcHdle,
                             UDS_Context.UDS_WeightCharHdle,
                             0, /* charValOffset */
                             2, /* charValueLen */
                             value);
}/* end Update_Char_Feature() */


static void UDS_UserControlPoint_Message(UDC_ProcedureComplete_t *msg){
	uint8_t ucp_value[UDS_PROCEDURE_COMPLETE_MAX_PARAMETER + 3];

	if(msg->ResponseCodeOpCode != UDS_UCP_OPCODE_RESPONSE_CODE){
		BLE_DBG_UDS_MSG("Wrong UDS Procedure Complete Response Code %d", msg->ResponseCodeOpCode);
		return;
	}

	if(msg->ResponseParameterLength > UDS_PROCEDURE_COMPLETE_MAX_PARAMETER){
		BLE_DBG_UDS_MSG("Wrong UDS Procedure Complete Parameter Length %d", msg->ResponseParameterLength);
		return;
	}

	ucp_value[0] = msg->ResponseCodeOpCode;
	ucp_value[1] = msg->RequestOpCode;
	ucp_value[2] = msg->ResponseValue;
	if(msg->ResponseParameterLength > 0){
		Osal_MemCpy( (void*)(ucp_value + 3), (const void*)(msg->ResponseParameter), msg->ResponseParameterLength );
	}

#if (BLE_DBG_UDS_EN != 0)
	BLE_DBG_UDS_MSG("User Control Message: ");
	for(int i=0; i<(msg->ResponseParameterLength + 3); i++){
		BLE_DBG_UDS_MSG("%02X ", ucp_value[i]);
	}
	BLE_DBG_UDS_MSG("\n\r");
#endif

	aci_gatt_update_char_value(UDS_Context.SvcHdle,
			UDS_Context.UserControlPointCharHdle,
			0, /* charValOffset */
			msg->ResponseParameterLength + 3 , /* charValueLen */
			(uint8_t *)  &ucp_value[0]);
}

