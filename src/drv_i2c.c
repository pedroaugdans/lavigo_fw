//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____   ____ _    __ ______ ____              ____ ___   ______            //
//    / __ \ / __ \ /  _/| |  / // ____// __ \            /  _/|__ \ / ____/            //
//   / / / // /_/ / / /  | | / // __/  / /_/ /  ______    / /  __/ // /                 //
//  / /_/ // _, _/_/ /   | |/ // /___ / _, _/  /_____/  _/ /  / __// /___               //
// /_____//_/ |_|/___/   |___//_____//_/ |_|           /___/ /____/\____/               //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "drv_i2c.h"
#include "drv_locks.h"

#include <time.h>
#include <stdio.h>

#include "driver/i2c.h"

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define I2C_PORT    I2C_NUM_1
#define I2C_WRITE   I2C_MASTER_WRITE
#define I2C_READ    I2C_MASTER_READ
#define I2C_CHECK   TRUE
#define I2C_ACK     0x00
#define I2C_NACK    0x01
#define I2C_TIMEOUT (100 / portTICK_RATE_MS)

/*** VARIABLES **************************************************************************/

static const char *TAG = "[DRV-I2C]";
/*Semaphore to avoid mixing I2C commands in peripheric*/
SemaphoreHandle_t i2c_lock;

/*** DEFINITIONS ************************************************************************/

/**/
void drv_i2c_init(void) {
    /*DEBUG*/ESP_LOGI(TAG, "init()");
    sph_create(&i2c_lock);

    i2c_config_t conf;

    conf.master.clk_speed = I2C_FREQ_HZ;
    conf.scl_io_num = I2C_SCL_IO;
    conf.sda_io_num = I2C_SDA_IO;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;

    if (sph_step_retries(&i2c_lock) == TRUE) {
        i2c_param_config(I2C_PORT, &conf);
        i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
        sph_give(&i2c_lock);
    } else {
        ESP_LOGE(TAG, "LOCK UNTAKEN");
    }
    ///*DEBUG*/ESP_LOGI(TAG, "~init()");
}

esp_err_t drv_i2c_write(uint8_t address, uint8_t registr, uint8_t data) {
    /*DEBUG*///ESP_LOGI(TAG, "write()");
    esp_err_t err = -1;
    if (sph_step_retries(&i2c_lock) == TRUE) {
        i2c_cmd_handle_t command = i2c_cmd_link_create();

        i2c_master_start(command);
        i2c_master_write_byte(command, (address << 1) | I2C_WRITE, I2C_CHECK);
        i2c_master_write_byte(command, (registr), I2C_CHECK);
        i2c_master_write_byte(command, data, I2C_CHECK);
        i2c_master_stop(command);

        err = i2c_master_cmd_begin(I2C_PORT, command, I2C_TIMEOUT);

        i2c_cmd_link_delete(command);

        if (err == ESP_FAIL) {
            ESP_LOGE(TAG, "(500) Write [%d]", err);
        }
        sph_give(&i2c_lock);
    } else {
        ESP_LOGE(TAG, "[Write] lock untaken");

    }
    /*DEBUG*///ESP_LOGI(TAG, "~write()");

    return err;
}

esp_err_t drv_i2c_read(uint8_t address, uint8_t registr, uint8_t *data) {
    /*DEBUG*///ESP_LOGI(TAG, "read()");
    esp_err_t err = -1;
    if (sph_step_retries(&i2c_lock) == TRUE) {
        i2c_cmd_handle_t command = i2c_cmd_link_create();

        i2c_master_start(command);
        i2c_master_write_byte(command, (address << 1) | I2C_WRITE, I2C_CHECK);
        i2c_master_write_byte(command, (registr), I2C_CHECK);
        i2c_master_start(command);
        i2c_master_write_byte(command, (address << 1) | I2C_READ, I2C_CHECK);
        i2c_master_read_byte(command, data, I2C_NACK);
        i2c_master_stop(command);

        err = i2c_master_cmd_begin(I2C_PORT, command, I2C_TIMEOUT);

        i2c_cmd_link_delete(command);

        if (err == ESP_FAIL) {
            ESP_LOGE(TAG, "(500) Read [%d]", err);
        }
        sph_give(&i2c_lock);
    } else {
        ESP_LOGE(TAG, "[WRITE] Lock untaken");
    }
    /*DEBUG*///ESP_LOGI(TAG, "~read()");

    return err;
}
