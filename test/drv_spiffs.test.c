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

#ifdef __TESTING_DRV_SPIFFS__

#include "drv_spiffs.h"
#include "base64_coding.h"
#include "string.h"

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "[TEST-DRV-SPIFFS]";

void hw_init(void);
void sw_init(void);
void fw_run (void);
void test_sleep(uint8_t ticks);

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

  drv_spiffs_init();

  ESP_LOGI(TAG, "~hw_   init()");
}

void sw_init(void) {
  ESP_LOGI(TAG, "sw_init()");

  ESP_LOGI(TAG, "~sw_init()");
}

void spiffs_test(void *params) {
  const char * dummie_file = "hello";
  const char * dummie_msg = "hello Lavigo! \n I am your saving filesystem. ";
  char dummie_buffer[100] = {0};
  char undummied_buffer[100] = {0};

  FOREVER {
    b64_encode((const unsigned char *)dummie_msg,dummie_buffer,strlen(dummie_msg));
    ESP_LOGI(TAG,"Encoded: %s",dummie_buffer);
    drv_spiffs_set  (dummie_file,(const char *)dummie_buffer);
    test_sleep(10);
    drv_spiffs_get(dummie_file,dummie_buffer);
    b64_decode(dummie_buffer, (unsigned char *)undummied_buffer);
    ESP_LOGI(TAG,"Decoded: %s",undummied_buffer);
    ESP_LOGI(TAG,"%s",dummie_buffer);
    test_sleep(10);
    ESP_LOGI(TAG,"TEST SPIFFS");
  }

  ESP_LOGI(TAG, "~spiffs_test()");
}

void fw_run(void) {
  ESP_LOGI(TAG, "fw_run()");

  xTaskCreatePinnedToCore(&spiffs_test ,"spiffs_test" ,1024*4 ,NULL ,5 ,NULL ,1);

  ESP_LOGI(TAG, "~fw_run()");
}

void test_sleep(uint8_t ticks){
  vTaskDelay(100*ticks / portTICK_RATE_MS);
}

#endif
