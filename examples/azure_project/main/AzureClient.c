/*
 * azure_core.c
 *
 *  Created on: Aug 2, 2020
 *      Author: mfawz
 */
#include "AzureClient.h"

#include "esp_log.h"

#define AZURE_CORE_TAG  "AzureCore"
#define EVENT_CONFIRMED -2
#define EVENT_FAILED -3

static IOTHUBMESSAGE_DISPOSITION_RESULT AzClient_tRxMessageCBk(IOTHUB_MESSAGE_HANDLE objMessageHandle,
        void *pvUserContextCBk);
static void AzClient_vConnectionStatusCBk(IOTHUB_CLIENT_CONNECTION_STATUS objConnectionStatus,
        IOTHUB_CLIENT_CONNECTION_STATUS_REASON objConnectionStatusReason, void *pvUserContextCBk);
static void AzClient_vReportConfirmationCBk(int32_t statusCode, void *pvUserContextCBk);
static void AzClient_vDeviceTwinCBk(DEVICE_TWIN_UPDATE_STATE updateState, const uint8_t *payLoad, size_t size,
        void *pvUserContextCBk);
static int32_t AzClient_s32DeviceMethodCBk(const char *methodName, const uint8_t *payload, size_t size,
        uint8_t **response, size_t *response_size, void *pvUserContextCBk);
static void AzClient_SendConfirmationCBk(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *pvUserContextCBk);
static void AzClient_vReportConfirmationCBk(int32_t statusCode, void *pvUserContextCBk);
static void AzClient_vFreeEventInstance(Azure_strEventInstanceType *objstrEventInstance);
static bool AzClient_bSendEventOnce(Azure_strEventInstanceType *pstrEventInstance);
static void AzClient_vCheckConnection();
static int32_t receiveContext = 0;
static bool bClientConnected = false;
static bool bResetClient = false;
static int32_t currentTrackingId = -1;
static int32_t callbackCounter;
static int32_t s32TrackingId = 0;
Azure_strConnectionInfoType *pstrConnectionInfoType = NULL;

static IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle = NULL;
static CONNECTION_STATUS_CALLBACK _connection_status_callback = NULL;
static SEND_CONFIRMATION_CALLBACK _send_confirmation_callback = NULL;
static MESSAGE_CALLBACK _message_callback = NULL;
static REPORT_CONFIRMATION_CALLBACK _report_confirmation_callback = NULL;
#define CONNECTION_TRIAL_MS_GAP             10
#define SENDING_EVENT_MS_GAP                10
#define SENDING_EVENTS_TIMEOUT_MS           10000
#define SEND_EVENT_RETRY_COUNT              5
#define CHECK_INTERVAL_MS                   5000

void AzClient_vSetConnectStatusCBk(CONNECTION_STATUS_CALLBACK connection_status_callback)
{
    _connection_status_callback = connection_status_callback;
}
void AzClient_vSetSendConfirmCBk(SEND_CONFIRMATION_CALLBACK send_confirmation_callback)
{
    _send_confirmation_callback = send_confirmation_callback;
}
void AzClient_vSetMessageCBk(MESSAGE_CALLBACK message_callback)
{
    _message_callback = message_callback;
}

