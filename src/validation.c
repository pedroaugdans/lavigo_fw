
#include "validation.h"

/***
 *     /$$    /$$  /$$$$$$  /$$       /$$$$$$ /$$$$$$$   /$$$$$$  /$$$$$$$$ /$$$$$$  /$$$$$$  /$$   /$$
 *    | $$   | $$ /$$__  $$| $$      |_  $$_/| $$__  $$ /$$__  $$|__  $$__/|_  $$_/ /$$__  $$| $$$ | $$
 *    | $$   | $$| $$  \ $$| $$        | $$  | $$  \ $$| $$  \ $$   | $$     | $$  | $$  \ $$| $$$$| $$
 *    |  $$ / $$/| $$$$$$$$| $$        | $$  | $$  | $$| $$$$$$$$   | $$     | $$  | $$  | $$| $$ $$ $$
 *     \  $$ $$/ | $$__  $$| $$        | $$  | $$  | $$| $$__  $$   | $$     | $$  | $$  | $$| $$  $$$$
 *      \  $$$/  | $$  | $$| $$        | $$  | $$  | $$| $$  | $$   | $$     | $$  | $$  | $$| $$\  $$$
 *       \  $/   | $$  | $$| $$$$$$$$ /$$$$$$| $$$$$$$/| $$  | $$   | $$    /$$$$$$|  $$$$$$/| $$ \  $$
 *        \_/    |__/  |__/|________/|______/|_______/ |__/  |__/   |__/   |______/ \______/ |__/  \__/
 *
 *
 *
 */

#include "launcher.h"

#include "msg_fmt.h"
#include "messages.h"
#include "registration.h"
#include "params.h"
#include "drv_mqtt.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "string.h"

#include "cJSON.h"
#include "hub_error.h"
#include "registration.h"

#define FOREVER  while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00
#define MAX_VALIDATION_ATTEMPTS 10

/*** VARIABLES **************************************************************************/

static const char *TAG = "[VALIDATE]";
bool validation_startup = false;

/*** DECLARATIONS ***********************************************************************/

void validation_sleep(uint8_t ticks);

bool validation_isActive();
uint8_t validation_isPending();
uint8_t validation_isIncomplete();

void validation_success_cb(void);

void validation_publishEvent();
hub_error_t validation_syntax_check(cJSON * object);
hub_error_t validation_content_check(cJSON * object);
static void validation_attempt(void);
/*** DEFINITIONS ************************************************************************/

static bool validation_isSent = 0;

void validation_task(void *pvParameters) {
    registration_set_validation_callback(validation_success_cb);
    //msg_fmt_install(Validation, validation_success_cb);
    drv_mqtt_install(validation_failure_cb, Disconnected);
    FOREVER{
        if (validation_isActive()) {
            if (validation_isPending()) {
              validation_attempt();
              for(uint8_t k= 0; k< MAX_VALIDATION_ATTEMPTS; k++){
                if(hub_isConnected){
                  validation_publishEvent();
                  validation_isSent = FALSE;
                  break;
                }
                validation_sleep(1);
              }
              validation_isSent = FALSE;
            }
            validation_sleep(1);
        } else {
            validation_sleep(1);
        }
      }
}
static void validation_attempt(void){
  if(!validation_startup){
    ESP_LOGI(TAG, "Initializing raw msg receiver");
    ESP_LOGI(TAG, "Enabling All receivers...");
    msg_fmt_configure();
    validation_startup = true;
    validation_sleep(1);
  } else {
    msg_fmt_configure();
    ESP_LOGI(TAG, "Not repeating receivers...");
  }
  ESP_LOGI(TAG, "Sending validation message...");
  msg_fmt_load(Validation);
  msg_fmt_edit_str(Validation, (char *) msg_fields[hubIdField], (char *) params_get(HUBID_PARAM));
  msg_fmt_send(Validation);
  validation_isSent = TRUE;
}

bool validation_isActive() {
    return check_activation_flags[cloud_check_flag]
            && !hub_isConnected && drv_mqtt_check();
}

uint8_t check_validation_pending(void){
    return !validation_isPending();
}

uint8_t validation_isPending() {
    return !validation_isSent;
}

uint8_t validation_isIncomplete() {
    return !hub_isConnected;
}

void validation_publishEvent() {
    ESP_LOGI(TAG, "CHECK!");
    fsm_q_evt(vldsync_Mevent);
}

void TEST_validation_success_cb(void){
    validation_success_cb();
}

void validation_success_cb(void) {

    hub_isConnected = TRUE;

}

void validation_failure_cb(void) {
    ESP_LOGE(TAG, "validation failure.");
    hub_isOnline = FALSE;
    hub_isConnected = FALSE;
    validation_isSent = FALSE;
}

void validation_sleep(uint8_t ticks) {
    //ESP_LOGI(TAG, "Sleep()");
    set_ram_usage(uxTaskGetStackHighWaterMark(NULL), validation_task_idx);
    vTaskDelay(suspendedThreadDelay[validation_flag] * ticks / portTICK_RATE_MS);
}

hub_error_t validation_syntax_check(cJSON * object) {
    if (_check_jSONField(object,
            msg_fields[statusField]) != HUB_OK) {
        return HUB_ERROR_COMM_GEN;
    }
    if (check_ifString(object,
            msg_fields[statusField]) != HUB_OK) {
        return HUB_ERROR_COMM_GEN;
    }
    char * status_field = _get_jSONField(object,
            msg_fields[statusField])->valuestring;

    if (check_field_content(status_field, validation_status, TOTAL_VALIDATION_RESULTS) != HUB_OK) {
        return HUB_ERROR_COMM_MSG_NOJSON;
    }

    return HUB_OK;
}

hub_error_t validation_content_check(cJSON * object) {
    char * validation_received_status = _get_jSONField(object,
            msg_fields[statusField])->valuestring;

    if (!strcmp(validation_received_status, validation_status[validationSuccess])) {
        return HUB_OK;
    }

    return HUB_ERROR_COMM_MSG_NOJSON;
}
