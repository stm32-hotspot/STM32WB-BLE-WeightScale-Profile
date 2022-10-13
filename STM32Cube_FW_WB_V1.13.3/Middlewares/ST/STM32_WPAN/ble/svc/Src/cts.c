/**
 ******************************************************************************
 * @file    cts.c
 * @author  GPM Application Team
 * @brief   Current Time Service
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
#include "cts.h"

/* 16-bit UUID */
/*
 * || Allocation type                     | Allocated UUID | Allocated for         ||
 * |+-------------------------------------+----------------+-----------------------+|
 * || GATT Service                        | 0x1805         | Current Time          ||
 * || GATT Characteristic and Object Type | 0x2A2B         | Current Time          ||
 */

/* Private typedef -----------------------------------------------------------*/
typedef struct {
	uint16_t SvcHdle; /**< Service handle, Current Time Service */
	/* Mandatory Service Characteristics */
	uint16_t CurrentTimeCharHdle; /**< Service Characteristic handle, Current Time */

	/* Optional Service Characteristics */
	uint16_t LocalTimeInfoCharHdle; /**< Service Characteristic handle, Local Time Information */
	uint16_t ReferenceTimeInfoCharHdle; /**< Service Characteristic handle, Reference Time Information */
} CTS_Context_t;

/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Store Value into a buffer in Little Endian Format */
#define STORE_LE_16(buf, val)    ( ((buf)[0] =  (uint8_t) (val)    ) , \
                                   ((buf)[1] =  (uint8_t) (val>>8) ) )

/* Private variables ---------------------------------------------------------*/
static CTS_Context_t CTS_Context;

/* Private function prototypes -----------------------------------------------*/
static SVCCTL_EvtAckStatus_t CTS_Event_Handler(void *Event);
static void UpateCurrentTime(CTS_Ch_t *data);

/* Public functions ----------------------------------------------------------*/

/**
 * @brief  Service initialization
 * @param  None
 * @retval None
 */
void CTS_Init(void) {
	tBleStatus hciCmdResult = BLE_STATUS_FAILED;
	uint16_t uuid;

	/**
	 *  Register the event handler to the BLE controller
	 */
	SVCCTL_RegisterSvcHandler(CTS_Event_Handler);

	/**
	 *  Add Current Time Service
	 *
	 * Max_Attribute_Records = 2*no_of_char + 1
	 * service_max_attribute_record = 1 for Current Time service +
	 *                                2 for Current Time characteristic +
	 *                                   1 for client char configuration descriptor
	 *
	 */
	uuid = CURRENT_TIME_SERVICE_UUID;
	hciCmdResult = aci_gatt_add_service(UUID_TYPE_16,
			(Service_UUID_t*) &uuid,
			PRIMARY_SERVICE,
			4,
			&(CTS_Context.SvcHdle));

	if (hciCmdResult == BLE_STATUS_SUCCESS) {
		BLE_DBG_CTS_MSG ("Current Time Service is added Successfully %04X\n\r",
				CTS_Context.SvcHdle);
	} else {
		BLE_DBG_CTS_MSG ("FAILED to add Current Time Service: Error: %02X !!\n\r",
				hciCmdResult);
	}

	/**
	 *  Add Weight Scale Feature Characteristic
	 */
	uuid = CURRENT_TIME_CHAR_UUID;
	hciCmdResult = aci_gatt_add_char(CTS_Context.SvcHdle,
			UUID_TYPE_16,
			(Char_UUID_t*) &uuid,
			10, /** 10 octets */
			CHAR_PROP_READ | CHAR_PROP_NOTIFY,
			ATTR_PERMISSION_NONE,
			GATT_DONT_NOTIFY_EVENTS, /* gattEvtMask */
			10, /* encryKeySize */
			0, /* fixed-length */
			&(CTS_Context.CurrentTimeCharHdle));

	if (hciCmdResult == BLE_STATUS_SUCCESS) {
		BLE_DBG_CTS_MSG ("Current Time Characteristic Added Successfully %04X\n\r",
				CTS_Context.CurrentTimeCharHdle);
	} else {
		BLE_DBG_CTS_MSG ("FAILED to add Current Time Characteristic: Error: %02X !!\n\r",
				hciCmdResult);
	}
}

