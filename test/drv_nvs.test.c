//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____   ____ _    __ ______ ____              _   __ _    __ _____         //
//    / __ \ / __ \ /  _/| |  / // ____// __ \            / | / /| |  / // ___/         //
//   / / / // /_/ / / /  | | / // __/  / /_/ /  ______   /  |/ / | | / / \__ \          //
//  / /_/ // _, _/_/ /   | |/ // /___ / _, _/  /_____/  / /|  /  | |/ / ___/ /          //
// /_____//_/ |_|/___/   |___//_____//_/ |_|           /_/ |_/   |___/ /____/           //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "hubware.h"

#ifdef __TESTING_DRV_NVS__

#include "drv_spiffs.h"
#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "params.h"
#include "drv_nvs.h"

static const char *TAG = "[TEST-DRV-NVS]";

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

  drv_nvs_init();

  ESP_LOGI(TAG, "~hw_init()");
}

void sw_init(void) {
  ESP_LOGI(TAG, "sw_init()");

  ESP_LOGI(TAG, "~sw_init()");
}

void nvs_test(void *params) {
  ESP_LOGI(TAG, "nvs_test()");



  char value[10];

  uint8_t idx = 0;

  FOREVER {
    ESP_LOGI(TAG, "spin()");

    uint8_t status = SUCCESS;

    status = drv_nvs_check(registration_commands[idx]);

    if (status == SUCCESS) {
      status = drv_nvs_get(registration_commands[idx], value);

      ESP_LOGI(TAG, "%s:[%s]", registration_commands[idx], value);
    }
    else {
      ESP_LOGI(TAG, "%s not found!", registration_commands[idx]);
    }

    if (status != SUCCESS) {
      status = drv_nvs_set(registration_commands[idx], "something");

      status = drv_nvs_check(registration_commands[idx]);

      if (status == SUCCESS) {
        status = drv_nvs_get(registration_commands[idx], value);

        ESP_LOGI(TAG, "%s:[%s]", registration_commands[idx], value);
      }
      else {
        ESP_LOGI(TAG, "%s not found!", registration_commands[idx]);
      }
    }

    vTaskDelay( 1000 / portTICK_RATE_MS);

    idx = (idx + 1) % NOF_REG_CMD;
  }

  ESP_LOGI(TAG, "~nvs_test()");
}

void fw_run(void) {
  ESP_LOGI(TAG, "fw_run()");

  xTaskCreatePinnedToCore(&nvs_test ,"nvs_test" ,1024*4 ,NULL ,5 ,NULL ,1);

  ESP_LOGI(TAG, "~fw_run()");
}

#endif
