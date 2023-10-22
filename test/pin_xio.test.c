//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____ _   __            _  __  ____ ____                                   //
//    / __ \ /  _// | / /           | |/ / /  _// __ \                                  //
//   / /_/ / / / /  |/ /  ______    |   /  / / / / / /                                  //
//  / ____/_/ / / /|  /  /_____/   /   | _/ / / /_/ /                                   //
// /_/    /___//_/ |_/            /_/|_|/___/ \____/                                    //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "hubware.h"

#ifdef __TESTING_PIN_XIO__

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define ADAPTER 7

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
  bool pin_status_holder[MAX_INPUT_TIMESTAMP] = {0};

  uint8_t adapter, pin;
  bool data, xdata  = FALSE;

  FOREVER {
    xdata = !xdata;

    ESP_LOGI(TAG, "spin() [0x%0X]", xdata);

    for (adapter = 0; adapter < ADAPTER+1; adapter++) {
      for (pin = 0; pin < 16; pin++) {
        data = xdata;

        pin_xio_set(adapter, pin, data);
      }

      vTaskDelay( 500 / portTICK_RATE_MS);

      for (pin = 0; pin < 16; pin++) {
        data = xdata;
        for(input_timestamp_t k = 0; k < MAX_INPUT_TIMESTAMP; k++){
          pin_xio_get(adapter, pin, &data,k);
          pin_status_holder[k] = data;
        }
        ESP_LOGI(TAG, "[0x%0X : 0x%X : {0:%d 1:%d 2:%d 3:%d 4:%d 5:%d}]", adapter, pin,
         pin_status_holder[0],pin_status_holder[1],pin_status_holder[2],pin_status_holder[3],
       pin_status_holder[4],pin_status_holder[5]);
      }

      vTaskDelay( 500 / portTICK_RATE_MS);
    }

    vTaskDelay( 1000 / portTICK_RATE_MS);
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
