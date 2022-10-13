/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    uds_app.c
  * @author  MCD Application Team
  * @brief   User Data Service Application
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
#include "uds.h"
#include "uds_app.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private defines ------------------------------------------------------------*/
#define MAX_SIZE_USER_DATA             0x10  /* 0xFE */ /* decrease database size for limited RAM size on STM32WB15CC */
#define MAX_CONSENT_CODE               0x270F /*9999*/

#define INTERVAL_REGISTER_NEW_USER     (1000000/CFG_TS_TICK_VAL)  /**< 1s */
#define INTERVAL_CONSENT               INTERVAL_REGISTER_NEW_USER
#define INTERVAL_DELETE_USER_DATA      INTERVAL_REGISTER_NEW_USER

#define MAXIMUM_CONSENT_TRIES          3


/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private typedef -----------------------------------------------------------*/
typedef struct {
	uint8_t user_index;
	uint16_t consent_code;
	uint16_t height;
	uint16_t weight;
} UDSAPP_UserData_t;

typedef struct {
	uint32_t tick;
	uint16_t consent_code;
} UCP_Buffer_RegisterNewUser_t;

typedef struct {
	uint32_t tick;
	uint8_t user_index;
	uint16_t consent_code;
	uint8_t tries;
} UCP_Buffer_Consent_t;

typedef struct{
  UDSAPP_UserData_t user_data[MAX_SIZE_USER_DATA];
  uint8_t user_data_size;

  /* buffer for User Control Point */
  UCP_Buffer_RegisterNewUser_t buf_register_new_user;
  UCP_Buffer_Consent_t buf_consent;

  uint16_t UDS_Char_Height;
  uint16_t UDS_Char_Weight;

  uint8_t user_data_access_permitted;

  UDC_ProcedureComplete_t UDC_ErrorMessage;

  uint16_t indicationStatusUerControlPoint;
  uint32_t StartTick;
} UDSAPP_Context_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private macros -------------------------------------------------------------*/

/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
static UDSAPP_Context_t UDSAPP_Context;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void UDSAPP_UserControlPoint_Error_Message(void);
static void UDS_App_Notif_Height(uint16_t height);
static void UDS_App_Notif_Weight(uint16_t weight);
static void UDS_App_Notif_UserControlPoint(UDS_App_Notification_UCP_t *data);

static void procedure_complete_error(uint8_t request_op_code, uint8_t error_code);
static void register_new_user(void);
static void consent(void);
static void delete_user_data(void);
static void mark_invalid_data(uint8_t index);


/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
static void UDSAPP_UserControlPoint_Error_Message(void){
	UDSAPP_Context.UDC_ErrorMessage.ResponseCodeOpCode = UDS_UCP_OPCODE_RESPONSE_CODE;
	UDSAPP_Context.UDC_ErrorMessage.ResponseParameterLength = 0;

	UDS_Update_Char(USER_CONTROL_POINT_CHAR_UUID, (uint8_t*)&(UDSAPP_Context.UDC_ErrorMessage));
}

static void procedure_complete_error(uint8_t request_op_code, uint8_t error_code){
	UDSAPP_Context.UDC_ErrorMessage.ResponseValue = error_code;
	UDSAPP_Context.UDC_ErrorMessage.RequestOpCode = request_op_code;
	UTIL_SEQ_SetTask( 1<<CFG_TASK_UDS_CRL_MSG_ID, CFG_SCH_PRIO_0);
}

