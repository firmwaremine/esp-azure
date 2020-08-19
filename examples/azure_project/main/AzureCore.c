// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This sample shows how to translate the Device Twin json received from Azure IoT Hub into meaningful data for your application.
// It uses the parson library, a very lightweight json parser.

// There is an analogous sample using the serializer - which is a library provided by this SDK to help parse json - in devicetwin_simplesample.
// Most applications should use this sample, not the serializer.

// WARNING: Check the return of all API calls when developing your solution. Return checks ommited for sample simplification.

#include <AzureClient.h>
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

// The protocol you wish to use should be uncommented
//

#define SAMPLE_MQTT
//#define SAMPLE_MQTT_OVER_WEBSOCKETS
//#define SAMPLE_AMQP
//#define SAMPLE_AMQP_OVER_WEBSOCKETS
//#define SAMPLE_HTTP

#ifdef SAMPLE_MQTT
#include "iothubtransportmqtt.h"
#endif // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
    #include "iothubtransportmqtt_websockets.h"
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS
#ifdef SAMPLE_AMQP
    #include "iothubtransportamqp.h"
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
    #include "iothubtransportamqp_websockets.h"
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
    #include "iothubtransporthttp.h"
#endif // SAMPLE_HTTP

#define MAJOR_VERSION "101"
#define MINOR_VERSION "5"

#define MESSAGE_MAX_LEN 300
#define INTIAL_DELAY_VALUE 600
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

/* Paste in the your iothub device connection string  */
static const char *connectionString = CONFIG_IOTHUB_CONNECTION_STRING;
const char *messageData = "{\"Time\":\"%s\", \"DeviceId\":\"%s\", \"Temperature\":%f, \"Humidity\":%f, \"Pressure\":%f}";
const char s8MotorOnResponseArray[] = "{ \"Response\": \"ON\" }";
const char s8MotorOffResponseArray[] = "{ \"Response\": \"OFF\" }";
const char s8EmptyResponseArray[] = "{ }";
int messageCount = 1;
#define DOWORK_LOOP_NUM     3
typedef enum
{
    csON,
    csOFF
}AzCore_enumMotorStateType;
typedef struct
{
    char * ps8MajorVersion;      /* reported property */
    char * ps8MinorVersion;      /* reported property */
}AzCore_strFirmwareVersionType;
typedef struct
{
    uint8_t  u8MotorSpeedPercentage;     /* desired property */
    uint32_t u32ReportingDelayInSeconds; /* desired property */
}AzCore_strSettingsType;
typedef struct
{
    AzCore_enumMotorStateType objenumMotorState;
}AzCore_strOperationType;
typedef struct
{
    AzCore_strOperationType       objstrOperationType;
    AzCore_strFirmwareVersionType objstrFirmwareVersion;/* reported property */
    AzCore_strSettingsType        objstrSettings;       /* desired property */
}AzCore_strConfigurationType;

static AzCore_strConfigurationType objstrConfiguration;
/*
{
  "desired": {
    "Settings": {
      "MotorMaxSpeed": 50,
      "ReportingInterval": 600
    }
  },
  "reported": {
    "Operation": {
      "MotorState": "ON"
    },
    "FirmwareVersion": {
      "MajorVersion": "010",
      "MinorVersion": "5"
    }
  }
}
*/
/*
{
"payload": {
        "input1": "ON",
        "input2": "ON"
    }
}
*/
void Azcore_vGetPassingParamters(const unsigned char* payload, char **ps8Parameter1, char **ps8Parameter2)
{
    JSON_Value* pRootValue = json_parse_string((char *)payload);
    JSON_Object* pRootObject = json_value_get_object(pRootValue);

    JSON_Value* objParameter1JSONValue = json_object_dotget_value(pRootObject, "payload.input1");
    JSON_Value* objParameter2JSONValue = json_object_dotget_value(pRootObject, "payload.input2");

    if (objParameter1JSONValue != NULL)
    {
        const char* ps8TempParameter1 = json_value_get_string(objParameter1JSONValue);
        if (ps8TempParameter1 != NULL)
        {
            size_t size = strlen(ps8TempParameter1) + 1;
            *ps8Parameter1 = malloc(size);
            if (*ps8Parameter1 != NULL)
            {
                (void)memcpy(*ps8Parameter1, ps8TempParameter1, size);
            }
        }
    }
    if (objParameter2JSONValue != NULL)
        {
            const char* ps8TempParameter2 = json_value_get_string(objParameter1JSONValue);
            if (ps8TempParameter2 != NULL)
            {
                size_t size = strlen(ps8TempParameter2) + 1;
                *ps8Parameter2 = malloc(size);
                if (*ps8Parameter2 != NULL)
                {
                    (void)memcpy(*ps8Parameter2, ps8TempParameter2, size);
                }
            }
        }
    // Free resources
    json_value_free(pRootValue);
}

