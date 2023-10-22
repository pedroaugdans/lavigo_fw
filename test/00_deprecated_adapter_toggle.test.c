/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

 /***
  *    ______                              _           _
  *    |  _  \                            | |         | |
  *    | | | |___ _ __  _ __ ___  ___ __ _| |_ ___  __| |
  *    | | | / _ \ '_ \| '__/ _ \/ __/ _` | __/ _ \/ _` |
  *    | |/ /  __/ |_) | | |  __/ (_| (_| | ||  __/ (_| |
  *    |___/ \___| .__/|_|  \___|\___\__,_|\__\___|\__,_|
  *              | |
  *              |_|
  */
#include "hubware.h"

#ifdef __TESTING_ADAPTER__

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define ADAPTER 2

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "drv_i2c.h"
#include "pin_xio.h"

static const char *TAG = "[TEST-PIN-XIO]";

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
  /*DEBUG*/ESP_LOGI(TAG, "hw_init()");

  drv_i2c_init();

  /*DEBUG*/ESP_LOGI(TAG, "~hw_init()");
}
void sw_init(void) {
  /*DEBUG*/ESP_LOGI(TAG, "sw_init()");

  pin_xio_init();

  /*DEBUG*/ESP_LOGI(TAG, "~sw_init()");
}

void gpio_sync(void *params) {
  /*DEBUG*/ESP_LOGI(TAG, "gpio_sync()");

  uint8_t adapter, pin;
  bool data, xdata  = FALSE;

  FOREVER {

    ESP_LOGI(TAG, "spin() [0x%0X]", xdata);

        for (adapter = ADAPTER; adapter < ADAPTER + 1; adapter++) {
            for (pin = 0; pin < 16; pin++) {
                pin_xio_set(adapter, pin, data);
                vTaskDelay(50 / portTICK_RATE_MS);
                pin_xio_set(adapter, pin, !data);
                vTaskDelay(50 / portTICK_RATE_MS);
            }

            vTaskDelay(100 / portTICK_RATE_MS);

      for (pin = 0; pin < 16; pin++) {
        data = xdata;

        pin_xio_get(adapter, pin, &data);

        ESP_LOGI(TAG, "[0x%0X : 0x%X : 0x%0X]", adapter, pin, data);
      }

      vTaskDelay( 100 / portTICK_RATE_MS);
    }

    vTaskDelay( 100 / portTICK_RATE_MS);
  }

  /*DEBUG*/ESP_LOGI(TAG, "~gpio_sync()");
}

void fw_run(void) {
  /*DEBUG*/ESP_LOGI(TAG, "fw_run()");

  xTaskCreatePinnedToCore(&pin_in_task ,"pin_in_task" ,1024*4 ,NULL ,5 ,NULL ,1);
  xTaskCreatePinnedToCore(&gpio_sync    ,"gpio_sync"    ,1024*4 ,NULL ,5 ,NULL ,1);

  /*DEBUG*/ESP_LOGI(TAG, "~fw_run()");
}
#endif
