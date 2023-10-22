//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____ _   __            ______ _    __ ______                              //
//    / __ \ /  _// | / /           / ____/| |  / //_  __/                              //
//   / /_/ / / / /  |/ /  ______   / __/   | | / /  / /                                 //
//  / ____/_/ / / /|  /  /_____/  / /___   | |/ /  / /                                  //
// /_/    /___//_/ |_/           /_____/   |___/  /_/                                   //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "hubware.h"

#ifdef __TESTING_PORT_XIO__

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
  port_xio_init();
  params_init();

  /*DEBUG*/ESP_LOGI(TAG, "~sw_init()");
}

/*Port status 1-place buffer*/
static bool fallback_port_last_state[256] = {0};
void port_trigger(void *params) {
  /*DEBUG*/ESP_LOGI(TAG, "port_trigger()");
  bool xdata = 0;
  uint8_t reading_port = 0;
  bool port_state = 0;

  FOREVER {
    /*Toggle all machine outputs for 2 seconds*/
    for(uint8_t k = 0;k < 32; k++){
      port_set(hub_machines_output_ports_list[k], xdata);
    }
    vTaskDelay(1000/portTICK_RATE_MS);

    /*Read all resource inputs*/
    for(uint8_t k = 0;k < 32; k++){
      reading_port = hub_machines_input_ports_list[k];
      if(port_look_for_status(reading_port,!fallback_port_last_state[(uint8_t) reading_port]) == SUCCESS){
        port_state = !fallback_port_last_state[(uint8_t) reading_port];
        fallback_port_last_state[(uint8_t) reading_port] = port_state;
        ESP_LOGI(TAG, "[Resource] Read [%d] from  [%d]",port_state ,reading_port);
      }
    }
    /*Read all retrofit inputs*/
    for(uint8_t k = 0;k < 32; k++){
      reading_port = hub_retrofit_input_ports_list[k];
      if(port_look_for_status(reading_port,!fallback_port_last_state[(uint8_t) reading_port]) == SUCCESS){
        port_state = !fallback_port_last_state[(uint8_t) reading_port];
        fallback_port_last_state[(uint8_t) reading_port] = port_state;
        ESP_LOGI(TAG, "[Resource] Read [%d] from  [%d]",port_state ,reading_port);
      }
    }
    /*Toggle all machine outputs for 2 seconds*/
    xdata = !xdata;
    for(uint8_t k = 0;k < 32; k++){
      port_set(hub_machines_output_ports_list[k], xdata);
    }
  }
  vTaskDelay(1000/portTICK_RATE_MS);

  /*DEBUG*/ESP_LOGI(TAG, "~port_trigger()");
}

void fw_run(void) {
  /*DEBUG*/ESP_LOGI(TAG, "fw_run()");

  xTaskCreatePinnedToCore(&pin_in_task ,"pin_in_task" ,1024*4 ,NULL ,5 ,NULL ,1);
  xTaskCreatePinnedToCore(&pin_out_task ,"pin_in_task" ,1024*4 ,NULL ,5 ,NULL ,1);
  xTaskCreatePinnedToCore(&port_trigger ,"port_trigger" ,1024*4 ,NULL ,5 ,NULL ,1);
  xTaskCreatePinnedToCore(&port_xio_task, "port_xio_task", 1024 * 2, NULL, 3, NULL, 1);

  /*DEBUG*/ESP_LOGI(TAG, "~fw_run()");
}

#endif
