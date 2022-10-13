/**
  ******************************************************************************
  * @file    uds.h
  * @author  GPM Application Team
  * @brief   Header for uds.c module
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
#ifndef __UDS_H
#define __UDS_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
#define UDS_USER_CONTROL_POINT_MAX_PARAMETER           8
#define UDS_PROCEDURE_COMPLETE_MAX_PARAMETER           18
#define UDS_APPLICATION_ERROR_CODE                     0x80
#define UDS_USER_INDEX_UNKNOW                          0xFF

typedef enum {
	UDS_ERROR_CODE_UserDataAccessNotPermitted = 0x80
} UDS_ErrorCode_t;

typedef struct{
	uint8_t op_code;
	uint8_t parameter_length;
	uint8_t parameter[UDS_USER_CONTROL_POINT_MAX_PARAMETER];
}UDS_App_Notification_UCP_t;

typedef enum {
	UDS_UCP_OPCODE_REGISTER_NEW_USER = 1,
	UDS_UCP_OPCODE_CONSENT = 2,
	UDS_UCP_OPCODE_DELETE_USER_DATA = 3,
	UDS_UCP_OPCODE_LIST_ALL_USERS = 4,
	UDS_UCP_OPCODE_DELETE_USER = 5,
	UDS_UCP_OPCODE_RESPONSE_CODE = 0x20,
} UDC_UserControlPoint_OpCode_t;

typedef struct {
	uint8_t ResponseCodeOpCode;
	uint8_t RequestOpCode;
	uint8_t ResponseValue;
	uint8_t ResponseParameter[UDS_PROCEDURE_COMPLETE_MAX_PARAMETER];
	uint8_t ResponseParameterLength;
} UDC_ProcedureComplete_t;

typedef enum
{
  UDS_USER_CONTROL_POINT_EVT=0,
  UDS_INDICATION_ENABLED,
  UDS_INDICATION_DISABLED,
  UDS_NOTIFY_HEIGHT,
  UDS_NOTIFY_WEIGHT,
} UDS_NotCode_t;

typedef struct
{
	uint8_t *pPayload;
	uint8_t Length;
}UDS_Data_t;

typedef struct
{
  UDS_NotCode_t             UDS_Evt_Opcode;
  UDS_Data_t                DataTransfered;
  uint16_t                  ConnectionHandle;
  uint8_t                   ServiceInstance;
}UDS_App_Notification_evt_t;


typedef enum {
	UDS_RESPONSE_VALUE_SUCCESS = 1,
	UDS_RESPONSE_VALUE_OP_CODE_NOT_SUPPORTED = 2,
	UDS_RESPONSE_VALUE_INVALID_PARAMETER = 3,
	UDS_RESPONSE_VALUE_OPERATION_FAILED = 4,
	UDS_RESPONSE_VALUE_USER_NOT_AUTHORIZED = 5
} UDS_ProcedureComplete_ResponseValue_t;

/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void UDS_Init(void);
void UDS_App_Notification(UDS_App_Notification_evt_t *pNotification);
void UDS_Update_Char(uint16_t UUID, uint8_t *pPayload);

uint8_t UDS_App_AccessPermitted(void);


#ifdef __cplusplus
}
#endif

#endif /*__UDS_H */