static char *Azcore_tSerialize(AzCore_strConfigurationType *pstrConfiguration)
{
    char *pSerializeData;

    JSON_Value *pRootValue = json_value_init_object();
    JSON_Object *pRootObject = json_value_get_object(pRootValue);
    /* will send the reported properties only */
    (void) json_object_dotset_string(pRootObject, "FirmwareVersion.MajorVersion", pstrConfiguration->objstrFirmwareVersion.ps8MajorVersion);
    (void) json_object_dotset_string(pRootObject, "FirmwareVersion.MinorVersion", pstrConfiguration->objstrFirmwareVersion.ps8MinorVersion);
    if(pstrConfiguration->objstrOperationType.objenumMotorState == csON)
    {
        (void) json_object_dotset_string(pRootObject, "Operation.MotorState", "ON");
    }
    else
    {
        (void) json_object_dotset_string(pRootObject, "Operation.MotorState", "OFF");
    }
    pSerializeData = json_serialize_to_string(pRootValue);
    json_value_free(pRootValue);

    return pSerializeData;
}
/*
 * Converts the desired properties of the Device Twin JSON blob received from IoT Hub into a
 * AzCore_strConfigurationType object.
 */
static AzCore_strConfigurationType* Azcore_tParseFromJson(const char *ps8JsonData, DEVICE_TWIN_UPDATE_STATE objUpdateState)
{
    JSON_Value *pRootValue = NULL;
    JSON_Object *pRootObject = NULL;

    AzCore_strConfigurationType *pstrConfiguration = malloc(sizeof(AzCore_strConfigurationType));

    if(pstrConfiguration == NULL) /* couldn't allocate AzCore_strConfigurationType */
    {
        (void)printf("ERROR: Failed to allocate memory\r\n");
    }
    else
    {
        (void) memset(pstrConfiguration, 0, sizeof(AzCore_strConfigurationType));
        pRootValue = json_parse_string(ps8JsonData);
        pRootObject = json_value_get_object(pRootValue);
        /* will extract only desired properties */
        JSON_Value *pMotorSpeedPercentageJsonValue;
        JSON_Value *pReportingDelayJsonValue;
        /*
         * The JSON structure different from complete update and partial update
         */
        if(objUpdateState == DEVICE_TWIN_UPDATE_COMPLETE)
        {
            pMotorSpeedPercentageJsonValue = json_object_dotget_value(pRootObject, "desired.Settings.MotorMaxSpeed");
            pReportingDelayJsonValue = json_object_dotget_value(pRootObject, "desired.Settings.ReportingInterval");
        }
        else /* Partial update */
        {
            pMotorSpeedPercentageJsonValue = json_object_dotget_value(pRootObject, "Settings.MotorMaxSpeed");
            pReportingDelayJsonValue = json_object_dotget_value(pRootObject, "Settings.ReportingInterval");
        }
        /* now check which values are received to load the AzCore_strConfigurationType object */
        if(pMotorSpeedPercentageJsonValue != NULL)
        {
            pstrConfiguration->objstrSettings.u8MotorSpeedPercentage = \
                    (uint8_t) json_value_get_number(pMotorSpeedPercentageJsonValue);
        }
        else
        {
            /*
             * Nothing to do
             */
        }
        if(pReportingDelayJsonValue != NULL)
        {
            pstrConfiguration->objstrSettings.u32ReportingDelayInSeconds = \
                    (uint32_t) json_value_get_number(pReportingDelayJsonValue);
        }
        else
        {
            /*
             * Nothing to do
             */
        }

    }


    return pstrConfiguration;
}

