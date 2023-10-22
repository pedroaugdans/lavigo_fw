/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
 /***
  *     /$$$$$$$$                                          /$$     /$$
  *    | $$_____/                                         | $$    |__/
  *    | $$      /$$   /$$  /$$$$$$   /$$$$$$$ /$$   /$$ /$$$$$$   /$$  /$$$$$$  /$$$$$$$
  *    | $$$$$  |  $$ /$$/ /$$__  $$ /$$_____/| $$  | $$|_  $$_/  | $$ /$$__  $$| $$__  $$
  *    | $$__/   \  $$$$/ | $$$$$$$$| $$      | $$  | $$  | $$    | $$| $$  \ $$| $$  \ $$
  *    | $$       >$$  $$ | $$_____/| $$      | $$  | $$  | $$ /$$| $$| $$  | $$| $$  | $$
  *    | $$$$$$$$/$$/\  $$|  $$$$$$$|  $$$$$$$|  $$$$$$/  |  $$$$/| $$|  $$$$$$/| $$  | $$
  *    |________/__/  \__/ \_______/ \_______/ \______/    \___/  |__/ \______/ |__/  |__/
  *
  *
  *
  */
#include "hubware.h"


#ifdef __TESTING_FALLBACK__

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define ADAPTER 2

#include "execution.h"
#include "params.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "machines.h"
#include "drv_i2c.h"
#include "clk_pit.h"
#include "pin_xio.h"
#include "pin_evt.h"
#include "pin_seq.h"
#include "cJSON.h"
#include "drv_nvs.h"
#include "drv_spiffs.h"
#include "registration.h"
#include "fallback.h"
#include "port_xio.h"
#include "string.h"

static const char *TAG = "[TEST-EXECUTE]";

void hw_init(void);
void sw_init(void);
void fw_run(void);

void fsm_run(void *params);

void hubware_init(void) {
    hw_init();
    sw_init();
}

void hubware_run(void) {
    fw_run();
}

void hw_init(void) {
    /*DEBUG*/ESP_LOGI(TAG, "hw_init()");
    drv_nvs_init();
    drv_i2c_init();
    clk_pit_init();

    /*DEBUG*/ESP_LOGI(TAG, "~hw_init()");
}

void sw_init(void) {
    /*DEBUG*/ESP_LOGI(TAG, "sw_init()");
    params_init();
    machines_init();
    pin_xio_init();
    port_xio_init();
    pin_evt_init();
    pin_seq_init();

    /*LAYOUT VERSION is necessary for any machine operation*/
    registration_set_layout_version(VERSION_0_5_0);
    set_layout_version(version_5_0);

    machines_load();
    run_activation_flags[fallback_run_flag] = 1;

    xTaskCreatePinnedToCore(&pin_in_task, "pin_in_task", 1024 * 4, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&pin_out_task, "pin_out_task", 1024 * 4, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&port_xio_task, "port_xio_task", 1024 * 2, NULL, 3, NULL, 1);

    /*DEBUG*/ESP_LOGI(TAG, "~sw_init()");
}

void fw_run(void) {

    xTaskCreatePinnedToCore(&drv_fallback_task, "fallback_task", 512*5, NULL, 5, NULL, 1);

    /*DEBUG*/ESP_LOGI(TAG, "fw_run()");

    vTaskDelay(10000 / portTICK_RATE_MS);
    while (1) {
        vTaskDelay(10000 / portTICK_RATE_MS);
    }

    /*DEBUG*/ESP_LOGI(TAG, "~fw_run()");
}
#endif