static void register_new_user(void){
	uint16_t consent_code = UDSAPP_Context.buf_register_new_user.consent_code;
	uint8_t index;
	UDC_ProcedureComplete_t response;

	APP_DBG_MSG("Register New User procedure [tick = %ld]", UDSAPP_Context.buf_register_new_user.tick);

	response.ResponseCodeOpCode = UDS_UCP_OPCODE_RESPONSE_CODE;
	response.RequestOpCode      = UDS_UCP_OPCODE_REGISTER_NEW_USER;

	if (consent_code > MAX_CONSENT_CODE) {
		procedure_complete_error(response.RequestOpCode, UDS_RESPONSE_VALUE_INVALID_PARAMETER);

		return;
	}

#ifdef SUPPORT_MULTI_USERS
	index = UDSAPP_Context.user_data_size + 1;
#else
	/* only one user */
	index = 1;
#endif /* SUPPORT_MULTI_USERS */

	if ((UDSAPP_Context.user_data[index].user_index != UDS_USER_INDEX_UNKNOW) &&
			(UDSAPP_Context.user_data[index].consent_code <= MAX_CONSENT_CODE)){
		/* already registered by other user -- only single user is allowed */
		procedure_complete_error(response.RequestOpCode, UDS_RESPONSE_VALUE_OPERATION_FAILED);

		return;
	}

	if (index >= 0xFF){
		/* arrive the range of User Index */
		procedure_complete_error(response.RequestOpCode, UDS_RESPONSE_VALUE_OPERATION_FAILED);

		return;
	}

	/* procedure */
	UDSAPP_Context.user_data_size = index;
	UDSAPP_Context.user_data[index].user_index = index;
	UDSAPP_Context.user_data[index].consent_code = consent_code;
	UDSAPP_Context.user_data[index].height = 0;  /* invalid value */
	UDSAPP_Context.user_data[index].weight = 0;  /* invalid value */

	/* reset the counter for consent tries */
	UDSAPP_Context.buf_consent.tries = 0;

	UDSAPP_Context.user_data_access_permitted = 0; /* disable */

	/* procedure complete message */
	response.ResponseValue = UDS_RESPONSE_VALUE_SUCCESS;
	response.ResponseParameter[0] = index;
	response.ResponseParameterLength = 1;

	UDS_Update_Char(USER_CONTROL_POINT_CHAR_UUID, (uint8_t*)&response);

#ifdef SUPPORT_MULTI_USERS
	/* prepare the next record */
	if(index < 0xFF){
		mark_invalid_data(index + 1);
	}
#endif /* SUPPORT_MULTI_USERS */
}

static void consent(void){
	uint16_t consent_code = UDSAPP_Context.buf_consent.consent_code;
	uint8_t user_index = UDSAPP_Context.buf_consent.user_index; /* User Index starts from 1 */
	UDC_ProcedureComplete_t response;
	uint16_t value;

	APP_DBG_MSG("Consent procedure [tick = %ld]", UDSAPP_Context.buf_consent.tick);

	response.ResponseCodeOpCode = UDS_UCP_OPCODE_RESPONSE_CODE;
	response.RequestOpCode      = UDS_UCP_OPCODE_CONSENT;

	if (consent_code > MAX_CONSENT_CODE) {
		procedure_complete_error(response.RequestOpCode, UDS_RESPONSE_VALUE_INVALID_PARAMETER);

		return;
	}

	if ((user_index == 0) || (user_index > UDSAPP_Context.user_data_size) || (user_index >= 0xFF)){
		procedure_complete_error(response.RequestOpCode, UDS_RESPONSE_VALUE_INVALID_PARAMETER);

		return;
	}

	if (UDSAPP_Context.user_data[user_index].consent_code != consent_code){
		if(UDSAPP_Context.buf_consent.tries < MAXIMUM_CONSENT_TRIES){
			procedure_complete_error(response.RequestOpCode, UDS_RESPONSE_VALUE_USER_NOT_AUTHORIZED);
		} else {
			procedure_complete_error(response.RequestOpCode, UDS_RESPONSE_VALUE_OPERATION_FAILED);
		}

		UDSAPP_Context.buf_consent.tries += 1;

		return;
	}

	/* procedure complete message */
	response.ResponseValue = UDS_RESPONSE_VALUE_SUCCESS;
	response.ResponseParameterLength = 0;

	UDS_Update_Char(USER_CONTROL_POINT_CHAR_UUID, (uint8_t*)&response);

	/* populate User Index */
	UDS_Update_Char(USER_INDEX_CHAR_UUID, (uint8_t*)&user_index);

	/* populate current User Data */
	if(UDSAPP_Context.user_data[user_index].height > 0){
		value = UDSAPP_Context.user_data[user_index].height;
		UDS_Update_Char(HEIGHT_CHAR_UUID, (uint8_t*)(&value));
	}
	if(UDSAPP_Context.user_data[user_index].weight > 0){
		value = UDSAPP_Context.user_data[user_index].weight;
		UDS_Update_Char(WEIGHT_CHAR_UUID, (uint8_t*)(&value));
	}

	UDSAPP_Context.user_data_access_permitted = 1; /* enable */
}