static int deviceMethodCallback(const char *method_name, const unsigned char *payload, int size,
        unsigned char **response, int *response_size, void *userContextCallback)
{
    (void) userContextCallback;
    (void) payload;
    (void) size;
    const char * ps8Response;

    printf("Method Name: %s and payload %s\n", method_name, payload);
    int result = 200;
    if(strcmp("getMotorState", method_name) == 0)
    {
        if(objstrConfiguration.objstrOperationType.objenumMotorState == csON)
        {
            ps8Response = s8MotorOnResponseArray;
            *response_size = sizeof(s8MotorOnResponseArray) - 1;
        }
        else
        {
            ps8Response = s8MotorOffResponseArray;
            *response_size = sizeof(s8MotorOffResponseArray) - 1;
        }
    }
    else if(strcmp("SetMotorState", method_name) == 0)
    {
        char *ps8FirstParameter, *ps8SecondParameter;
        ps8FirstParameter = NULL;
        ps8SecondParameter = NULL;
        Azcore_vGetPassingParamters(payload, &ps8FirstParameter, &ps8SecondParameter);
        if(ps8FirstParameter != NULL)
        {
            if(strcmp("ON", (char*)ps8FirstParameter) == 0)
            {
                printf("Turn Motor ON\r\n");
                ps8Response = s8MotorOnResponseArray;
                *response_size = sizeof(s8MotorOnResponseArray) - 1;
                objstrConfiguration.objstrOperationType.objenumMotorState = csON;
            }
            else if(strcmp("OFF", (char*)ps8FirstParameter) == 0)
            {
                printf("Turn Motor OFF\r\n");
                ps8Response = s8MotorOffResponseArray;
                *response_size = sizeof(s8MotorOffResponseArray) - 1;
                objstrConfiguration.objstrOperationType.objenumMotorState = csOFF;
            }
            else
            {
                // All other entries are ignored.
                ps8Response = s8EmptyResponseArray;
                *response_size = sizeof(s8EmptyResponseArray) - 1;
                result = -1; /* override the good result */
            }
        }
        else
        {
            ps8Response = s8EmptyResponseArray;
            *response_size = sizeof(s8EmptyResponseArray) - 1;
            result = -1; /* override the good result */
        }
    }
    else
    {
        // All other entries are ignored.
        ps8Response = s8EmptyResponseArray;
        *response_size = sizeof(s8EmptyResponseArray) - 1;
        result = -1; /* override the good result */
    }
    *response = malloc(*response_size);
    if(*response != NULL)
    {
        (void) memcpy(*response, ps8Response, *response_size);
    }
    else
    {
        result = -1; /* override the good result */
    }

    return result;
}

static void deviceTwinCallback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char *payLoad, int size,
        void *userContextCallback)
{
    (void) update_state;
    (void) size;
    AzCore_strConfigurationType *pstrOldConfiguration = (AzCore_strConfigurationType*) userContextCallback;
    AzCore_strConfigurationType *pstrNewConfiguration = Azcore_tParseFromJson((const char*) payLoad, update_state);
    if(pstrNewConfiguration != NULL)
    {
        if(pstrNewConfiguration->objstrSettings.u32ReportingDelayInSeconds != 0)
        {
            if(pstrNewConfiguration->objstrSettings.u32ReportingDelayInSeconds != \
                    pstrOldConfiguration->objstrSettings.u32ReportingDelayInSeconds)
            {
                printf("Received a new reporting delay = %" PRIu32 "\n", \
                        pstrNewConfiguration->objstrSettings.u32ReportingDelayInSeconds);
                pstrOldConfiguration->objstrSettings.u32ReportingDelayInSeconds = \
                        pstrNewConfiguration->objstrSettings.u32ReportingDelayInSeconds;
            }
        }
        if(pstrNewConfiguration->objstrSettings.u8MotorSpeedPercentage != 0)
        {
            if(pstrNewConfiguration->objstrSettings.u8MotorSpeedPercentage != \
                    pstrOldConfiguration->objstrSettings.u8MotorSpeedPercentage)
            {
                printf("Received a new desired_maxSpeed = %" PRIu8 "\n", \
                        pstrNewConfiguration->objstrSettings.u8MotorSpeedPercentage);
                pstrOldConfiguration->objstrSettings.u8MotorSpeedPercentage = \
                        pstrNewConfiguration->objstrSettings.u8MotorSpeedPercentage;
            }
        }
        free(pstrNewConfiguration); /* release the allocated object in Azcore_tParseFromJson */
    }
    else
    {
        printf("Error: JSON parsing failed!\r\n");
    }
}
static void Azure_s32InnerProcess(void)
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
    // Select the Protocol to use with the connection
