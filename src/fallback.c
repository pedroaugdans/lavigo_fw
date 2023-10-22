/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "fallback.h"

#include "msg_fmt.h"
#include "messages.h"
#include "registration.h"
#include "lavigoMasterFSM.h"
#include "execution.h"
#include "params.h"
#include "string.h"
#include "hub_error.h"
#include "machines.h"
#include "params.h"
#include "drv_gpio.h"
#include "lavigoMasterFSM.h"
#include "pin_xio.h"
#include "pin_evt.h"
#include "machines_mapping.h"
#include "monitoring.h"
#include "port_xio.h"

#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define FOREVER while (1)

/*** variables ************************************************************************/
static const char * TAG = "[FBACK-MODE]";

/*Port being read*/
static uint8_t copying_port = 0;
/*Port being updated*/
static uint8_t pasting_port = 0;
/*Status to update*/
static bool port_state = 0;
/*Port status 1-place buffer*/
static bool fallback_port_last_state[256] = {0};

/*** declarations ************************************************************************/
/*This function is for a manual fallback turn using the side button of the hub*/
void fallback_mode_pisr(void);

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
static void task_sleep(uint16_t ticks);
void fallback_update(void);

/*** DEFINITIONS ************************************************************************/
/***
 *     _____         _
 *    |_   _|       | |
 *      | | __ _ ___| | __
 *      | |/ _` / __| |/ /
 *      | | (_| \__ \   <
 *      \_/\__,_|___/_|\_\
 *
 *
 */

/*Standard main loop*/

void task_init(void) {
    drv_gpio_install_cb(GPIO_AP_MOD, fallback_mode_pisr, pattern_blink);
}

void drv_fallback_task(void *pvParameters) {
    task_init();
    FOREVER{
      /*Fallback needs no preparation. These functions just satisfy the launcher requirements*/
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
        task_sleep(1);
      }
}

static void task_engagement_done(void) {
    fallback_or_running = hub_fallback_mode;
    engage_activation_flags[fallback_engage_flag] = 0;
}

static void task_disengagement_done(void) {
    disengage_activation_flags[fallback_disengage_flag] = 0;
}

static bool task_isConnected(void) {
    return hub_isConnected;
}

static void task_run_offline(void) {
    fallback_update();
    vTaskDelay(40/portTICK_RATE_MS);
}

static bool task_engage(void) {
    run_confirmation_flag[fallback_current] = 1;
    return true;
}

static void task_run(void) {
    fallback_update();
}

static bool task_disengage(void) {
    ESP_LOGW(TAG, "Disengaging fallback task");
    run_confirmation_flag[fallback_current] = 0;
    return true;
}

static bool task_isRun(void) {
    if (run_activation_flags[fallback_run_flag]) {
        if (is_hardware_version_selected) {
            return TRUE;
        } else {
            ESP_LOGE(TAG, "Hardware version not selected");
            task_sleep(5);
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

static bool task_isEngage(void) {
    return engage_activation_flags[fallback_engage_flag];
}

static bool task_isDisengage(void) {
    return disengage_activation_flags[fallback_disengage_flag];
}

static void task_idle(void) {
    //ESP_LOGI(TAG, "idle()");
    task_sleep(500);
}

void task_sleep(uint16_t ticks) {
    vTaskDelay(suspendedThreadDelay[fallback_flag] * ticks / portTICK_RATE_MS);
}

/***
 *    ______          _                                                                  _
 *    | ___ \        | |                                                                | |
 *    | |_/ /__  _ __| |_    _ __ ___   __ _ _ __   __ _  __ _  ___ _ __ ___   ___ _ __ | |_
 *    |  __/ _ \| '__| __|  | '_ ` _ \ / _` | '_ \ / _` |/ _` |/ _ \ '_ ` _ \ / _ \ '_ \| __|
 *    | | | (_) | |  | |_   | | | | | | (_| | | | | (_| | (_| |  __/ | | | | |  __/ | | | |_
 *    \_|  \___/|_|   \__|  |_| |_| |_|\__,_|_| |_|\__,_|\__, |\___|_| |_| |_|\___|_| |_|\__|
 *                                                        __/ |
 *                                                       |___/
 */

void fallback_update(void) {
    for (uint8_t k = 0; k < 32; k++) {
      /*hub_machines_input_ports_list is a list assembled by port xio service, depending on the layout*/
      copying_port = hub_machines_input_ports_list[k];
      /*RESOURCE*/
      if(port_look_for_status(copying_port, !fallback_port_last_state[(uint8_t) copying_port]) == SUCCESS){
        pasting_port = hub_retrofit_output_ports_list[k];
        port_state = !fallback_port_last_state[(uint8_t) copying_port];
        port_set(pasting_port, port_state);
        fallback_port_last_state[(uint8_t) copying_port] = port_state;
        ESP_LOGI(TAG, "Copying [%d] from  [%d] [Resource] to [%d]",port_state ,copying_port, pasting_port);
      }

      /*Retrofit*/
      copying_port = hub_retrofit_input_ports_list[k];
      if(port_look_for_status(copying_port, !fallback_port_last_state[(uint8_t) copying_port]) == SUCCESS){
        pasting_port = hub_machines_output_ports_list[k];
        port_state = !fallback_port_last_state[(uint8_t) copying_port];
        port_set(pasting_port, port_state);
        fallback_port_last_state[(uint8_t) copying_port] = port_state;
        ESP_LOGI(TAG, "Copying [%d] from [%d] [Retrofit] to [%d]",port_state, copying_port, pasting_port);
      }
    }
}

void fallback_mode_pisr(void) {
    fsm_q_evt(trgfb_Mevent);
}