bool AzClient_bSendEventInstance(Azure_strEventInstanceType *objstrEventInstance)
{
    if(objstrEventInstance == NULL)
    {
        return false;
    }

    return AzClient_bSendEventOnce(objstrEventInstance);
}
void AzClient_vSetReportConfirmCBk(REPORT_CONFIRMATION_CALLBACK report_confirmation_callback)
{
    _report_confirmation_callback = report_confirmation_callback;
}
bool AzClient_bInitialize(Azure_strConnectionInfoType *pstrNewConnectionInfoType)
{
    bool bInitStatus = true;
    IOTHUB_CLIENT_RESULT objClientResultType;
    int32_t s32ConenctionTrialCounts = (pstrNewConnectionInfoType->s32ConnectionTrialIntervalMS
            / CONNECTION_TRIAL_MS_GAP);

    if(iotHubClientHandle == NULL)
    {
        if(IoTHub_Init() != 0)
        {
            ESP_LOGE(AZURE_CORE_TAG, "Failed to initialize the platform.");
        }
        else
        {
            pstrConnectionInfoType = pstrNewConnectionInfoType;
            if((iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(
                    pstrNewConnectionInfoType->ps8DeviceConnectionString,
                    pstrNewConnectionInfoType->objProtocolTypeType)) == NULL)
            {
                ESP_LOGE(AZURE_CORE_TAG, "iotHubClientHandle is NULL!");
            }
            else
            {
                if(bInitStatus == true)
                {
                    objClientResultType = IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_LOG_TRACE,
                            &pstrNewConnectionInfoType->bEnabletracing);
                    if(objClientResultType != IOTHUB_CLIENT_OK)
                    {
                        ESP_LOGE(AZURE_CORE_TAG, "Failed to set option OPTION_LOG_TRACE");
                        bInitStatus = false;
                    }
                    else
                    {
                        /*
                         * Nothing to do
                         */
                    }
                }
                else
                {
                    /*
                     * Nothing to do
                     */
                }
                if(bInitStatus == true)
                {
                    objClientResultType = IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_KEEP_ALIVE,
                            &pstrNewConnectionInfoType->s32KeepaliveInSeconds);
                    if(objClientResultType != IOTHUB_CLIENT_OK)
                    {
                        ESP_LOGE(AZURE_CORE_TAG, "Failed to set option OPTION_KEEP_ALIVE");
                        bInitStatus = false;
                    }
                    else
                    {
                        /*
                         * Nothing to do
                         */
                    }
                }
                else
                {
                    /*
                     * Nothing to do
                     */
                }
#if 0
                if(bInitStatus == true)
                {
                    objClientResultType = IoTHubDeviceClient_LL_SetOption(iotHubClientHandle, "TrustedCerts", pstrNewConnectionInfoType->ps8certificates);
                    if(objClientResultType != IOTHUB_CLIENT_OK)
                    {
                        ESP_LOGE(AZURE_CORE_TAG, "failure to set option \"TrustedCerts\"\r\n");
                        bInitStatus = false;
                    }
                    else
                    {
                        /*
                         * Nothing to do
                         */
                    }
                }
                else
                {
                    /*
                     * Nothing to do
                     */
                }
#endif // SET_TRUSTED_CERT_IN_SAMPLES
                if(bInitStatus == true)
                {
                    objClientResultType = IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, AzClient_tRxMessageCBk,
                            &receiveContext);
                    if(objClientResultType != IOTHUB_CLIENT_OK)
                    {
                        ESP_LOGE(AZURE_CORE_TAG, "IoTHubClient_LL_SetMessageCallback is FAILED");
                        bInitStatus = false;
                    }
                    else
                    {
                        /*
                         * Nothing to do
                         */
                    }
                }
                else
                {
                    /*
                     * Nothing to do
                     */
                }
                if(bInitStatus == true)
                {
                    objClientResultType = IoTHubClient_LL_SetConnectionStatusCallback(iotHubClientHandle,
                            AzClient_vConnectionStatusCBk, NULL);
                    if(objClientResultType != IOTHUB_CLIENT_OK)
                    {
                        ESP_LOGE(AZURE_CORE_TAG, "IoTHubClient_LL_SetConnectionStatusCallback is FAILED");
                        bInitStatus = false;
                    }
                    else
                    {
                        /*
                         * Nothing to do
                         */
                    }
                }
                else
                {
                    /*
                     * Nothing to do
                     */
                }
                if(pstrNewConnectionInfoType->bEnableDeviceTwin == true)
                {
                    if(bInitStatus == true)
                    {
                        objClientResultType = IoTHubClient_LL_SetDeviceTwinCallback(iotHubClientHandle,
                                AzClient_vDeviceTwinCBk, pstrNewConnectionInfoType->pvTwinUserContextCBk);
                        if(objClientResultType != IOTHUB_CLIENT_OK)
                        {
                            ESP_LOGE(AZURE_CORE_TAG, "IoTHubClient_LL_SetDeviceTwinCallback is FAILED");
                            bInitStatus = false;
                        }
                        else
                        {
                            /*
                             * Nothing to do
                             */
                        }
                    }
                    else
                    {
                        /*
                         * Nothing to do
                         */
                    }
                }
                else
                {
                    /*
                     * Nothing to do
                     */
                }
                if(pstrNewConnectionInfoType->bEnableDeviceMethod == true)
                {
                    if(bInitStatus == true)
                    {
                        objClientResultType = IoTHubClient_LL_SetDeviceMethodCallback(iotHubClientHandle,
                                AzClient_s32DeviceMethodCBk, NULL);
                        if(objClientResultType != IOTHUB_CLIENT_OK)
                        {
                            ESP_LOGE(AZURE_CORE_TAG, "IoTHubClient_LL_SetDeviceMethodCallback is FAILED");
                            bInitStatus = false;
                        }
                        else
                        {
                            /*
                             * Nothing to do
                             */
                        }
                    }
                    else
                    {
                        /*
                         * Nothing to do
                         */
                    }
                }
                else
                {
                    /*
                     * Nothing to do
                     */
                }
                if(bInitStatus == true)
                {
                    while(s32ConenctionTrialCounts != 0)
                    {
                        IoTHubDeviceClient_LL_DoWork(iotHubClientHandle);
                        if(bClientConnected == true)
                        {
                            break;
                        }
                        else
                        {
                            ThreadAPI_Sleep(CONNECTION_TRIAL_MS_GAP);
                            s32ConenctionTrialCounts--;
                        }
                    }
                }
                else
                {

                }

            }
        }
    }
    else
    {
        /*
         * Nothing to do
         */
    }
    return bInitStatus;
}
void AzClient_vClose(void)
{
    if(iotHubClientHandle != NULL)
    {
        IoTHubClient_LL_Destroy(iotHubClientHandle);
        iotHubClientHandle = NULL;
    }
}
void AzClient_vTick()
{
    if(iotHubClientHandle == NULL)
    {
        return;
    }
    AzClient_vCheckConnection();
    for(int32_t i = 0; i < 5; i++)
    {
        IoTHubClient_LL_DoWork(iotHubClientHandle);
        if(bResetClient)
        {
            // Disconnected
            break;
        }
    }
}
bool AzClient_bSendEvent(const char *text)
{
    if(text == NULL)
    {
        return false;
    }
    for(int32_t i = 0; i < SEND_EVENT_RETRY_COUNT; i++)
    {
        if(AzClient_bSendEventOnce(AzClient_tBuildEvent(text, MESSAGE)))
        {
            return true;
        }
    }
    return false;
}