#ifdef SAMPLE_MQTT
    protocol = MQTT_Protocol;
#endif // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
    protocol = MQTT_WebSocket_Protocol;
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS
#ifdef SAMPLE_AMQP
    protocol = AMQP_Protocol;
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
    protocol = AMQP_Protocol_over_WebSocketsTls;
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
    protocol = HTTP_Protocol;
#endif // SAMPLE_HTTP
    memset(&objstrConfiguration, 0, sizeof(AzCore_strConfigurationType));
    objstrConfiguration.objstrFirmwareVersion.ps8MajorVersion = MAJOR_VERSION;
    objstrConfiguration.objstrFirmwareVersion.ps8MinorVersion = MINOR_VERSION;
    objstrConfiguration.objstrOperationType.objenumMotorState = csOFF;
    objstrConfiguration.objstrSettings.u32ReportingDelayInSeconds = INTIAL_DELAY_VALUE;
    Azure_strConnectionInfoType objstrConnectionInfoType;
    objstrConnectionInfoType.bEnableDeviceMethod = true;
    objstrConnectionInfoType.bEnableDeviceTwin = true;
    objstrConnectionInfoType.bEnabletracing = true;
    objstrConnectionInfoType.objProtocolTypeType = protocol;
    objstrConnectionInfoType.ps8DeviceConnectionString = connectionString;
    objstrConnectionInfoType.ps8certificates = NULL;
    objstrConnectionInfoType.s32ConnectionTrialIntervalMS = 30000;
    objstrConnectionInfoType.s32KeepaliveInSeconds = 120;
    objstrConnectionInfoType.pFuncDeviceMethodCallBack = deviceMethodCallback;
    objstrConnectionInfoType.pFuncDeviceTwinCallBack = deviceTwinCallback;
    objstrConnectionInfoType.pvTwinUserContextCBk = (void *)&objstrConfiguration;
    if(AzClient_bInitialize(&objstrConnectionInfoType) == false)
    {
        (void) printf("Failed to initialize the platform.\r\n");
    }
    else
    {
        (void) printf("Good Init.\r\n");
        char *reportedProperties = Azcore_tSerialize(&objstrConfiguration);
        if(reportedProperties != NULL)
        {
            AzClient_bReportState(reportedProperties);
            free(reportedProperties);
        }
        else
        {
            printf("Error: JSON serialization failed!\r\n");
        }
        time_t sent_time = 0;
        time_t current_time = 0;
        while(1)
        {
            time(&current_time);
            double timediffrent = difftime(current_time, sent_time);
            if(timediffrent > objstrConfiguration.objstrSettings.u32ReportingDelayInSeconds)
            {
                char messagePayload[MESSAGE_MAX_LEN];
                float temperature = (rand() % 10);
                float humidity = (rand() % 20);
                float pressure = (rand() % 30);
                char strftime_buf[64];
                struct tm timeinfo;
                time_t now = time(NULL);
                localtime_r(&now, &timeinfo);
                strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%dT%H:%M:%S.0Z", &timeinfo);
                snprintf(messagePayload, MESSAGE_MAX_LEN, messageData,strftime_buf, CONFIG_DEVICE_ID , temperature, humidity, pressure);
                printf("%s\r\n", messagePayload);
                AzClient_bSendEventInstance(AzClient_tBuildEvent(messagePayload, MESSAGE));
                time(&sent_time);
            }
            else
            {
                AzClient_vTick();
            }
            ThreadAPI_Sleep(10);
        }

    }
}

int AzCore_s32Process(void)
{
    Azure_s32InnerProcess();
    return 0;
}

