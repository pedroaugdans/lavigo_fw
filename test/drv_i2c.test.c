//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____   ____ _    __ ______ ____              ____ ___   ______            //
//    / __ \ / __ \ /  _/| |  / // ____// __ \            /  _/|__ \ / ____/            //
//   / / / // /_/ / / /  | | / // __/  / /_/ /  ______    / /  __/ // /                 //
//  / /_/ // _, _/_/ /   | |/ // /___ / _, _/  /_____/  _/ /  / __// /___               //
// /_____//_/ |_|/___/   |___//_____//_/ |_|           /___/ /____/\____/               //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "hubware.h"

#ifdef __TESTING_DRV_I2C__

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "drv_i2c.h"

static const char *TAG = "[TEST-DRV-I2C]";

void hw_init(void);
void sw_init(void);
void fw_run (void);

void fsm_run (void *params);

void hubware_init(void){
  hw_init();
  sw_init();
}

void hubware_run (void) {
  fw_run();
}

void hw_init(void) {
  ESP_LOGI(TAG, "hw_init()");

  drv_i2c_init();

  ESP_LOGI(TAG, "~hw_init()");
}

void sw_init(void) {
  ESP_LOGI(TAG, "sw_init()");

  ESP_LOGI(TAG, "~sw_init()");
}


void I2C_test(void *params) {
  ESP_LOGI(TAG, "I2C_test()");

  uint8_t adapter = 0;

  FOREVER {
    uint8_t data  = 0x00;

    ESP_LOGI(TAG, "spin() [0x%0X]", 0x20 + adapter);

    drv_i2c_write(0x20 + adapter, 0x00, 0xFF);
    drv_i2c_read (0x20 + adapter, 0x00, &data);

    ESP_LOGI(TAG, "[0x%0X : 0x%0X]", 0x00, data);

    drv_i2c_write(0x20 + adapter, 0x01, 0xFF);
    drv_i2c_read (0x20 + adapter, 0x01, &data);

    ESP_LOGI(TAG, "[0x%0X : 0x%0X]", 0x01, data);

    drv_i2c_write(0x20 + adapter, 0x12, 0xAA);
    drv_i2c_read (0x20 + adapter, 0x12, &data);

    ESP_LOGI(TAG, "[0x%0X : 0x%0X]", 0x12, data);

    drv_i2c_write(0x20 + adapter, 0x12, 0x55);
    drv_i2c_read (0x20 + adapter, 0x12, &data);

    ESP_LOGI(TAG, "[0x%0X : 0x%0X]", 0x12, data);

    drv_i2c_write(0x20 + adapter, 0x13, 0xAA);
    drv_i2c_read (0x20 + adapter, 0x13, &data);

    ESP_LOGI(TAG, "[0x%0X : 0x%0X]", 0x13, data);

    drv_i2c_write(0x20 + adapter, 0x13, 0x55);
    drv_i2c_read (0x20 + adapter, 0x13, &data);

    ESP_LOGI(TAG, "[0x%0X : 0x%0X]", 0x13, data);

    vTaskDelay( 1000 / portTICK_RATE_MS);

    drv_i2c_write(0x20 + adapter, 0x00, 0x00);
    drv_i2c_read (0x20 + adapter, 0x00, &data);

    ESP_LOGI(TAG, "[0x%0X : 0x%0X]", 0x00, data);

    drv_i2c_write(0x20 + adapter, 0x01, 0x00);
    drv_i2c_read (0x20 + adapter, 0x01, &data);

    ESP_LOGI(TAG, "[0x%0X : 0x%0X]", 0x00, data);

    drv_i2c_write(0x20 + adapter, 0x12, 0xAA);
    drv_i2c_read (0x20 + adapter, 0x12, &data);

    ESP_LOGI(TAG, "[0x%0X : 0x%0X]", 0x12, data);

    drv_i2c_write(0x20 + adapter, 0x12, 0x55);
    drv_i2c_read (0x20 + adapter, 0x12, &data);

    ESP_LOGI(TAG, "[0x%0X : 0x%0X]", 0x12, data);

    drv_i2c_write(0x20 + adapter, 0x13, 0xAA);
    drv_i2c_read (0x20 + adapter, 0x13, &data);

    ESP_LOGI(TAG, "[0x%0X : 0x%0X]", 0x13, data);

    drv_i2c_write(0x20 + adapter, 0x13, 0x55);
    drv_i2c_read (0x20 + adapter, 0x13, &data);

    ESP_LOGI(TAG, "[0x%0X : 0x%0X]", 0x13, data);

    vTaskDelay( 1000 / portTICK_RATE_MS);

    adapter = (adapter+1)%8;
  }

  ESP_LOGI(TAG, "~I2C_test()");
}

void fw_run(void) {
  ESP_LOGI(TAG, "fw_run()");

  xTaskCreatePinnedToCore(&I2C_test ,"I2C_test" ,1024*4 ,NULL ,5 ,NULL ,1);

  ESP_LOGI(TAG, "~fw_run()");
}

#endif