void CTS_Update_Char(uint16_t UUID, uint8_t *pPayload) {
	switch (UUID) {
	case CURRENT_TIME_CHAR_UUID:
		UpateCurrentTime((CTS_Ch_t*) pPayload);
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
static SVCCTL_EvtAckStatus_t CTS_Event_Handler(void *Event) {
	SVCCTL_EvtAckStatus_t return_value;
	hci_event_pckt *event_pckt;
	evt_blue_aci *blue_evt;
	aci_gatt_attribute_modified_event_rp0 *attribute_modified;
	CTS_App_Notification_evt_t Notification;

	return_value = SVCCTL_EvtNotAck;
	event_pckt = (hci_event_pckt*) (((hci_uart_pckt*) Event)->data);

	switch (event_pckt->evt) {
	case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE: {
		blue_evt = (evt_blue_aci*) event_pckt->data;
		switch (blue_evt->ecode) {
		case EVT_BLUE_GATT_ATTRIBUTE_MODIFIED: {
			attribute_modified = (aci_gatt_attribute_modified_event_rp0*) blue_evt->data;
			if (attribute_modified->Attr_Handle == (CTS_Context.CurrentTimeCharHdle + 2)) {
				BLE_DBG_CTS_MSG("CTS event -> EVT_BLUE_GATT_ATTRIBUTE_MODIFIED -> (CurrentTimeCharHdle + 2)\n\r");

				if (attribute_modified->Attr_Data[0] & COMSVC_Notification) {
					Notification.CTS_Evt_Opcode = CTS_NOTIFY_ENABLED_EVT;
				} else {
					Notification.CTS_Evt_Opcode = CTS_NOTIFY_DISABLED_EVT;
				}

				CTS_App_Notification(&Notification);

				return_value = SVCCTL_EvtAckFlowEnable;
			}
		}
			break;

		case ACI_GATT_WRITE_PERMIT_REQ_VSEVT_CODE: {
			aci_gatt_write_permit_req_event_rp0 *write_perm_req;
			write_perm_req = (aci_gatt_write_permit_req_event_rp0*) blue_evt->data;
			if (write_perm_req->Attribute_Handle == (CTS_Context.CurrentTimeCharHdle + 1)) {
				BLE_DBG_CTS_MSG("CTS event -> ACI_GATT_WRITE_PERMIT_REQ_VSEVT_CODE -> (CurrentTimeCharHdle + 1)\n\r");
				aci_gatt_write_resp(write_perm_req->Connection_Handle,
						write_perm_req->Attribute_Handle, 0x01, /* write_status = 1 (error))*/
						CTS_ERR_CODE_DATA_FIELD_IGNORED, /* err_code */
						write_perm_req->Data_Length,
						(uint8_t*) &(write_perm_req->Data[0]));
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

	return (return_value);
}/* end CTS_Event_Handler */

static void UpateCurrentTime(CTS_Ch_t *data) {
	uint8_t buf[10];
	uint8_t length = 0;
	CTS_DateTime_t *date = &(data->exact_time_256.day_date_time.date_time);

	/* Day Date time */
	STORE_LE_16(buf + length, date->Year);
	length += 2;
	buf[length++] = date->Month;
	buf[length++] = date->Day;
	buf[length++] = date->Hours;
	buf[length++] = date->Minutes;
	buf[length++] = date->Seconds;

	/* day of week */
	buf[length++] = data->exact_time_256.day_date_time.day_of_week;

	/* Fractions 256 */
	buf[length++] = data->exact_time_256.fractions256;

	/* Adjust Reason */
	buf[length++] = data->adjust_reason;

	aci_gatt_update_char_value(CTS_Context.SvcHdle,
			CTS_Context.CurrentTimeCharHdle, 0, /* charValOffset */
			length, /* charValLength */
			buf);
}
