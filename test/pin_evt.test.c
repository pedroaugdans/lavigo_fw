//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____ _   __            ______ _    __ ______                              //
//    / __ \ /  _// | / /           / ____/| |  / //_  __/                              //
//   / /_/ / / / /  |/ /  ______   / __/   | | / /  / /                                 //
//  / ____/_/ / / /|  /  /_____/  / /___   | |/ /  / /                                  //
// /_/    /___//_/ |_/           /_____/   |___/  /_/                                   //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "hubware.h"

#ifdef __TESTING_PIN_EVT__

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define ADAPTER 5

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "drv_i2c.h"
#include "pin_xio.h"
#include "pin_evt.h"
#include "port_xio.h"
#include "params.h"

static const char *TAG = "[TEST-PIN-EVT]";

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
  pin_evt_init();
  port_xio_init();
  params_init();

  /*DEBUG*/ESP_LOGI(TAG, "~sw_init()");
}

void gpio_trigger(void *params) {
  /*DEBUG*/ESP_LOGI(TAG, "gpio_trigger()");

  uint8_t port, pin, data, xdata = 0x00;
  bool status;
  bool pin_status_holder[MAX_INPUT_TIMESTAMP] = {0};
  pin_evt_event_t event;

  for (port = ((ADAPTER << 4) + 0x00); port <= ((ADAPTER << 4) + 0x0F); port++) {
    pin_evt_register(port);
  }

  FOREVER {
    xdata = 1 - xdata;

    data = xdata;

    ESP_LOGI(TAG, "spin() [0x%0X]", xdata);

    for (port = ((ADAPTER << 4) + 0x00); port <= ((ADAPTER << 4) + 0x0F); port++) {

      pin_evt_push(0, port, 0, data);
      pin_evt_push(0, port, 1, data);
    }

    vTaskDelay( 1000 / portTICK_RATE_MS);

    for (pin = 0; pin < 16; pin++) {
      data = xdata;

      for(input_timestamp_t k = 0; k < MAX_INPUT_TIMESTAMP; k++){
        pin_xio_get(ADAPTER, pin, &status,k);
        pin_status_holder[k] = status;
      }
      ESP_LOGI(TAG, "[0x%0X : 0x%X : {0:%d 1:%d 2:%d 3:%d 4:%d 5:%d}]", ADAPTER, pin,
       pin_status_holder[0],pin_status_holder[1],pin_status_holder[2],pin_status_holder[3],
       pin_status_holder[4],pin_status_holder[5]);
      /*DEBUG*///ESP_LOGI(TAG, "[0x%0X : 0x%X : 0x%0X]", 1, pin, status);
    }

    while (pin_evt_next(&event) == SUCCESS) {
      /*DEBUG*/ESP_LOGI(TAG, "port:[0x%X] level:[0x%X]", event.port, event.level);
    }
    drv_i2c_read (0x20 + ADAPTER, 0x12, &data);
    ESP_LOGI(TAG, "[A : 0x%0X]", data);
    drv_i2c_read (0x20 + ADAPTER, 0x13, &data);
    ESP_LOGI(TAG, "[B : 0x%0X]", data);
    vTaskDelay( 1000 / portTICK_RATE_MS);
  }

  /*DEBUG*/ESP_LOGI(TAG, "~gpio_trigger()");
}

void fw_run(void) {
  /*DEBUG*/ESP_LOGI(TAG, "fw_run()");

  xTaskCreatePinnedToCore(&pin_in_task ,"pin_in_task" ,1024*4 ,NULL ,5 ,NULL ,1);
  xTaskCreatePinnedToCore(&pin_out_task ,"pin_in_task" ,1024*4 ,NULL ,5 ,NULL ,1);
  xTaskCreatePinnedToCore(&pin_evt_task ,"pin_evt_task" ,1024*4 ,NULL ,5 ,NULL ,1);
  xTaskCreatePinnedToCore(&gpio_trigger ,"gpio_trigger" ,1024*4 ,NULL ,5 ,NULL ,1);
  xTaskCreatePinnedToCore(&port_xio_task, "port_xio_task", 1024 * 2, NULL, 3, NULL, 1);

  /*DEBUG*/ESP_LOGI(TAG, "~fw_run()");
}

#endif