bool AzClient_bReportState(const char *stateString)
{
    if(stateString == NULL)
    {
        return false;
    }
    for(int32_t i = 0; i < SEND_EVENT_RETRY_COUNT; i++)
    {
        if(AzClient_bSendEventOnce(AzClient_tBuildEvent(stateString, STATE)))
        {
            return true;
        }
    }
    return false;
}
Azure_strEventInstanceType* AzClient_tBuildEvent(const char *eventString, Azure_enumEventTypeType objenumEventType)
{
    if(eventString == NULL)
    {
        return NULL;
    }

    Azure_strEventInstanceType *objstrEventInstance = (Azure_strEventInstanceType*) malloc(
            sizeof(Azure_strEventInstanceType));
    objstrEventInstance->objenumEventType = objenumEventType;

    if(objenumEventType == MESSAGE)
    {
        objstrEventInstance->objMessageHandle = IoTHubMessage_CreateFromByteArray((const uint8_t*) eventString,
                strlen(eventString));
        if(objstrEventInstance->objMessageHandle == NULL)
        {
            ESP_LOGE(AZURE_CORE_TAG, "iotHubMessageHandle is NULL!");
            free(objstrEventInstance);
            return NULL;
        }
    }
    else if(objenumEventType == STATE)
    {
        objstrEventInstance->ps8StateString = eventString;
    }

    return objstrEventInstance;
}
/////////////////////////////////////////////////////////////////////////////////////////////
static void AzClient_vConnectionStatusCBk(IOTHUB_CLIENT_CONNECTION_STATUS objConnectionStatus,
        IOTHUB_CLIENT_CONNECTION_STATUS_REASON objConnectionStatusReason, void *pvUserContextCBk)
{
    bClientConnected = false;

    switch(objConnectionStatusReason)
    {
        case IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE:
            break;
        case IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN:
            if(objConnectionStatus == IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED)
            {
                bResetClient = true;
                ESP_LOGI(AZURE_CORE_TAG, ">>>Connection status: timeout");
            }
            break;
        case IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED:
            break;
        case IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL:
            break;
        case IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED:
            break;
        case IOTHUB_CLIENT_CONNECTION_NO_NETWORK:
            if(objConnectionStatus == IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED)
            {
                bResetClient = true;
                ESP_LOGI(AZURE_CORE_TAG, ">>>Connection status: disconnected");
            }
            break;
        case IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR:
            break;
        case IOTHUB_CLIENT_CONNECTION_OK:
            if(objConnectionStatus == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED)
            {
                bClientConnected = true;
                ESP_LOGI(AZURE_CORE_TAG, ">>>Connection status: connected");
            }
            break;
    }

    if(_connection_status_callback)
    {
        _connection_status_callback(objConnectionStatus, objConnectionStatusReason);
    }
}
static void AzClient_SendConfirmationCBk(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *pvUserContextCBk)
{
    Azure_strEventInstanceType *objstrEventInstanceInstance = (Azure_strEventInstanceType*) pvUserContextCBk;
    ESP_LOGI(AZURE_CORE_TAG, ">>>Confirmation[%d] received for message tracking id = %d with result = %d",
            callbackCounter++, objstrEventInstanceInstance->s32TrackingId, result);

    if(currentTrackingId == objstrEventInstanceInstance->s32TrackingId)
    {
        if(result == IOTHUB_CLIENT_CONFIRMATION_OK)
        {
            currentTrackingId = EVENT_CONFIRMED;
        }
        else
        {
            currentTrackingId = EVENT_FAILED;
        }
    }

    // Free the message
    AzClient_vFreeEventInstance(objstrEventInstanceInstance);

    if(_send_confirmation_callback)
    {
        _send_confirmation_callback(result);
    }
}
static void AzClient_vFreeEventInstance(Azure_strEventInstanceType *objstrEventInstance)
{
    if(objstrEventInstance != NULL)
    {
        if(objstrEventInstance->objenumEventType == MESSAGE)
        {
            IoTHubMessage_Destroy(objstrEventInstance->objMessageHandle);
        }
        else
        {
            /*
             * Nothing to do
             */
        }
        free(objstrEventInstance);
    }
    else
    {
        /*
         * Nothing to do
         */
    }
}

