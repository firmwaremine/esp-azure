
#include <stdbool.h>
#include <stdint.h>
#include <sdkconfig.h>
#include "driver/i2c.h"
#include "esp_log.h"

#define I2C_DRIVER_TAG "I2C Driver"
/*
 * I2C configuration
 */
#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)
#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA               /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUMBER(CONFIG_I2C_MASTER_PORT_NUM) /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ CONFIG_I2C_MASTER_FREQUENCY        /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define ACK_VAL 0x0                             /*!< I2C ack value */
#define NACK_VAL 0x1                            /*!< I2C nack value */
#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                       /*!< I2C master will not check ack from slave */


bool InitI2C()
{
    bool status;

    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    esp_err_t error = i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);

    ESP_ERROR_CHECK(error);
    if(error)
    {
        status = false;
    }
    else
    {
        status = true;
    }
    return status;
}
bool writeI2CData(uint8_t ui8Addr, uint8_t *Data, uint8_t ui8ByteCount)
{
    bool status;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ui8Addr << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write(cmd, Data, ui8ByteCount, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t error = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
     if(error == ESP_OK)
     {

         status = true;
     }
     else
     {
         ESP_LOGE(I2C_DRIVER_TAG,"failed to write");
         status = false;
     }
     return status;
}
bool writeI2C(uint8_t ui8Addr, uint8_t ui8Reg, uint8_t *Data, uint8_t ui8ByteCount)
{
    bool status;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ui8Addr << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, ui8Reg, ACK_CHECK_EN);
    i2c_master_write(cmd, Data, ui8ByteCount, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t error = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
     if(error == ESP_OK)
     {

         status = true;
     }
     else
     {
         ESP_LOGE(I2C_DRIVER_TAG,"failed to write");
         status = false;
     }
     return status;
}

bool readI2C(uint8_t ui8Addr, uint8_t ui8Reg, uint8_t *Data, uint8_t ui8ByteCount)
{
    bool status;
    esp_err_t error;

    if (ui8ByteCount == 0)
    {
       return false;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ui8Addr << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, ui8Reg, ACK_CHECK_EN);
    error = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if(error == ESP_OK)
    {
        //vTaskDelay(30 / portTICK_RATE_MS);
        cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, ui8Addr << 1 | READ_BIT, ACK_CHECK_EN);
        if (ui8ByteCount > 1)
        {
           i2c_master_read(cmd, Data, ui8ByteCount - 1, ACK_VAL);
        }
        i2c_master_read_byte(cmd, Data + ui8ByteCount - 1, NACK_VAL);
        i2c_master_stop(cmd);
        error = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);

        if(error == ESP_OK)
        {

            status = true;
        }
        else
        {
            ESP_LOGE(I2C_DRIVER_TAG,"read is failed");
            status = false;
        }
    }
    else
    {
        ESP_LOGE(I2C_DRIVER_TAG,"read is failed");
        status = false;
    }
    return status;
}

bool readBurstI2C(uint8_t ui8Addr, uint8_t ui8Reg, uint8_t *Data, uint32_t ui32ByteCount)
{
    bool status;
    esp_err_t error;

    if (ui32ByteCount == 0)
    {
       return false;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ui8Addr << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, ui8Reg, ACK_CHECK_EN);
    error = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if(error == ESP_OK)
    {
        //vTaskDelay(30 / portTICK_RATE_MS);
        cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, ui8Addr << 1 | READ_BIT, ACK_CHECK_EN);
        if (ui32ByteCount > 1)
        {
           i2c_master_read(cmd, Data, ui32ByteCount - 1, ACK_VAL);
        }
        i2c_master_read_byte(cmd, Data + ui32ByteCount - 1, NACK_VAL);
        i2c_master_stop(cmd);
        error = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);

        if(error == ESP_OK)
        {

            status = true;
        }
        else
        {
            ESP_LOGE(I2C_DRIVER_TAG,"read burst is failed");
            status = false;
        }
    }
    else
    {
        ESP_LOGE(I2C_DRIVER_TAG,"read burst is failed");
        status = false;
    }
    return status;
}


