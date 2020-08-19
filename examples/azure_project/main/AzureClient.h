/*
 * azure_core.h
 *
 *  Created on: Aug 2, 2020
 *      Author: mfawz
 */

#ifndef MAIN_AZURECLIENT_H_
#define MAIN_AZURECLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "azure_macro_utils/macro_utils.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/platform.h"
#include "iothub_client.h"
#include "iothub_device_client.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "iothub.h"
#include "iothub_message.h"
#include "parson.h"
#include "sdkconfig.h"

typedef void (*CONNECTION_STATUS_CALLBACK)(IOTHUB_CLIENT_CONNECTION_STATUS result,
        IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason);
typedef void (*SEND_CONFIRMATION_CALLBACK)(IOTHUB_CLIENT_CONFIRMATION_RESULT result);
typedef void (*MESSAGE_CALLBACK)(const char *message, int32_t length);
typedef void (*DEVICE_TWIN_CALLBACK)(DEVICE_TWIN_UPDATE_STATE updateState, const uint8_t *payLoad, int32_t length,
        void *userContextCallback);
typedef int32_t (*DEVICE_METHOD_CALLBACK)(const char *methodName, const uint8_t *payload, int32_t length,
        uint8_t **response, int32_t *responseLength, void *userContextCallback);
typedef void (*REPORT_CONFIRMATION_CALLBACK)(int32_t status_code);
typedef enum
{
    MESSAGE, STATE
} Azure_enumEventTypeType;

typedef struct
{
    Azure_enumEventTypeType objenumEventType;
    IOTHUB_MESSAGE_HANDLE objMessageHandle;
    const char *ps8StateString;
    int32_t s32TrackingId; // For tracking the events within the user callback.
} Azure_strEventInstanceType;
typedef struct
{
    const char *ps8DeviceConnectionString;
    int32_t s32KeepaliveInSeconds;
    int32_t s32ConnectionTrialIntervalMS;
    bool bEnableDeviceTwin;
    bool bEnableDeviceMethod;
    bool bEnabletracing; //enable verbose logging (e.g., for debugging).
    const char *ps8certificates;
    IOTHUB_CLIENT_TRANSPORT_PROVIDER objProtocolTypeType;
    DEVICE_METHOD_CALLBACK pFuncDeviceMethodCallBack;
    DEVICE_TWIN_CALLBACK pFuncDeviceTwinCallBack;
    void *pvTwinUserContextCBk;
} Azure_strConnectionInfoType;

bool AzClient_bInitialize(Azure_strConnectionInfoType *pstrConnectionInfoType);
bool AzClient_bReportState(const char *stateString);
bool AzClient_bSendEvent(const char *text);
void AzClient_vSetDeviceTwinCBk(DEVICE_TWIN_CALLBACK device_twin_callback);
void AzClient_vSetMessageCBk(MESSAGE_CALLBACK message_callback);
void AzClient_vSetConnectStatusCBk(CONNECTION_STATUS_CALLBACK connection_status_callback);
void AzClient_vSetSendConfirmCBk(SEND_CONFIRMATION_CALLBACK send_confirmation_callback);
void AzClient_vSetDeviceMethodCBk(DEVICE_METHOD_CALLBACK device_method_callback);
bool AzClient_bSendEventInstance(Azure_strEventInstanceType *objstrEventInstance);
void AzClient_vTick(void);
void AzClient_vClose(void);
Azure_strEventInstanceType* AzClient_tBuildEvent(const char *eventString, Azure_enumEventTypeType objenumEventType);
#endif /* MAIN_AZURECLIENT_H_ */
