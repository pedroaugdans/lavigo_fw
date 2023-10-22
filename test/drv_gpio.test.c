/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "hubware.h"
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
#ifdef __TESTING_GPIO__

#include "drv_gpio.h"

#include "lavigoMasterFSM.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "drv_nvs.h"
#include "drv_i2c.h"
#include "drv_uart.h"
#include "drv_wifi.h"
#include "drv_mqtt.h"

#include "launcher.h"
#include "registration.h"
#include "connection.h"
#include "validation.h"
#include "update.h"
#include "monitoring.h"
#include "deployment.h"
#include "execution.h"
#include "drv_console.h"
#include "drv_spiffs.h"

#include "pin_xio.h"
#include "pin_evt.h"


static const char *TAG = "[GPIO-TST]";


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
  drv_gpio_init();

  ESP_LOGI(TAG, "~hw_init()");
}

void gpio_toggle_task(void *params) {
    while(1){
        drv_gpio_high(GPIO_LED_0);
        vTaskDelay(1000/portTICK_RATE_MS);
        drv_gpio_low(GPIO_LED_0);
        vTaskDelay(200/portTICK_RATE_MS);
        drv_gpio_high(GPIO_LED_1);
        vTaskDelay(1000/portTICK_RATE_MS);
        drv_gpio_low(GPIO_LED_1);
        vTaskDelay(200/portTICK_RATE_MS);
    }
}

void gpio_check_task(void *params) {
    bool gpio0_stat=0,apmod_stat=0;
    while(1){
        if(drv_gpio_read(GPIO_0) != gpio0_stat){
            ESP_LOGI(TAG,"GPIO0 stat [%d]",gpio0_stat);
            gpio0_stat = !gpio0_stat;
        }
        if(drv_gpio_read(GPIO_AP_MOD) != apmod_stat){
            ESP_LOGI(TAG,"APMOD stat [%d]",apmod_stat);
            apmod_stat = !apmod_stat;
        }
        vTaskDelay(200/portTICK_RATE_MS);
    }
}


void gpio0_pisr(void){ESP_LOGI(TAG,"GPIO PISR");}

void sw_init(void) {
  ESP_LOGI(TAG, "sw_init()");

  drv_gpio_install_cb(GPIO_0,gpio0_pisr);
  drv_gpio_install_cb(GPIO_AP_MOD,gpio0_pisr);
  xTaskCreatePinnedToCore(    &gpio_toggle_task ,    "gpio_toggle"    ,1024*4 ,NULL  ,5 ,NULL ,1);

  ESP_LOGI(TAG, "~sw_init()");
}

void fw_run(void) {
  ESP_LOGI(TAG, "fw_run()");


  ESP_LOGI(TAG, "~fw_run()");
}





#endif
