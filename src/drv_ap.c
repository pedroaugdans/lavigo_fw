/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */


#include "msg_fmt.h"
#include "messages.h"
#include "registration.h"
#include "lavigoMasterFSM.h"
#include "execution.h"
#include "params.h"
#include "string.h"
#include "drv_wifi.h"
#include "drv_ap.h"
#include "hub_error.h"
#include "drv_nvs.h"
#include "drv_http.h"
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <sys/param.h>
#include "esp_netif.h"
#include <esp_http_server.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "machines.h"
#include "params.h"
#include "drv_gpio.h"
#include "lavigoMasterFSM.h"
#include "ap_interphase.h"


#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define FOREVER while (1)

/*** variables ************************************************************************/

static const char * TAG = "AP-MODE";
//static const char index_html[] = "<!DOCTYPE HTML><html><head>  <title>ESP Input Form</title>  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">  </head><body>  <form action=\"/get\">    input1: <input type=\"text\" name=\"input1\">    <input type=\"submit\" value=\"Submit\"> </form><br>  <form action=\"/get\">    input2: <input type=\"text\" name=\"input2\">    <input type=\"submit\" value=\"Submit\">  </form><br>  <form action=\"/get\">    input3: <input type=\"text\" name=\"input3\">    <input type=\"submit\" value=\"Submit\">  </form></body></html>";


/*** declarations ************************************************************************/
static void AP_dispatcher(void);

static void task_run(void);
static void task_idle(void);
static bool task_disengage(void);
static bool task_isDisengage(void);
static bool task_isEngage(void);
static bool task_isRun(void);
static bool task_engage(void);
static void task_init(void);
static void task_run_offline(void);
static bool task_isConnected(void);
static void task_engagement_done(void);
static void task_disengagement_done(void);
static void task_sleep(uint8_t ticks);

/*** DEFINITIONS ************************************************************************/

void drv_ap_task(void *pvParameters) {
    task_init();
    FOREVER{
        if (task_isEngage()) {
            bool check_running_flag = task_engage();
            if (check_running_flag) {
                task_engagement_done();
            }

        } else if (task_isRun()) {
            if (task_isConnected()) {
                task_run();
            } else {
                task_run_offline();
            }

        } else if (task_isDisengage()) {
            bool uncheck_running_flag = task_disengage();
            if (uncheck_running_flag) {
                task_disengagement_done();
            }

        } else {
            task_idle();
        }
        task_sleep(5);}
}

static void task_engagement_done(void) {
    engage_activation_flags[AP_engage_flag] = 0;
}

static void task_disengagement_done(void) {
    disengage_activation_flags[AP_disengage_flag] = 0;
}

static bool task_isConnected(void) {
    return hub_isConnected;
    //return FALSE;
}

static void task_run_offline(void) {
    //SP_LOGI(TAG, "Running ofline()");
}
static char main_page[128] = {0};

static bool task_engage(void) {
    hub_error_t err;
    err = drv_nvs_get(SERVER_STORAGE, main_page);
    if (err != SUCCESS) {
        ESP_LOGE(TAG, "Failed loading web page");
    }
    set_ram_usage(uxTaskGetStackHighWaterMark(NULL), ap_mode_task);

    run_confirmation_flag[AP_current] = 1;
    return true;
}

static void task_run(void) {
    set_ram_usage(uxTaskGetStackHighWaterMark(NULL), ap_mode_task);
    AP_dispatcher();
}

static bool task_disengage(void) {

    run_confirmation_flag[AP_current] = 0;
    return true;
}

static bool task_isRun(void) {
    return run_activation_flags[AP_run_flag];
}

static bool task_isEngage(void) {
    return engage_activation_flags[AP_engage_flag];
}

static bool task_isDisengage(void) {
    return disengage_activation_flags[AP_disengage_flag];
}

static void task_idle(void) {
    task_sleep(5);
}

void task_sleep(uint8_t ticks) {
    /*DEBUG*///ESP_LOGI(TAG, "sleep()");
    set_ram_usage(uxTaskGetStackHighWaterMark(NULL), ap_mode_task);
    vTaskDelay(suspendedThreadDelay[update_flag] * ticks / portTICK_RATE_MS);

    /*DEBUG*///ESP_LOGI(TAG, "~sleep()");
}

static void AP_dispatcher(void) {
    AP_dispatcher();
}

void ap_mode_pisr(void) {
    fsm_q_evt(apSet_Mevent);
}
char * method_get = "/get";
char * method_main = "/";

bool check_input(char * input) {
    if (input == NULL) {
        ESP_LOGE(TAG, "Error, no new input");
        return FALSE;
    }
    if (strlen(input) < 5) {
        ESP_LOGE(TAG, "Error, key too short");
        return FALSE;
    }
    return TRUE;
}

void input1_cb(char * input) {
    ESP_LOGI(TAG, "Input 1 : [%s]", input);
    if (check_input(input)) {
        drv_nvs_set(APSSID_IDX, input);
    }
}

void input2_cb(char * input) {
    ESP_LOGI(TAG, "Input 2 : [%s]", input);
    if (check_input(input)) {
        drv_nvs_set(APPSWD_IDX, input);
    }
}

void input3_cb(char * input) {
    ESP_LOGI(TAG, "Input 3 : [%s]", input);
}

void task_init(void) {
    drv_gpio_install_cb(GPIO_AP_MOD, ap_mode_pisr, pattern_fixed);
    drv_http_register_key("input1", input1_cb);
    drv_http_register_key("input2", input2_cb);
    drv_http_register_key("input3", input3_cb);
}