static void delete_user_data(void){
	uint8_t index;
	UDC_ProcedureComplete_t response;

	APP_DBG_MSG("Delete User Data procedure [tick = %ld]", UDSAPP_Context.buf_consent.tick);

	response.ResponseCodeOpCode = UDS_UCP_OPCODE_RESPONSE_CODE;
	response.RequestOpCode      = UDS_UCP_OPCODE_DELETE_USER_DATA;

	index = UDSAPP_Context.user_data_size;

	if ( (UDSAPP_Context.user_data_size == 0) || (index < 1) || (UDSAPP_Context.user_data[index].user_index == UDS_USER_INDEX_UNKNOW)){
		procedure_complete_error(response.RequestOpCode, UDS_RESPONSE_VALUE_USER_NOT_AUTHORIZED);

		return;
	}

	if (index >= 0xFF){
		/* arrive the range of User Index */
		procedure_complete_error(response.RequestOpCode, UDS_RESPONSE_VALUE_OPERATION_FAILED);

		return;
	}

	/* mark current data as invalid */
	mark_invalid_data(index);

	UDSAPP_Context.user_data_size -= 1;

	UDSAPP_Context.user_data_access_permitted = 0; /* disable */

	/* procedure complete message */
	response.ResponseValue = UDS_RESPONSE_VALUE_SUCCESS;
	response.ResponseParameterLength = 0;

	UDS_Update_Char(USER_CONTROL_POINT_CHAR_UUID, (uint8_t*)&response);
}

static void mark_invalid_data(uint8_t index){
	/* mark current data as invalid */
	UDSAPP_Context.user_data[index].user_index = UDS_USER_INDEX_UNKNOW;
	UDSAPP_Context.user_data[index].consent_code = (MAX_CONSENT_CODE + 1);
	UDSAPP_Context.user_data[index].height = 0;
	UDSAPP_Context.user_data[index].weight = 0;
}

static void UDS_App_Notif_Height(uint16_t height){
	uint8_t index;

	if ((UDSAPP_Context.user_data_size == 0) || (UDSAPP_Context.user_data_size >= 0xFF) || (UDSAPP_Context.user_data[UDSAPP_Context.user_data_size].user_index == UDS_USER_INDEX_UNKNOW)){
		/* invalid current user index */
		return;
	}

	index = UDSAPP_Context.user_data_size;

	UDSAPP_Context.user_data[index].height = height;
}

static void UDS_App_Notif_Weight(uint16_t weight){
	uint8_t index;

	if ((UDSAPP_Context.user_data_size == 0) || (UDSAPP_Context.user_data_size >= 0xFF) || (UDSAPP_Context.user_data[UDSAPP_Context.user_data_size].user_index == UDS_USER_INDEX_UNKNOW)){
		/* invalid current user index */
		return;
	}

	index = UDSAPP_Context.user_data_size;

	UDSAPP_Context.user_data[index].height = weight;
}

static void UDS_App_Notif_UserControlPoint(UDS_App_Notification_UCP_t *data)
{
  uint32_t tick = HAL_GetTick();
  APP_DBG_MSG("[enabled = %d] UDS_App_Notif_UserControlPoint, code = %d (tick = %ld)\n\r",
		  UDSAPP_Context.indicationStatusUerControlPoint, data->op_code, tick);

  if(UDSAPP_Context.indicationStatusUerControlPoint == 0){
	  return;
  }
  
  switch(data->op_code)
  {
  case 0:
	  /* Reserved for future use */
	  break;
  case UDS_UCP_OPCODE_REGISTER_NEW_USER:
	  /* Register New User */
	  UDSAPP_Context.buf_register_new_user.tick = tick;
	  UDSAPP_Context.buf_register_new_user.consent_code = *((uint16_t*)data->parameter);
	  UTIL_SEQ_SetTask( 1<<CFG_TASK_UDS_REG_NEW_ID, CFG_SCH_PRIO_0);

	  break;
  case UDS_UCP_OPCODE_CONSENT:
	  /* Consent */
	  UDSAPP_Context.buf_consent.tick = tick;
	  UDSAPP_Context.buf_consent.user_index = data->parameter[0];
	  UDSAPP_Context.buf_consent.consent_code = *((uint16_t*)(data->parameter + 1));

	  UTIL_SEQ_SetTask( 1<<CFG_TASK_UDS_CONSENT_ID, CFG_SCH_PRIO_0);
	  break;
  case UDS_UCP_OPCODE_DELETE_USER_DATA:
	  /* Delete User Data */
	  UTIL_SEQ_SetTask( 1<<CFG_TASK_UDS_DEL_USER_ID, CFG_SCH_PRIO_0);
	  break;
  case UDS_UCP_OPCODE_LIST_ALL_USERS:
	  /* List All Users */
	  procedure_complete_error(UDS_UCP_OPCODE_LIST_ALL_USERS, UDS_RESPONSE_VALUE_OP_CODE_NOT_SUPPORTED);
	  break;
  case UDS_UCP_OPCODE_DELETE_USER:
	  /* Delete User(s) */
	  procedure_complete_error(UDS_UCP_OPCODE_DELETE_USER, UDS_RESPONSE_VALUE_OP_CODE_NOT_SUPPORTED);
	  break;
  default:
	  /* 0x06−0x1F, Reserved for future use */
	  /* 0x20, Response Code */
	  /* 0x21−0xFF, Reserved for future use */
	  //UDS_RESPONSE_VALUE_OP_CODE_NOT_SUPPORTED
	  break;
  }

  return;
}

