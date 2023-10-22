//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____ _   __            _____  ______ ____                                 //
//    / __ \ /  _// | / /           / ___/ / ____// __ \                                //
//   / /_/ / / / /  |/ /  ______    \__ \ / __/  / / / /                                //
//  / ____/_/ / / /|  /  /_____/   ___/ // /___ / /_/ /                                 //
// /_/    /___//_/ |_/            /____//_____/ \___\_\                                 //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "hubware.h"

#ifdef __TESTING_PIN_SEQ__

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define ADAPTER 2

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "machines.h"
#include "drv_i2c.h"
#include "clk_pit.h"
#include "pin_xio.h"
#include "pin_evt.h"
#include "pin_seq.h"
#include "port_xio.h"
#include "params.h"
#include "drv_nvs.h"

#include "string.h"

static const char *TAG = "[TEST-PIN-SEQ]";

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
  clk_pit_init();
  drv_nvs_init();

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
  machines_load();
  /*DEBUG*/ESP_LOGI(TAG, "~sw_init()");
}

void sequence_trigger(void *params) {
  /*DEBUG*/ESP_LOGI(TAG, "sequence_trigger()");

  machines_machine_t *machine_p;
  pin_seq_sequence_t event;
  uint8_t target, sequence;
  uint8_t data;

  char *rows[ROW_MAX] = {
    ROW_1,
    ROW_2,
    ROW_3,
    ROW_4,
  };

  char *columns[COLUMN_MAX] = {
    COLUMN_1,
    COLUMN_2,
    COLUMN_3,
    COLUMN_4,
    COLUMN_5,
    COLUMN_6,
    COLUMN_7,
    COLUMN_8,
  };

  uint8_t row = 0, column = 2;

  FOREVER {
    ESP_LOGI(TAG, "spin()");

    ESP_LOGI(TAG, "--- DEPLOYMENT -----------------------------------------------------");
    machines_make(&machine_p);

    ESP_LOGI(TAG, "row:[%s] column:[%s(%d)], @%p", rows[row], columns[column], column+1,machine_p);

    machine_p->deployment_info.resource = 17;

    machine_p->deployment_info.signals[Resource][0].name = 'S';
    machine_p->deployment_info.signals[Resource][0].port = machines_layout(rows[row], columns[column], 0, 0);
    machine_p->deployment_info.nof_signals[Resource]++;
    machine_p->deployment_info.signals[Resource][1].name = 'A';
    machine_p->deployment_info.signals[Resource][1].port = machines_layout(rows[row], columns[column], 1, 0);
    machine_p->deployment_info.nof_signals[Resource]++;
    machine_p->deployment_info.signals[Retrofit][0].name = 'S';
    machine_p->deployment_info.signals[Retrofit][0].port = machines_layout(rows[row], columns[column], 1, 1);
    machine_p->deployment_info.nof_signals[Retrofit]++;
    machine_p->deployment_info.signals[Retrofit][1].name = 'A';
    machine_p->deployment_info.signals[Retrofit][1].port = machines_layout(rows[row], columns[column], 0, 1);
    machine_p->deployment_info.nof_signals[Retrofit]++;


    strlcpy(machine_p->deployment_info.sequences[Resource][Action][0].name   , "start"                             , strlen("start"                             )+1);
    strlcpy(machine_p->deployment_info.sequences[Resource][Action][0].pattern, "S:L|S:H|%%:14|S:L"                  , strlen("S:L|S:H|%%:14|S:L"                  )+1);
    strlcpy(machine_p->deployment_info.sequences[Resource][Action][0].status , "running"                           , strlen("running"                           )+1);
            machine_p->deployment_info.sequences[Resource][Action][0].target = Resource;
            machine_p->deployment_info.nof_sequences[Resource][Action]++;

    strlcpy(machine_p->deployment_info.sequences[Retrofit][Action][0].name   , "reset"                             , strlen("reset"                             )+1);
    strlcpy(machine_p->deployment_info.sequences[Retrofit][Action][0].pattern, "A:L|A:H|%%:10|A:L|%%:20|A:H|%%:40|A:L", strlen("A:L|A:H|%%:10|A:L|%%:10|A:H|%%:10|A:L")+1);
    strlcpy(machine_p->deployment_info.sequences[Retrofit][Action][0].status , "success"                           , strlen("success"                           )+1);
            machine_p->deployment_info.sequences[Retrofit][Action][0].target = Retrofit;
            machine_p->deployment_info.nof_sequences[Retrofit][Action]++;

    strlcpy(machine_p->deployment_info.sequences[Resource][Event] [0].name   , "start"                             , strlen("start"                             )+1);
    strlcpy(machine_p->deployment_info.sequences[Resource][Event] [0].pattern, "S:l|S:h|S:l|%%:20"                  , strlen("S:l|S:h|S:l|%%:20"                  )+1);
    //strlcpy(machine_p->sequences[Resource][Event] [0].pattern, "S:l"                  , strlen("S:l"                  )+1);
    strlcpy(machine_p->deployment_info.sequences[Resource][Event] [0].status , "success"                           , strlen("success"                           )+1);
            machine_p->deployment_info.sequences[Resource][Event] [0].target = Resource;
            machine_p->deployment_info.nof_sequences[Resource][Event]++;
            ESP_LOGI(TAG,"[%d] sequencies",machine_p->deployment_info.nof_sequences[Resource][Event]);

    strlcpy(machine_p->deployment_info.sequences[Retrofit][Event] [0].name   , "start"                             , strlen("start"                             )+1);
    strlcpy(machine_p->deployment_info.sequences[Retrofit][Event] [0].pattern, "A:l|A:h|A:l"                       , strlen("A:l|A:h|A:l"                       )+1);
    strlcpy(machine_p->deployment_info.sequences[Retrofit][Event] [0].status , "success"                           , strlen("success"                           )+1);
            machine_p->deployment_info.sequences[Retrofit][Event] [0].target = Retrofit;
            machine_p->deployment_info.nof_sequences[Retrofit][Event]++;

    ESP_LOGI(TAG, "--- EXECUTION ------------------------------------------------------");

    ESP_LOGI(TAG, "push resource event: start");
    sequence = machines_sequence(machine_p, Resource, Event, "start");
    pin_seq_push(machine_p, &(machine_p->deployment_info.sequences[Resource][Event][sequence]));

    //ESP_LOGI(TAG, "push retrofit event: start");
    sequence = machines_sequence(machine_p, Retrofit, Event, "start");
    pin_seq_push(machine_p, &(machine_p->deployment_info.sequences[Retrofit][Event][sequence]));

    //ESP_LOGI(TAG, "push resource action: start");
    target   = machines_target  (machine_p, "start");
    sequence = machines_sequence(machine_p, target, Action, "start");
    pin_seq_push(machine_p, &(machine_p->deployment_info.sequences[target][Action][sequence]));

    //ESP_LOGI(TAG, "push retrofit action: reset");
    target   = machines_target  (machine_p, "reset");
    sequence = machines_sequence(machine_p, target, Action, "reset");
    pin_seq_push(machine_p, &(machine_p->deployment_info.sequences[target][Action][sequence]));

    //ESP_LOGI(TAG, "waiting for sequences...");
      while (pin_seq_next(&event) == SUCCESS) {
        ESP_LOGI(TAG, "resource:[%d] target:[%d] name:[%s] pattern:[%s] status:[%s]",
          event.machine_p->deployment_info.resource,
          event.sequence_p->target,
          event.sequence_p->name,
          event.sequence_p->pattern,
          event.sequence_p->status
        );
      }

      //drv_i2c_read (0x20 + ADAPTER, 0x12, &data);
      //ESP_LOGI(TAG, "[A : 0x%0X]", data);
      //drv_i2c_read (0x20 + ADAPTER, 0x13, &data);
      //ESP_LOGI(TAG, "[B : 0x%0X]", data);

    vTaskDelay( 1000 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "--------------------------------------------------------------------");
  }

  /*DEBUG*/ESP_LOGI(TAG, "~sequence_trigger()");
}

void fw_run(void) {
  /*DEBUG*/ESP_LOGI(TAG, "fw_run()");

  xTaskCreatePinnedToCore(&clk_pit_task     ,"clk_pit_task"     ,1024*4 ,NULL ,5 ,NULL ,1);
  xTaskCreatePinnedToCore(&pin_in_task     ,"pin_in_task"     ,1024*4 ,NULL ,5 ,NULL ,1);
  xTaskCreatePinnedToCore(&pin_evt_task     ,"pin_evt_task"     ,1024*4 ,NULL ,5 ,NULL ,1);
  xTaskCreatePinnedToCore(&pin_seq_task     ,"pin_seq_task"     ,1024*4 ,NULL ,5 ,NULL ,1);
  xTaskCreatePinnedToCore(&port_xio_task, "port_xio_task", 1024 * 2, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(&sequence_trigger ,"sequence_trigger" ,1024*4 ,NULL ,5 ,NULL ,1);

  /*DEBUG*/ESP_LOGI(TAG, "~fw_run()");
}

#endif
