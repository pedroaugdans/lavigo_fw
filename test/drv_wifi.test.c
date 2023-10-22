//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____   ____ _    __ ______ ____              _       __ ____ ______ ____  //
//    / __ \ / __ \ /  _/| |  / // ____// __ \            | |     / //  _// ____//  _/  //
//   / / / // /_/ / / /  | | / // __/  / /_/ /  ______    | | /| / / / / / /_    / /    //
//  / /_/ // _, _/_/ /   | |/ // /___ / _, _/  /_____/    | |/ |/ /_/ / / __/  _/ /     //
// /_____//_/ |_|/___/   |___//_____//_/ |_|              |__/|__//___//_/    /___/     //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "hubware.h"

#ifdef __TESTING_DRV_WIFI__

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "drv_nvs.h"
#include "drv_wifi.h"

/*TEST DEFINITIONS*/
/*************************************Wifi Test CONFIG****************************************************/
#define TEST_SSID "Aloha"
#define TEST_PSWD "chaussettes"
/******************************************************************************************************/

static const char *TAG = "[TEST-DRV-WIFI]";

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
  drv_wifi_init();

  ESP_LOGI(TAG, "~hw_init()");
}

void sw_init(void) {
  ESP_LOGI(TAG, "sw_init()");

  ESP_LOGI(TAG, "~sw_init()");
}

void wifi_online_cb(void) {
  ESP_LOGI(TAG, "[online!]");
}

void wifi_offline_cb(void) {
  ESP_LOGI(TAG, "[offline!]");
}

void wifi_test(void *params) {
  ESP_LOGI(TAG, "wifi_test()");

  drv_wifi_install(wifi_online_cb,  Online );
  drv_wifi_install(wifi_offline_cb, Offline);

  drv_wifi_configure(TEST_SSID, TEST_PSWD);

  uint8_t online = FALSE;

  FOREVER {
    ESP_LOGI(TAG, "spin()");

    if (online == FALSE) {
      drv_wifi_connect();
    }

    for (uint8_t i = 0; i < 40; i++) {
      if (drv_wifi_check() == TRUE) {
        online = TRUE;

        break;
      }

      ESP_LOGI(TAG, "waiting...");
      vTaskDelay( 250 / portTICK_RATE_MS);
    }

    if (online) {
      ESP_LOGI(TAG, "online!");
    }

    vTaskDelay( 2000 / portTICK_RATE_MS);
  }

  ESP_LOGI(TAG, "~wifi_test()");
}

void fw_run(void) {
  ESP_LOGI(TAG, "fw_run()");

  xTaskCreatePinnedToCore(&wifi_test ,"wifi_test" ,1024*4 ,NULL ,5 ,NULL ,1);

  ESP_LOGI(TAG, "~fw_run()");
}

#endif
