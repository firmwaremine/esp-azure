/*
 * SensorCore.c
 *
 *  Created on: Aug 12, 2020
 *      Author: mfawz
 */
#include "SensorCore.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <math.h>
#include "bmi160_support.h"
#include "bme280_support.h"
#include "tmp007.h"
#include "opt3001.h"
#include "esp_log.h"
#include "azure_c_shared_utility/threadapi.h"
#include "LCDIIC.h"

//***** Definitions *****
#define USING_BOSCH_BP
#define SAMPLE_TIME_1               (53)
#define SAMPLE_TIME_2               (26)
#define SAMPLE_TIME_4               (13)
#define SAMPLE_TIME_6               (8)
#define SAMPLE_TIME_8               (6)
#define SAMPLE_TIME_10              (5)
#define NUM_AVGR_SUMS               2 //x^2 frames

const uint8_t timeSamplesBMI [6] = {
        SAMPLE_TIME_1,      //Sample at 1 time per second
        SAMPLE_TIME_2,      //Sample at 2 times per second
        SAMPLE_TIME_4,      //Sample at 4 times per second
        SAMPLE_TIME_6,      //Sample at 6 times per second
        SAMPLE_TIME_8,      //Sample at 8 times per second
        SAMPLE_TIME_10,     //Sample at 10 times per second
};

const uint8_t timeSamplesBMM [6] = {
        SAMPLE_TIME_1,      //Sample at 1 time per second
        SAMPLE_TIME_2,      //Sample at 2 times per second
        SAMPLE_TIME_4,      //Sample at 4 times per second
        SAMPLE_TIME_6,      //Sample at 6 times per second
        SAMPLE_TIME_8,      //Sample at 8 times per second
        SAMPLE_TIME_10,     //Sample at 10 times per second
};

const uint8_t timeSamplesBME [6] = {
        SAMPLE_TIME_1,      //Sample at 1 time per second
        SAMPLE_TIME_2,      //Sample at 2 times per second
        SAMPLE_TIME_4,      //Sample at 4 times per second
        SAMPLE_TIME_6,      //Sample at 6 times per second
        SAMPLE_TIME_8,      //Sample at 8 times per second
        SAMPLE_TIME_10,     //Sample at 10 times per second
};

const uint8_t timeSamplesTMP [6] = {
        SAMPLE_TIME_1,      //Sample at 1 time per second
        SAMPLE_TIME_2,      //Sample at 2 times per second
        SAMPLE_TIME_4,      //Sample at 4 times per second
        SAMPLE_TIME_6,      //Sample at 6 times per second
        SAMPLE_TIME_8,      //Sample at 8 times per second
        SAMPLE_TIME_10,     //Sample at 10 times per second
};

const uint8_t timeSamplesOPT [6] = {
        SAMPLE_TIME_1,      //Sample at 1 time per second
        SAMPLE_TIME_2,      //Sample at 2 times per second
        SAMPLE_TIME_4,      //Sample at 4 times per second
        SAMPLE_TIME_6,      //Sample at 6 times per second
        SAMPLE_TIME_8,      //Sample at 8 times per second
        SAMPLE_TIME_10,     //Sample at 10 times per second
};

//Default time sample values for each sensor
volatile uint8_t sampleTimePeriodBMI = 2; //2
volatile uint8_t sampleTimePeriodBMM = 5; //5
volatile uint8_t sampleTimePeriodBME = 0;
volatile uint8_t sampleTimePeriodTMP = 0;
volatile uint8_t sampleTimePeriodOPT = 0;
// BMI160
BMI160_RETURN_FUNCTION_TYPE returnValue;
struct bmi160_gyro_t        s_gyroXYZ;
struct bmi160_accel_t       s_accelXYZ;
struct bmi160_mag_xyz_s32_t s_magcompXYZ;

// BME280
s32 g_s32ActualTemp   = 0;
u32 g_u32ActualPress  = 0;
u32 g_u32ActualHumity = 0;
static float fTemperature;
static float fHumidity;
static float fPressure;
static float fAltitude;
// OPT3001
uint16_t rawData;
float    convertedLux;

// TMP007
uint16_t rawTemp;
uint16_t rawObjTemp;
float    tObjTemp;
float    tObjAmb;
//Calibration off-sets
int8_t accel_off_x;
int8_t accel_off_y;
int8_t accel_off_z;
int16_t gyro_off_x;
int16_t gyro_off_y;
int16_t gyro_off_z;

//gesture recognition
int getGestures = 1;
int dominant = 0;
uint16_t gyroAbsX, gyroAbsY, gyroAbsZ;
uint16_t deltaAccelX, deltaAccelY, deltaAccelZ;
int16_t prevAccelX = 0;
int16_t prevAccelY = 0;
int16_t prevAccelZ = 0;
int16_t prevGyroX = 0;
int16_t prevGyroY = 0;
int16_t prevGyroZ = 0;
int16_t stillCount = 0;
int32_t gyroAvgX = 0.0;
int32_t gyroAvgY = 0.0;
int32_t gyroAvgZ = 0.0;
int32_t accelAvgX = 0.0;
int32_t accelAvgY = 0.0;
int32_t accelAvgZ = 0.0;

//Sensor Status Variables
bool BME_on = true;
bool BMI_on = true;
bool TMP_on = true;
bool OPT_on = true;
// TMP007
uint16_t rawTemp;
uint16_t rawObjTemp;
float    tObjTemp;
float    tObjAmb;
// OPT3001
uint16_t rawData;
float    convertedLux;