static IOTHUBMESSAGE_DISPOSITION_RESULT AzClient_tRxMessageCBk(IOTHUB_MESSAGE_HANDLE objMessageHandle,
        void *pvUserContextCBk)
{
    int32_t *counter = (int32_t*) pvUserContextCBk;
    const char *buffer;
    size_t size;

    // Message content
    if(IoTHubMessage_GetByteArray(objMessageHandle, (const uint8_t**) &buffer, &size) != IOTHUB_MESSAGE_OK)
    {
        ESP_LOGE(AZURE_CORE_TAG, "unable to retrieve the message data");
        return IOTHUBMESSAGE_REJECTED;
    }
    else
    {
        char *temp = (char*) malloc(size + 1);
        if(temp == NULL)
        {
            ESP_LOGE(AZURE_CORE_TAG, "Failed to malloc for command");
            return IOTHUBMESSAGE_REJECTED;
        }
        memcpy(temp, buffer, size);
        temp[size] = '\0';
        ESP_LOGI(AZURE_CORE_TAG, ">>>Received Message [%d], Size=%d Message %s", *counter, (int32_t )size, temp);
        if(_message_callback)
        {
            _message_callback(temp, size);
        }
        free(temp);
    }

    /* Some device specific action code goes here... */
    (*counter)++;
    return IOTHUBMESSAGE_ACCEPTED;
}

static void AzClient_vDeviceTwinCBk(DEVICE_TWIN_UPDATE_STATE updateState, const uint8_t *payLoad, size_t size,
        void *pvUserContextCBk)
{
    if(pstrConnectionInfoType != NULL)
    {
        if(pstrConnectionInfoType->pFuncDeviceTwinCallBack)
        {
            pstrConnectionInfoType->pFuncDeviceTwinCallBack(updateState, payLoad, size,pvUserContextCBk);
        }
    }
}

static int32_t AzClient_s32DeviceMethodCBk(const char *methodName, const uint8_t *payload, size_t size,
        uint8_t **response, size_t *response_size, void *pvUserContextCBk)
{
    if(pstrConnectionInfoType != NULL)
    {
        if(pstrConnectionInfoType->pFuncDeviceMethodCallBack)
        {
            return pstrConnectionInfoType->pFuncDeviceMethodCallBack(methodName, payload, size, response,
                    (int32_t*) response_size, pvUserContextCBk);
        }
    }
    const char *responseMessage = "\"No method found\"";
    *response_size = strlen(responseMessage);
    *response = (uint8_t*) strdup("\"No method found\"");

    return 404;
}

