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


#ifdef __TESTING_EXECUTION__

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
#include "deployment.h"

#include "string.h"

static const char *TAG = "[TEST-EXECUTE]";

/*EXAMPLE OF DEPLOYMENT PAYLOAD*/
/*PROTOCOL: D, PULSE LENGTH: 10 SECONDS*/
/*RESOURCE NAME: 56*/
/*PLEASE CHANGE <hub-GIRK3XBJE2YA> FOR THE ACTUAL HUB ID*/
 static char * new_machine = "{\"hub\":\"hub-GIRK3XBJE2YA\",\"resource\":17,\"action\":\"init\",\"template\":{\"signals\":[{\"name\":\"S\",\"port\":0,\"direction\":0},{\"name\":\"A\",\"port\":0,\"direction\":1}],\"ports\":[{\"color\":\"orange\",\"retrofit\":1,\"letter\":\"A\"}],\"version\":1,\"actions\":[{\"name\":\"cycle\",\"pattern\":\"A:l|S:L|S:H\",\"idle\":1,\"enable\":1,\"target\":0,\"status\":\"running\"},{\"name\":\"reset\",\"pattern\":\"S:L\",\"target\":0,\"status\":\"success\"},{\"name\":\"deny\",\"pattern\":\"A:H\",\"target\":1,\"status\":\"success\"},{\"name\":\"allow\",\"pattern\":\"A:L\",\"target\":1,\"status\":\"success\"}],\"events\":[{\"name\":\"cycle\",\"pattern\":\"S:l|S:h|#:10|S:L|#:1\",\"idle\":1,\"enable\":1,\"target\":0,\"status\":\"success\"},{\"name\":\"cycle\",\"pattern\":\"S:l|S:h\",\"idle\":1,\"enable\":1,\"target\":1,\"status\":\"trigger\"}]}}";


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
    pin_evt_init();
    pin_seq_init();

    /*LAYOUT VERSION is necessary for any machine operation*/
    registration_set_layout_version(VERSION_0_5_0);
    set_layout_version(version_5_0);

    machines_load();
    machines_prepare();

    run_activation_flags[execute_run_flag] = 1;

    /*DEBUG*/ESP_LOGI(TAG, "~sw_init()");
}

static char * execution_msg = "{\r\n\"hub\": \"hub-LL54EHVPDFXO\",\r\n\"resource\": 17,\r\n\"action\": \"cycle\"\r\n}";

void fw_run(void) {

    msg_fmt_test_cb(Deployment,new_machine);
    TEST_deployment_init_cb();

    msg_fmt_test_cb(Execution, execution_msg);
    /*DEBUG*/ESP_LOGI(TAG, "fw_run()");
    xTaskCreatePinnedToCore(&execution_task, "execution_task", 1024 * 5, NULL, 5, NULL, 1);
    vTaskDelay(10000 / portTICK_RATE_MS);
    while (1) {
        TEST_execution_cb();
        vTaskDelay(10000 / portTICK_RATE_MS);
    }

    /*DEBUG*/ESP_LOGI(TAG, "~fw_run()");
}
#endif
