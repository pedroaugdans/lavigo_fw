//////////////////////////////////////////////////////////////////////////////////////////
//     __  ___ ___    ______ __  __ ____ _   __ ______ _____                            //
//    /  |/  //   |  / ____// / / //  _// | / // ____// ___/                            //
//   / /|_/ // /| | / /    / /_/ / / / /  |/ // __/   \__ \                             //
//  / /  / // ___ |/ /___ / __  /_/ / / /|  // /___  ___/ /                             //
// /_/  /_//_/  |_|\____//_/ /_//___//_/ |_//_____/ /____/                              //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "hubware.h"

#ifdef __TESTING_MACHINES__

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "drv_nvs.h"
#include "machines.h"

#include "string.h"

static const char *TAG = "[TEST-MACHINES]";

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
  drv_nvs_init();
  /*DEBUG*/ESP_LOGI(TAG, "~hw_init()");
}
void sw_init(void) {
  /*DEBUG*/ESP_LOGI(TAG, "sw_init()");

  machines_init();

  /*DEBUG*/ESP_LOGI(TAG, "~sw_init()");
}

void machine_lifecycle(void *params) {
  /*DEBUG*/ESP_LOGI(TAG, "machine_lifecycle()");

  machines_machine_t *machine_p[MACHINES_MAX_MACHINES] = { NULL };

  //machines_machine_t *fetch_machine_p[MAX_MACHINES] = { NULL };

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

  FOREVER {
    ESP_LOGI(TAG, "spin()");

    ESP_LOGI(TAG, "--- LAYOUT ----------------------------------------------------------");

    for (uint8_t column = 0; column < COLUMN_MAX; column ++) {
      for (uint8_t row = 0; row < ROW_MAX; row ++) {
        uint8_t resource_start = machines_layout(rows[row], columns[column], 0, 0);
        uint8_t resource_ack   = machines_layout(rows[row], columns[column], 1, 0);
        uint8_t retrofit_ack   = machines_layout(rows[row], columns[column], 0, 1);
        uint8_t retrofit_start = machines_layout(rows[row], columns[column], 1, 1);

        ESP_LOGI(TAG, "row:[%s] column:[%s] resource:[0x%X][0x%X] retrofit:[0x%X][0x%X]", rows[row], columns[column], resource_start, resource_ack, retrofit_start, retrofit_ack);
      }
    }

    vTaskDelay( 250 / portTICK_RATE_MS);

    ESP_LOGI(TAG, "--- MEMORY ----------------------------------------------------------");

    for (uint16_t max = 1; max < MACHINES_MAX_MACHINES; max = max << 1) {
      ESP_LOGI(TAG, "max:[0x%X]", max);

      for (uint8_t idx = 0; idx < max; idx++) {
        //ESP_LOGI(TAG, "init  [%p]", machine_p[idx]);
      }

      for (uint8_t idx = 0; idx < max; idx++) {
        machines_make(&machine_p[idx]);
        /*******FLAGS******************************/
        machines_clear_all_status(machine_p[0]);
        machine_p[idx]->deployment_info.resource = idx + 1;
      }

      ESP_LOGI(TAG, "make  [0x%X] [%p]", 0+1, machine_p[0]    );
      ESP_LOGI(TAG, "make  [0x%X] [%p]", max, machine_p[max-1]);

      for (uint8_t idx = 0; idx < max; idx++) {
        machines_save(machine_p[idx]);
        //ESP_LOGI(TAG, "save  [%p]", machine_p[idx]);
      }

      machines_machine(&machine_p[0],     0+1);
      machines_machine(&machine_p[max-1], max);
      if(machine_p[max-1] != NULL){
        ESP_LOGI(TAG, "fetch [0x%X] [%p]", 0+1, machine_p[0]    );
      } else {
        ESP_LOGE(TAG,"Error loading machine [%d]",0);
      }

      if(machine_p[max-1] != NULL){
        ESP_LOGI(TAG, "fetch [0x%X] [%p]", max, machine_p[max-1]);
      } else {
        ESP_LOGE(TAG,"Error loading machine [%d]",max-1);
      }

      for (uint8_t idx = 0; idx < max; idx++) {
        machines_free(&machine_p[idx]);
        //ESP_LOGI(TAG, "free  [%p]", machine_p[idx]);
      }
    }

    vTaskDelay( 250 / portTICK_RATE_MS);

    ESP_LOGI(TAG, "---------------------------------------------------------------------");

    machines_make(&machine_p[0]);
    machines_save(machine_p[0]);

    uint8_t row = 1, column = 2;

    ESP_LOGI(TAG, "row:[%s] column:[%s(%d)]", rows[row], columns[column], column+1);

    ESP_LOGI(TAG, "--- SIGNALS ---------------------------------------------------------");

    ESP_LOGI(TAG, "[0x%X]", machines_signal(machine_p[0],Resource,'S'));
    ESP_LOGI(TAG, "[0x%X]", machines_signal(machine_p[0],Resource,'A'));
    ESP_LOGI(TAG, "[0x%X]", machines_signal(machine_p[0],Retrofit,'S'));
    ESP_LOGI(TAG, "[0x%X]", machines_signal(machine_p[0],Retrofit,'A'));

    machine_p[0]->deployment_info.signals[Resource][0].name = 'S';
    machine_p[0]->deployment_info.signals[Resource][0].port = machines_layout(rows[row], columns[column], 0, 0);
    machine_p[0]->deployment_info.nof_signals[Resource]++;
    machine_p[0]->deployment_info.signals[Resource][1].name = 'A';
    machine_p[0]->deployment_info.signals[Resource][1].port = machines_layout(rows[row], columns[column], 1, 0);
    machine_p[0]->deployment_info.nof_signals[Resource]++;
    machine_p[0]->deployment_info.signals[Retrofit][0].name = 'S';
    machine_p[0]->deployment_info.signals[Retrofit][0].port = machines_layout(rows[row], columns[column], 0, 1);
    machine_p[0]->deployment_info.nof_signals[Retrofit]++;
    machine_p[0]->deployment_info.signals[Retrofit][1].name = 'A';
    machine_p[0]->deployment_info.signals[Retrofit][1].port = machines_layout(rows[row], columns[column], 1, 1);
    machine_p[0]->deployment_info.nof_signals[Retrofit]++;

    ESP_LOGI(TAG, "[0x%X]", machines_signal(machine_p[0], Resource, 'S'));
    ESP_LOGI(TAG, "[0x%X]", machines_signal(machine_p[0], Resource, 'A'));
    ESP_LOGI(TAG, "[0x%X]", machines_signal(machine_p[0], Retrofit, 'S'));
    ESP_LOGI(TAG, "[0x%X]", machines_signal(machine_p[0], Retrofit, 'A'));

    vTaskDelay( 250 / portTICK_RATE_MS);

    ESP_LOGI(TAG, "--- SEQUENCES -------------------------------------------------------");

    ESP_LOGI(TAG, "[0x%X]", machines_sequence(machine_p[0], Resource, Action, "start"));
    ESP_LOGI(TAG, "[0x%X]", machines_sequence(machine_p[0], Resource, Action, "reset"));
    ESP_LOGI(TAG, "[0x%X]", machines_sequence(machine_p[0], Retrofit, Action, "deny" ));
    ESP_LOGI(TAG, "[0x%X]", machines_sequence(machine_p[0], Retrofit, Action, "allow"));
    ESP_LOGI(TAG, "[0x%X]", machines_sequence(machine_p[0], Resource, Event , "start"));
    ESP_LOGI(TAG, "[0x%X]", machines_sequence(machine_p[0], Retrofit, Event , "start"));

    strlcpy(machine_p[0]->deployment_info.sequences[Resource][Action][0].name   , "start"           , strlen("start"           )+1);
    strlcpy(machine_p[0]->deployment_info.sequences[Resource][Action][0].pattern, "S:H|%%:10|S:L|A:h", strlen("S:H|%%:10|S:L|A:h")+1);
    strlcpy(machine_p[0]->deployment_info.sequences[Resource][Action][0].status , "running"         , strlen("running"         )+1);
            machine_p[0]->deployment_info.sequences[Resource][Action][0].target = Resource;
            machine_p[0]->deployment_info.nof_sequences[Resource][Action]++;

    strlcpy(machine_p[0]->deployment_info.sequences[Resource][Action][1].name   , "reset"           , strlen("reset"           )+1);
    strlcpy(machine_p[0]->deployment_info.sequences[Resource][Action][1].pattern, "S:L"             , strlen("S:L"             )+1);
    strlcpy(machine_p[0]->deployment_info.sequences[Resource][Action][1].status , "success"         , strlen("success"         )+1);
            machine_p[0]->deployment_info.sequences[Resource][Action][1].target = Resource;
            machine_p[0]->deployment_info.nof_sequences[Resource][Action]++;

    strlcpy(machine_p[0]->deployment_info.sequences[Retrofit][Action][0].name   , "deny"            , strlen("deny"            )+1);
    strlcpy(machine_p[0]->deployment_info.sequences[Retrofit][Action][0].pattern, "A:H"             , strlen("A:H"             )+1);
    strlcpy(machine_p[0]->deployment_info.sequences[Retrofit][Action][0].status , "success"         , strlen("success"         )+1);
            machine_p[0]->deployment_info.sequences[Retrofit][Action][0].target = Retrofit;
            machine_p[0]->deployment_info.nof_sequences[Retrofit][Action]++;

    strlcpy(machine_p[0]->deployment_info.sequences[Retrofit][Action][1].name   , "allow"           , strlen("allow"           )+1);
    strlcpy(machine_p[0]->deployment_info.sequences[Retrofit][Action][1].pattern, "A:L"             , strlen("A:L"             )+1);
    strlcpy(machine_p[0]->deployment_info.sequences[Retrofit][Action][1].status , "success"         , strlen("success"         )+1);
            machine_p[0]->deployment_info.sequences[Retrofit][Action][1].target = Retrofit;
            machine_p[0]->deployment_info.nof_sequences[Retrofit][Action]++;

    strlcpy(machine_p[0]->deployment_info.sequences[Resource][Event] [0].name   , "start"           , strlen("start"           )+1);
    strlcpy(machine_p[0]->deployment_info.sequences[Resource][Event] [0].pattern, "A:l|A:h|A:l"     , strlen("A:l|A:h|A:l"     )+1);
    strlcpy(machine_p[0]->deployment_info.sequences[Resource][Event] [0].status , "success"         , strlen("success"         )+1);
            machine_p[0]->deployment_info.sequences[Resource][Event] [0].target = Resource;
            machine_p[0]->deployment_info.nof_sequences[Resource][Event]++;

    strlcpy(machine_p[0]->deployment_info.sequences[Retrofit][Event] [0].name   , "start"           , strlen("start"           )+1);
    strlcpy(machine_p[0]->deployment_info.sequences[Retrofit][Event] [0].pattern, "S:h"             , strlen("S:h"             )+1);
    strlcpy(machine_p[0]->deployment_info.sequences[Retrofit][Event] [0].status , "trigger"         , strlen("trigger"         )+1);
            machine_p[0]->deployment_info.sequences[Retrofit][Event] [0].target = Retrofit;
            machine_p[0]->deployment_info.nof_sequences[Retrofit][Event]++;

    ESP_LOGI(TAG, "[0x%X] [%s]", machines_sequence(machine_p[0], Resource, Action, "start"),
      machine_p[0]->deployment_info.sequences[Resource][Action][machines_sequence(machine_p[0], Resource, Action, "start")].pattern);
    ESP_LOGI(TAG, "[0x%X] [%s]", machines_sequence(machine_p[0], Resource, Action, "reset"),
      machine_p[0]->deployment_info.sequences[Resource][Action][machines_sequence(machine_p[0], Resource, Action, "reset")].pattern);
    ESP_LOGI(TAG, "[0x%X] [%s]", machines_sequence(machine_p[0], Retrofit, Action, "deny" ),
      machine_p[0]->deployment_info.sequences[Retrofit][Action][machines_sequence(machine_p[0], Retrofit, Action, "deny" )].pattern);
    ESP_LOGI(TAG, "[0x%X] [%s]", machines_sequence(machine_p[0], Retrofit, Action, "allow"),
      machine_p[0]->deployment_info.sequences[Retrofit][Action][machines_sequence(machine_p[0], Retrofit, Action, "allow")].pattern);
    ESP_LOGI(TAG, "[0x%X] [%s]", machines_sequence(machine_p[0], Resource, Event , "start"),
      machine_p[0]->deployment_info.sequences[Resource][Event ][machines_sequence(machine_p[0], Resource, Event , "start")].pattern);
    ESP_LOGI(TAG, "[0x%X] [%s]", machines_sequence(machine_p[0], Retrofit, Event , "start"),
      machine_p[0]->deployment_info.sequences[Retrofit][Event ][machines_sequence(machine_p[0], Retrofit, Event , "start")].pattern);

    /*******FLAGS******************************/
    machines_clear_all_status(machine_p[0]);

    vTaskDelay( 250 / portTICK_RATE_MS);

    ESP_LOGI(TAG, "---------------------------------------------------------------------");

    machines_free(&machine_p[0]);
  }

  /*DEBUG*/ESP_LOGI(TAG, "~machine_lifecycle()");
}

void fw_run(void) {
  /*DEBUG*/ESP_LOGI(TAG, "fw_run()");

  xTaskCreatePinnedToCore(&machine_lifecycle ,"machine_lifecycle" ,1024*4 ,NULL ,5 ,NULL ,1);

  /*DEBUG*/ESP_LOGI(TAG, "~fw_run()");
}

#endif