/* Public functions ----------------------------------------------------------*/
uint8_t UDS_App_AccessPermitted(void){
	return UDSAPP_Context.user_data_access_permitted != 0;
}

void UDS_App_Notification(UDS_App_Notification_evt_t *pNotification){
	switch(pNotification->UDS_Evt_Opcode){
	case UDS_INDICATION_ENABLED:
		UDSAPP_Context.indicationStatusUerControlPoint = 1;
		break;
	case UDS_INDICATION_DISABLED:
		UDSAPP_Context.indicationStatusUerControlPoint = 0;
		break;
	case UDS_USER_CONTROL_POINT_EVT:{
		UDS_App_Notification_UCP_t noti;
		noti.op_code = pNotification->DataTransfered.pPayload[0];
		noti.parameter_length = pNotification->DataTransfered.Length - 1;
		if(noti.parameter_length > 0){
			Osal_MemCpy(noti.parameter, pNotification->DataTransfered.pPayload + 1, noti.parameter_length);
		}

		UDS_App_Notif_UserControlPoint(&noti);
	}
		break;
	case UDS_NOTIFY_HEIGHT:
		UDS_App_Notif_Height(*(uint16_t*)(pNotification->DataTransfered.pPayload));
		break;
	case UDS_NOTIFY_WEIGHT:
		UDS_App_Notif_Weight(*(uint16_t*)(pNotification->DataTransfered.pPayload));
		break;
	default:
		/* do nothing */
		break;
	}
}

void UDSAPP_Reset(void){
	APP_DBG_MSG("UDSAPP_Reset\n\r");

#ifndef UDS_SINGLE_TRUSTED_COLLECTOR
	/*
	* Reset Application Context
	*/
	UDSAPP_Context.indicationStatusUerControlPoint = 0;  /* disable */
	UDSAPP_Context.user_data_access_permitted = 0; /* disable */
	UDSAPP_Context.UDS_Char_Height = 0;
	UDSAPP_Context.UDS_Char_Weight = 0;
	UDSAPP_Context.user_data_size = 0;
	UDSAPP_Context.StartTick = HAL_GetTick();
#endif /* ! UDS_SINGLE_TRUSTED_COLLECTOR */
}

void UDSAPP_Init(void)
{
  APP_DBG_MSG("UDSAPP_Init\n\r");
  
  /*
   * Initialize Application Context
   */
  UDSAPP_Context.indicationStatusUerControlPoint = 0;  /* disable */
  UDSAPP_Context.user_data_access_permitted = 0; /* disable */
  UDSAPP_Context.UDS_Char_Height = 0;
  UDSAPP_Context.UDS_Char_Weight = 0;
  UDSAPP_Context.user_data_size = 0;
  UDSAPP_Context.StartTick = HAL_GetTick();
  
  mark_invalid_data(1);

  /**
   * Initialize User Data Characteristics
   */

  /*
   * Register task for User Control Point
   */
  UTIL_SEQ_RegTask( 1<< CFG_TASK_UDS_CRL_MSG_ID, UTIL_SEQ_RFU, UDSAPP_UserControlPoint_Error_Message );
  UTIL_SEQ_RegTask( 1<< CFG_TASK_UDS_REG_NEW_ID, UTIL_SEQ_RFU, register_new_user );
  UTIL_SEQ_RegTask( 1<< CFG_TASK_UDS_CONSENT_ID, UTIL_SEQ_RFU, consent );
  UTIL_SEQ_RegTask( 1<< CFG_TASK_UDS_DEL_USER_ID, UTIL_SEQ_RFU, delete_user_data );
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */
