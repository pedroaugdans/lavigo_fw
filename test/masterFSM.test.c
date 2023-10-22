#include "hubware.h"

#ifdef __TESTING_MASTERFSM__

#include "lavigoMasterFSM.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "drv_nvs.h"
#include "drv_i2c.h"
#include "drv_uart.h"
#include "drv_wifi.h"
#include "drv_mqtt.h"
#include "drv_http.h"

#include "launcher.h"
#include "registration.h"
#include "connection.h"
#include "validation.h"
#include "update.h"
#include "monitoring.h"
#include "deployment.h"
#include "execution.h"
#include "drv_console.h"
#include "drv_spiffs.h"
#include "drv_gpio.h"
#include "params.h"
#include "pin_xio.h"
#include "pin_evt.h"
#include "drv_gpio.h"
#include "drv_ap.h"
#include "indication.h"
#include "fallback.h"
#include "clk_pit.h"
#include "port_xio.h"
#include "port_xio.h"

static const char *TAG = "[MASTER-FSM]";

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
    ESP_LOGI(TAG, "hw_init()");

    drv_nvs_init();
    drv_i2c_init();
    drv_uart_init();
    drv_wifi_init();
    drv_init_http();
    drv_gpio_init();

    ESP_LOGI(TAG, "~hw_init()");
}

void sw_init(void) {
    ESP_LOGI(TAG, "sw_init()");
    params_init();
    machines_init();
    pin_xio_init();
    port_xio_init();
    pin_evt_init();
    pin_seq_init();
    msg_fmt_init();

    xTaskCreatePinnedToCore(&machines_update_tsk, "machines_update", 1024 * 3, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(&port_xio_task, "port_xio_task", 1024 * 2, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(&drv_fallback_task, "fallback_task", 512*5, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&gpio_task, "gpio_task", 512 * 5, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&indication_task, "indication", 1024 * 3, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&drv_ap_task, "drv_ap", 1024 * 2, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(&drv_mqtt_task, "drv_mqtt_task", 512*7 , NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&msg_fmt_o_task, "msg_fmt_o_task", 1024 * 3, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&msg_fmt_i_task, "msg_fmt_i_task", 1024 * 3, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&launcher_task, "launcher_task", 512*5 , NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&registration_task, "registration_task", 512 * 7, NULL, 5, NULL, 1); // Requires >= 1024*3
    xTaskCreatePinnedToCore(&connection_task, "connection_task", 1024 * 5, NULL, 5, NULL, 1); // Requires >= 1024*3
    xTaskCreatePinnedToCore(&validation_task, "validation_task", 1024 * 3, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&update_task, "update task", 1024 * 16, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&monitoring_task, "monitoring_task", 1024 * 3, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(&deployment_task, "deployment_task", 1024 * 3, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(&execution_task, "execution_task", 1024 * 5, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&provisory_console_task, "uConsole_task", 512*7, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(&clk_pit_task, "clk_pit_task", 512*5, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&pin_in_task, "pin_in_task", 1024 * 4, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&pin_out_task, "pin_out_task", 1024 * 4, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&pin_evt_task, "pin_evt_task", 512*5, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&pin_seq_task, "pin_seq_task", 1024 * 3, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&execution_msg_q, "exec_msg_task", 512 * 5, NULL, 3, NULL, 1);
    ESP_LOGI(TAG, "~sw_init()");
}

void fw_run(void) {
    ESP_LOGI(TAG, "fw_run()");

    //xTaskCreatePinnedToCore(&queueRandomEvents, "FSMEVTS", 9216, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&fsm_run, "app_task", 512*5, NULL, 5, NULL, 1);

    ESP_LOGI(TAG, "~fw_run()");
}

void fsm_run(void *params) {
    runMasterFSM();
}

void queueRandomEvents();
void queueSequentialEvents();

masterFSM_events sequentialEvents[] = {};

void queueRandomEvents() {
    srand(xTaskGetTickCount());

    while (1) {
        int nextRandomEvent = rand() % total_Mevent;

        ESP_LOGE(TAG, "Setting next event as % d", nextRandomEvent);
        fsm_q_evt((masterFSM_events) nextRandomEvent);
        vTaskDelay(5000 / portTICK_RATE_MS);
    }
}

void queueSequentialEvents() {
    srand(xTaskGetTickCount());

    while (1) {
        int nextRandomEvent = rand() % total_Mevent;

        ESP_LOGE(TAG, "Setting next event as % d", nextRandomEvent);
        fsm_q_evt((masterFSM_events) nextRandomEvent);
        vTaskDelay(5000 / portTICK_RATE_MS);
    }
}

#endif