#define SENSOR_TAG "Sensors"
#define SEA_LEVEL_PRESSURE_HPA (1013.25)
void Sensor_s32Process()
{
    bool bInitStatus;
    s8 s8Status;
    char s8Buffer[100];
    bInitStatus = bmi160_initialize_sensor();
    if(bInitStatus == true)
    {
        ESP_LOGI(SENSOR_TAG,"bmi160 is successfully init");
    }
    else
    {
        ESP_LOGE(SENSOR_TAG,"bmi160 is failed to init");
    }

    s8Status = bmi160_config_running_mode(APPLICATION_NAVIGATION);
    if(s8Status == SUCCESS)
    {
        ESP_LOGI(SENSOR_TAG,"bmi160 config is success");
    }
    else
    {
        ESP_LOGE(SENSOR_TAG,"bmi160 config is failed");
    }
    s8Status = bmi160_accel_foc_trigger_xyz(0x03, 0x03, 0x01, &accel_off_x, &accel_off_y, &accel_off_z);
    if(s8Status == SUCCESS)
    {
        ESP_LOGI(SENSOR_TAG,"bmi160 accel init is success");
    }
    else
    {
        ESP_LOGE(SENSOR_TAG,"bmi160 accel init is failed");
    }
    s8Status = bmi160_set_foc_gyro_enable(0x01, &gyro_off_x, &gyro_off_y, &gyro_off_z);
    if(s8Status == SUCCESS)
    {
        ESP_LOGI(SENSOR_TAG,"bmi160 gyro init is success");
    }
    else
    {
        ESP_LOGE(SENSOR_TAG,"bmi160 gyro init is failed");
    }
    // Initialize bme280 sensor
    bme280_data_readout_template();
    s8Status = bme280_set_power_mode(BME280_SLEEP_MODE);
    if(s8Status == SUCCESS)
    {
       ESP_LOGI(SENSOR_TAG,"bme280 set Power mode to sleep is success");
    }
    else
    {
       ESP_LOGE(SENSOR_TAG,"bme280 set Power mode to sleep is failed");
    }
    s8Status = bme280_set_power_mode(BME280_NORMAL_MODE);
    if(s8Status == SUCCESS)
    {
        ESP_LOGI(SENSOR_TAG,"bmi160 set power mode to Normal is success");
    }
    else
    {
        ESP_LOGE(SENSOR_TAG,"bmi160 set power mode to Normal is failed");
    }
    // Initialize tmp007 sensor
    bInitStatus = sensorTmp007Init();
    if(bInitStatus == true)
    {
        ESP_LOGI(SENSOR_TAG,"Tmp007 is successfully init");
    }
    else
    {
        ESP_LOGE(SENSOR_TAG,"Tmp007 is failed to init");
    }
    bInitStatus = sensorTmp007Enable(true);
    if(bInitStatus == true)
    {
        ESP_LOGI(SENSOR_TAG,"Tmp007 is successfully enabled");
    }
    else
    {
        ESP_LOGE(SENSOR_TAG,"Tmp007 is failed to enable");
    }
    // Initialize opt3001 sensor
    bInitStatus = sensorOpt3001Init();
    if(bInitStatus == true)
    {
        ESP_LOGI(SENSOR_TAG,"Opt3001 is successfully init");
    }
    else
    {
        ESP_LOGE(SENSOR_TAG,"Opt3001 is failed to init");
    }
    sensorOpt3001Enable(true);
    while(1)
    {
        /* read object temprature */
        sensorTmp007Read(&rawTemp, &rawObjTemp);
        sensorTmp007Convert(rawTemp, rawObjTemp, &tObjTemp, &tObjAmb);
        printf("Object Temperature:%5.2fC\r\n", tObjTemp);
        /* Read and convert OPT values */
        sensorOpt3001Read(&rawData);
        sensorOpt3001Convert(rawData, &convertedLux);
        printf("lux:%5.2f\r\n", convertedLux);
        /* Read BME environmental data */
        bme280_read_pressure_temperature_humidity(&g_u32ActualPress, &g_s32ActualTemp, &g_u32ActualHumity);
        fTemperature = g_s32ActualTemp / 100.0;
        fHumidity   = g_u32ActualHumity / 1024.0 ;
        fPressure   = g_u32ActualPress / 100;
        fAltitude = 44330.0 * (1.0 - pow(fPressure / SEA_LEVEL_PRESSURE_HPA, 0.1903));
        printf("humid:%5.2f%%rH,press:%5.2fhPa,amb_temp:%5.2fC, Altitude:%f\n",
                fHumidity, fPressure, fTemperature, fAltitude);
        /* Read Mag value (BMM) through BMI */
        returnValue = bmi160_bmm150_mag_compensate_xyz(&s_magcompXYZ);
        printf("mag: x:%ld,y:%ld,z:%ld\n",
                s_magcompXYZ.x, s_magcompXYZ.y, s_magcompXYZ.z);
        //LCDIIC_vClearDisplay();
        snprintf(s8Buffer, 100, "Humidity:%5.2f%%rH", fHumidity);
        LCDIIC_vShowStringAt(1 ,1 , s8Buffer);
        snprintf(s8Buffer, 100, "Pressure:%5.2fhPa",fPressure);
        LCDIIC_vShowStringAt(1 ,2 , s8Buffer);
        snprintf(s8Buffer, 100, "Temperature:%5.2fC",fTemperature);
        LCDIIC_vShowStringAt(1 ,3 , s8Buffer);

        ThreadAPI_Sleep(2000);
    }
}
