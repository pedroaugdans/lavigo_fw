//////////////////////////////////////////////////////////////////////////////////////////
//    ______ __     __ __             ____   ____ ______                                //
//   / ____// /    / //_/            / __ \ /  _//_  __/                                //
//  / /    / /    / ,<     ______   / /_/ / / /   / /                                   //
// / /___ / /___ / /| |   /_____/  / ____/_/ /   / /                                    //
// \____//_____//_/ |_|           /_/    /___/  /_/                                     //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "hubware.h"

#ifdef __TESTING_CLK_PIT__

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "clk_pit.h"

static const char *TAG = "[TEST-CLK_PIT]";

void hw_init(void);
void sw_init(void);
void fw_run (void);

void fsm_run (void *params);

void hubware_init(void){
  ESP_LOGI(TAG,"Now starting unit TEST on clock pit...");
  hw_init();
  sw_init();
}

void hubware_run (void) {
  fw_run();
}

void hw_init(void) {
  /*DEBUG*/ESP_LOGI(TAG, "hw_init()");

  /*DEBUG*/ESP_LOGI(TAG, "~hw_init()");
}
void sw_init(void) {
  /*DEBUG*/ESP_LOGI(TAG, "sw_init()");

  clk_pit_init();

  /*DEBUG*/ESP_LOGI(TAG, "~sw_init()");
}

void time_trigger(void *params) {
  /*DEBUG*/ESP_LOGI(TAG, "time_trigger()");

  clk_pit_event_t event;
  uint16_t ref = 0;

  FOREVER {
    ESP_LOGI(TAG, "spin()");

    uint8_t timers = 0;

    clk_pit_push(ref, 10); timers++;
    ESP_LOGI(TAG,"Pushing event [%d] of [%d] mS",ref,10);
    ref++;
    clk_pit_push(ref, 60); timers++;
    ESP_LOGI(TAG,"Pushing event [%d] of [%d] mS",ref,60);
    ref++;
    clk_pit_push(ref, 110); timers++;
    ESP_LOGI(TAG,"Pushing event [%d] of [%d] mS",ref,110);
    ref++;
    while (timers) {
      while (clk_pit_next(&event) == SUCCESS) {
        timers--;

        /*DEBUG*/ESP_LOGI(TAG, "event ref:[0x%X]", event.ref);
      }

      vTaskDelay( 10 / portTICK_RATE_MS);
    }

    vTaskDelay( 1000 / portTICK_RATE_MS);
  }

  /*DEBUG*/ESP_LOGI(TAG, "~time_trigger()");
}

void fw_run(void) {
  /*DEBUG*/ESP_LOGI(TAG, "fw_run()");

  xTaskCreatePinnedToCore(&clk_pit_task ,"clk_pit_task" ,1024*4 ,NULL ,5 ,NULL ,1);
  xTaskCreatePinnedToCore(&time_trigger ,"time_trigger" ,1024*4 ,NULL ,5 ,NULL ,1);

  /*DEBUG*/ESP_LOGI(TAG, "~fw_run()");
}

#endif
