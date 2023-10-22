/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
 /***
  *     /$$        /$$$$$$  /$$   /$$ /$$   /$$  /$$$$$$  /$$   /$$ /$$$$$$$$ /$$$$$$$
  *    | $$       /$$__  $$| $$  | $$| $$$ | $$ /$$__  $$| $$  | $$| $$_____/| $$__  $$
  *    | $$      | $$  \ $$| $$  | $$| $$$$| $$| $$  \__/| $$  | $$| $$      | $$  \ $$
  *    | $$      | $$$$$$$$| $$  | $$| $$ $$ $$| $$      | $$$$$$$$| $$$$$   | $$$$$$$/
  *    | $$      | $$__  $$| $$  | $$| $$  $$$$| $$      | $$__  $$| $$__/   | $$__  $$
  *    | $$      | $$  | $$| $$  | $$| $$\  $$$| $$    $$| $$  | $$| $$      | $$  \ $$
  *    | $$$$$$$$| $$  | $$|  $$$$$$/| $$ \  $$|  $$$$$$/| $$  | $$| $$$$$$$$| $$  | $$
  *    |________/|__/  |__/ \______/ |__/  \__/ \______/ |__/  |__/|________/|__/  |__/
  *
  *
  *
  */
#include "launcher.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hubware.h"
#include "params.h"
/*VARIABLES*****************************************************************/
static const char *TAG = "[MFSMLNCHR]";

desired_running_t desiredRunning = 0;
desired_running_t confirmed_running = 0;


/*DECLARATIONS**************************************************************/
static void updateCurrent();
uint8_t launch_engagement();
uint8_t launch_disengagement();
uint8_t launch_fallback(void);
uint8_t launch_idle(void);
uint8_t idle_set(void);

/*DEFINITIONS***************************************************************/
void setDesired(desired_running_t newDesired) {
    desiredRunning = newDesired;
}

desired_running_t getCurrent(void) {
    return confirmed_running;
}

void setCurrent(desired_running_t newCurrent) {
    confirmed_running = newCurrent;
}

desired_running_t getDesired(void) {
    return desiredRunning;
}

static void task_sleep(void) {
    set_ram_usage(uxTaskGetStackHighWaterMark(NULL), launcher_task_idx);
    vTaskDelay(suspendedThreadDelay[launcher_flag] / portTICK_RATE_MS);

}

void launcher_task(void *pvParameters) {
    updateCurrent();
    set_ram_usage(uxTaskGetStackHighWaterMark(NULL), launcher_task_idx);
    while (1) {
        if (run_activation_flags[launcher_run_flag]) {
            if (desiredRunning != confirmed_running) {
                switch (desiredRunning) {
                    case no_one_running:
                        launch_disengagement();
                        break;
                    case idle_mode:
                        launch_idle();
                        break;
                    case running_mode:
                        launch_engagement();
                        break;
                    case fallback_mode:
                        launch_fallback();
                        break;
                }
            } else {
                ESP_LOGI(TAG, "Synched!");
                task_sleep();
            }
            continue;
            if (desiredRunning > confirmed_running) {
                launch_engagement();
            } else if (desiredRunning < confirmed_running) {
                launch_disengagement();
            } else {
                ESP_LOGI(TAG, "Synched!");
                task_sleep();
            }
        } else {
            task_sleep();
        }
    }
}

static void updateCurrent() {
    confirmed_running = 0;

    for (uint8_t k = 0; k < total_runningFlags; k++) {
        confirmed_running |= (run_confirmation_flag[k] << k);
    }
    ESP_LOGI(TAG, "Got [%d] in confirmation", confirmed_running);

}

void launcher_running_states(void) {
    ESP_LOGI(TAG, "Got [%d] mon [%d] exec [%d] deploy [%d] cons [%d] fallb",
            run_confirmation_flag[monitor_current],
            run_confirmation_flag[execution_current],
            run_confirmation_flag[deployment_current],
            run_confirmation_flag[console_current],
            run_confirmation_flag[fallback_current]);
}

uint8_t launch_fallback(void) {
    while (!hub_isOnline && !hub_no_internet) {
        ESP_LOGI(TAG, "[FALLBACK]Waiting to launch...");
        vTaskDelay(suspendedThreadDelay[launcher_flag]*2 / portTICK_RATE_MS);
      }
    engage_activation_flags[monitor_engage_flag] = 1;
    engage_activation_flags[deploy_engage_flag] = 1;
    engage_activation_flags[console_engage_flag] = 1;
    disengage_activation_flags[execute_engage_flag] = 1;
    engage_activation_flags[fallback_engage_flag] = 1;

    while (desiredRunning != confirmed_running) {
        set_ram_usage(uxTaskGetStackHighWaterMark(NULL), launcher_task_idx);
        updateCurrent();
        launcher_running_states();
        vTaskDelay(suspendedThreadDelay[launcher_flag] / portTICK_RATE_MS);
    }
    if (desiredRunning == confirmed_running) {
        ESP_LOGI(TAG, "[FALLBACK]queing engaged event");

        engage_activation_flags[monitor_engage_flag] = 0;
        engage_activation_flags[deploy_engage_flag] = 0;
        engage_activation_flags[console_engage_flag] = 0;
        engage_activation_flags[fallback_engage_flag] = 0;
        disengage_activation_flags[execute_engage_flag] = 0;
        fsm_q_evt(fbset_Mevent);
    }
    return 0;
}