static void AzClient_vReportConfirmationCBk(int32_t statusCode, void *pvUserContextCBk)
{
    Azure_strEventInstanceType *objstrEventInstance = (Azure_strEventInstanceType*) pvUserContextCBk;
    ESP_LOGI(AZURE_CORE_TAG, ">>>Confirmation[%d] received for state tracking id = %d with state code = %d",
            callbackCounter++, objstrEventInstance->s32TrackingId, statusCode);

    if(statusCode == 204)
    {
        if(currentTrackingId == objstrEventInstance->s32TrackingId)
        {
            currentTrackingId = EVENT_CONFIRMED;
        }
    }
    else
    {
        ESP_LOGE(AZURE_CORE_TAG, "Report confirmation failed with state code %d", statusCode);
    }

    // Free the state
    AzClient_vFreeEventInstance(objstrEventInstance);

    if(_report_confirmation_callback)
    {
        _report_confirmation_callback(statusCode);
    }
}
static void AzClient_vCheckConnection()
{
    if(bResetClient)
    {
        ESP_LOGI(AZURE_CORE_TAG, ">>>Re-connect.");
        // Re-connect the IoT Hub
        AzClient_vClose();
        AzClient_bInitialize(pstrConnectionInfoType);
        bResetClient = false;
    }
}
static bool AzClient_bSendEventOnce(Azure_strEventInstanceType *pstrEventInstance)
{
    int32_t s32TrialCounts = (SENDING_EVENTS_TIMEOUT_MS / SENDING_EVENT_MS_GAP);
    if(pstrEventInstance == NULL)
    {
        return false;
    }

    if(iotHubClientHandle == NULL)
    {
        AzClient_vFreeEventInstance(pstrEventInstance);
        return false;
    }
    pstrEventInstance->s32TrackingId = s32TrackingId++;
    currentTrackingId = pstrEventInstance->s32TrackingId;
    AzClient_vCheckConnection();
    if(pstrEventInstance->objenumEventType == MESSAGE)
    {
        if(IoTHubClient_LL_SendEventAsync(iotHubClientHandle, pstrEventInstance->objMessageHandle,
                AzClient_SendConfirmationCBk, pstrEventInstance) != IOTHUB_CLIENT_OK)
        {
            ESP_LOGE(AZURE_CORE_TAG, "IoTHubClient_LL_SendEventAsync..........FAILED!");
            AzClient_vFreeEventInstance(pstrEventInstance);
            return false;
        }
        ESP_LOGI(AZURE_CORE_TAG, ">>>IoTHubClient_LL_SendEventAsync accepted message for transmission to IoT Hub.");
    }
    else if(pstrEventInstance->objenumEventType == STATE)
    {
        if(IoTHubClient_LL_SendReportedState(iotHubClientHandle, (const uint8_t*) pstrEventInstance->ps8StateString,
                strlen(pstrEventInstance->ps8StateString), AzClient_vReportConfirmationCBk, pstrEventInstance)
                != IOTHUB_CLIENT_OK)
        {
            ESP_LOGE(AZURE_CORE_TAG, "IoTHubClient_LL_SendReportedState..........FAILED!");
            AzClient_vFreeEventInstance(pstrEventInstance);
            return false;
        }
        ESP_LOGI(AZURE_CORE_TAG, ">>>IoTHubClient_LL_SendReportedState accepted state for transmission to IoT Hub.");
    }

    while(s32TrialCounts)
    {
        IoTHubClient_LL_DoWork(iotHubClientHandle);

        if(currentTrackingId == EVENT_CONFIRMED)
        {
            // IoT Hub got this pstrEventInstance
            return true;
        }
        if(bResetClient)
        {
            // bResetClient also can be set as true in the IoTHubClient_LL_DoWork
            // Disconnected, re-send the message
            break;
        }
        else
        {
            // Sleep a while
            ThreadAPI_Sleep(SENDING_EVENT_MS_GAP);
        }
        s32TrialCounts--;
        if(s32TrialCounts == 0)
        {
            // Time out, reset the client
            ESP_LOGE(AZURE_CORE_TAG, "Waiting for send confirmation, time is up %dMS", SENDING_EVENTS_TIMEOUT_MS);
            bResetClient = true;
        }

    }

    return false;
}