uint8_t launch_fallback_exit() {
    disengage_activation_flags[monitor_disengage_flag] = 1;
    disengage_activation_flags[deploy_disengage_flag] = 1;
    disengage_activation_flags[execute_disengage_flag] = 1;
    disengage_activation_flags[console_disengage_flag] = 1;
    disengage_activation_flags[console_disengage_flag] = 1;

    while (desiredRunning != confirmed_running) {
        updateCurrent();
        launcher_running_states();
        vTaskDelay(suspendedThreadDelay[launcher_flag] / portTICK_RATE_MS);
    }
    if (desiredRunning == confirmed_running) {
        ESP_LOGI(TAG, "[FALLBACK] queing disingaged event");
        disengage_activation_flags[monitor_disengage_flag] = 0;
        disengage_activation_flags[deploy_disengage_flag] = 0;
        disengage_activation_flags[execute_disengage_flag] = 0;
        disengage_activation_flags[console_disengage_flag] = 0;
        fsm_q_evt(diseng_Mevent);
    }
    return 0;
}

uint8_t launch_engagement() {
  while (!hub_isOnline && !hub_no_internet) {
      ESP_LOGI(TAG, "[RUNNING]Waiting to launch...");
      vTaskDelay(suspendedThreadDelay[launcher_flag]*2 / portTICK_RATE_MS);
  }
    engage_activation_flags[monitor_engage_flag] = 1;
    engage_activation_flags[deploy_engage_flag] = 1;
    engage_activation_flags[execute_engage_flag] = 1;
    engage_activation_flags[console_engage_flag] = 1;
    disengage_activation_flags[fallback_disengage_flag] = 1;
    while (desiredRunning != confirmed_running) {
        set_ram_usage(uxTaskGetStackHighWaterMark(NULL), launcher_task_idx);
        updateCurrent();
        launcher_running_states();
        vTaskDelay(suspendedThreadDelay[launcher_flag] / portTICK_RATE_MS);
    }
    if (desiredRunning == confirmed_running) {
        ESP_LOGI(TAG, "[RUNNING]queing engaged event");

        engage_activation_flags[monitor_engage_flag] = 0;
        engage_activation_flags[deploy_engage_flag] = 0;
        engage_activation_flags[execute_engage_flag] = 0;
        engage_activation_flags[console_engage_flag] = 0;
        disengage_activation_flags[fallback_disengage_flag] = 0;
        fsm_q_evt(engaged_Mevent);
    }
    return 0;
}

uint8_t launch_disengagement() {
    disengage_activation_flags[monitor_disengage_flag] = 1;
    disengage_activation_flags[deploy_disengage_flag] = 1;
    disengage_activation_flags[execute_disengage_flag] = 1;
    disengage_activation_flags[console_disengage_flag] = 1;
    disengage_activation_flags[fallback_disengage_flag] = 1;

    while (desiredRunning != confirmed_running) {
        updateCurrent();
        launcher_running_states();
        vTaskDelay(suspendedThreadDelay[launcher_flag] / portTICK_RATE_MS);
    }
    if (desiredRunning == confirmed_running) {
        ESP_LOGI(TAG, "[RUNNING] queing disingaged event");
        disengage_activation_flags[monitor_disengage_flag] = 0;
        disengage_activation_flags[deploy_disengage_flag] = 0;
        disengage_activation_flags[execute_disengage_flag] = 0;
        disengage_activation_flags[console_disengage_flag] = 0;
        disengage_activation_flags[fallback_disengage_flag] = 0;
        fsm_q_evt(diseng_Mevent);
    }
    return 0;
}

uint8_t launch_update_engagement() {
    disengage_activation_flags[monitor_engage_flag] = 1;
    disengage_activation_flags[deploy_engage_flag] = 1;
    disengage_activation_flags[execute_engage_flag] = 1;
    disengage_activation_flags[console_engage_flag] = 1;
    while (desiredRunning < confirmed_running) {
        updateCurrent();
        launcher_running_states();
        vTaskDelay(suspendedThreadDelay[launcher_flag] / portTICK_RATE_MS);
    }
    if (desiredRunning == confirmed_running) {
        ESP_LOGI(TAG, "[UPDATE] queing disingaged event");

        disengage_activation_flags[monitor_engage_flag] = 0;
        disengage_activation_flags[deploy_engage_flag] = 0;
        disengage_activation_flags[execute_engage_flag] = 0;
        disengage_activation_flags[console_engage_flag] = 0;
    }
    return 0;
}

uint8_t launch_update(void) {
    run_confirmation_flag[update_current] = 1;
    return 0;
}

uint8_t stop_update(void) {
    run_confirmation_flag[update_current] = 0;
    return 0;
}

uint8_t launch_idle(void) {
    engage_activation_flags[monitor_engage_flag] = 1;
    engage_activation_flags[console_engage_flag] = 1;
    while (desiredRunning != confirmed_running) {
        set_ram_usage(uxTaskGetStackHighWaterMark(NULL), launcher_task_idx);
        updateCurrent();
        launcher_running_states();
        vTaskDelay(suspendedThreadDelay[launcher_flag] / portTICK_RATE_MS);
    }
    if (desiredRunning == confirmed_running) {
        ESP_LOGI(TAG, "[IDLE] queing engaged event");
        while (!hub_isOnline && !hub_no_internet) {
            ESP_LOGI(TAG, "[IDLE]Waiting to launch...");
            vTaskDelay(suspendedThreadDelay[launcher_flag]*2 / portTICK_RATE_MS);
        }
        engage_activation_flags[monitor_engage_flag] = 0;
        engage_activation_flags[console_engage_flag] = 0;
        fsm_q_evt(idset_Mevent);
    }
    return 0;
}

uint8_t idle_set(void) {

    return 0;
}
