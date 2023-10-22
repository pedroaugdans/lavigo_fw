#include "update.h"

/***
 *     /$$   /$$ /$$$$$$$  /$$$$$$$   /$$$$$$  /$$$$$$$$ /$$$$$$$$
 *    | $$  | $$| $$__  $$| $$__  $$ /$$__  $$|__  $$__/| $$_____/
 *    | $$  | $$| $$  \ $$| $$  \ $$| $$  \ $$   | $$   | $$
 *    | $$  | $$| $$$$$$$/| $$  | $$| $$$$$$$$   | $$   | $$$$$
 *    | $$  | $$| $$____/ | $$  | $$| $$__  $$   | $$   | $$__/
 *    | $$  | $$| $$      | $$  | $$| $$  | $$   | $$   | $$
 *    |  $$$$$$/| $$      | $$$$$$$/| $$  | $$   | $$   | $$$$$$$$
 *     \______/ |__/      |_______/ |__/  |__/   |__/   |________/
 *
 *
 *
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>
#include "esp_spi_flash.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"

#include "drv_wifi.h"
#include "drv_nvs.h"
#include "msg_fmt.h"
#include "messages.h"
#include "registration.h"
#include "lavigoMasterFSM.h"
#include "launcher.h"
#include "params.h"

#define FOREVER  while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define TO_HUB   1
#define FROM_HUB 0

#define UPDATE_INVALID 0
#define UPDATE_PENDING 1
#define UPDATE_SUCCESS 2
#define UPDATE_FAILURE 3

#define UPDATE_PERIOD 60

#define OTARST_IDX "otarst"

/*** VARIABLES **************************************************************************/

static const char *TAG = "[OTA]";

static char update_url[256] = "\0";

static char loaded_cert_pem[2000] = {0};

bool update_reset = false;

/*** DECLARATIONS ***********************************************************************/

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
 /*Default sleep ticks (miliseconds) task*/
void update_sleep(uint8_t ticks);
/*Active update tasks means that the task would react upon a message targetted to it*/
bool update_isActive(void);
/*Checks MQTT / Platform connectivity*/
bool update_isConnected(void);
/*Check if last message received made the necessary configurations for required operation successfully*/
bool update_isReady(void);
/*When finished update, a reboot to start on new firmware is required*/
bool update_isReboot(void);

/*Upon receiving correct information for new configuration, and finish parsing, the update service status will be set ready for next step*/
void update_setReady(bool ready);
/*Upon finishing HTTP downlading of new firmware, an update from this service will be requested*/
void update_setReboot(bool reboot);
/*Check keys and firmware version*/
bool update_init(void);
/*function called for an update "init" command, which should trigger a firmware update*/
bool update_trigger(char *url);
/*Function called to assemble and stablish http connection to a previously provided URL, to download new firmware*/
bool update_fetch(void);
/*soft reset device*/
bool update_reboot(void * arg);

/***
 *                                                                 _
 *                                                                | |
 *     _ __ ___   __ _ _ __   __ _  __ _  ___ _ __ ___   ___ _ __ | |_
 *    | '_ ` _ \ / _` | '_ \ / _` |/ _` |/ _ \ '_ ` _ \ / _ \ '_ \| __|
 *    | | | | | | (_| | | | | (_| | (_| |  __/ | | | | |  __/ | | | |_
 *    |_| |_| |_|\__,_|_| |_|\__,_|\__, |\___|_| |_| |_|\___|_| |_|\__|
 *                                  __/ |
 *                                 |___/
 */
 /*Message subscription for command reception on msg_fmt*/
void update_subscribe(void);
/*Update command dispatcher*/
void update_success_cb(void);
/*Deprecated communication messages*/
void update_invalid(void);
void update_pending(void);
void update_success(void);
void update_failure(void);

/*Offline handler, pointless for this service*/
static void task_run_offline(void);

esp_err_t _event_handler(esp_http_client_event_t *evt);

/*** DEFINITIONS ************************************************************************/

void update_task(void *pvParameters) {
    /*DEBUG*/ESP_LOGI(TAG, "task()");

    update_init();

    while (!update_isActive()) { // Whether the task has been enabled for the first time
        update_sleep(1); // Wait for one (1) second and continue
    }

    update_subscribe(); // Subscribe to "url" MQTT message from /hubs

    FOREVER{
        if (update_isActive()) { // Whether the task is enabled by the FSM
            if (update_isConnected()) { // Whether there is a valid internet (IP) connection
                if (update_isReboot()) { // Whether the app just rebooted from an UPDATE update
                    update_success(); // Send a "success" MQTT message to /hubs
                    update_setReboot(false); // Turn off "reboot" flag to close the reboot sequence
                }

                if (update_isReady()) { // Whether there is a valid url to launch an UPDATE update
                    if (update_fetch()) { // Fetch firmware update
                        update_pending(); // Send a "pending" MQTT message to /hubs
                        update_setReady(false); // Remove url to avoid repeating update
                        update_setReboot(true); // Set "reboot" flag to initiate the reboot sequence
                        update_sleep(3); // Wait for three (3) seconds and continue
                        update_reboot(NULL); // WARNING: rebooting ESP32!
                    } else { // If fetching firmware update failed
                        update_failure(); // Send a "failure" MQTT message to /hubs
                        update_setReady(false); // Remove url to avoid repeating update
                        update_reboot(NULL);
                    }
                } else { // If there is not valid url
                    update_sleep(1); // Wait for one (1) second and continue
                }
            } else {
                task_run_offline(); // If there is not internet connection
                update_sleep(1); // Wait for one (1) second and continue
            }
        } else { // If the task is disabled byt the FSM
            update_sleep(5); // Wait for one (1) second and continue
        }}

    /*DEBUG*/ESP_LOGI(TAG, "~task()");
}

void update_sleep(uint8_t ticks) {
    /*DEBUG*///ESP_LOGI(TAG, "sleep()");

    vTaskDelay(suspendedThreadDelay[update_flag] * ticks / portTICK_RATE_MS);

    /*DEBUG*///ESP_LOGI(TAG, "~sleep()");
}

bool update_isActive(void) {
    /*DEBUG*///ESP_LOGI(TAG, "isActive()");

    return check_activation_flags[update_check_flag];
}

bool update_isConnected(void) {
    ///*DEBUG*/ESP_LOGI(TAG, "isOnline()");

    return hub_isConnected;
}

bool update_isReady(void) {
    ///*DEBUG*/ESP_LOGI(TAG, "isReady()");

    return (update_url[0] != '\0') && run_activation_flags[update_run_flag];
}

bool update_isReboot(void) {
    ///*DEBUG*/ESP_LOGI(TAG, "isReboot()");

    return update_reset;
}

void update_setReady(bool ready) {
    /*DEBUG*/ESP_LOGI(TAG, "setReady()");

    if (!ready) {
        update_url[0] = '\0';
    }

    /*DEBUG*/ESP_LOGI(TAG, "~setReady()");
}

void update_setReboot(bool reboot) {
    /*DEBUG*/ESP_LOGI(TAG, "setReboot()");

    if (reboot) {
        drv_nvs_set(OTARST_IDX, "reboot");
    } else {
        drv_nvs_set(OTARST_IDX, "noop");
    }

    char otarst[6] = "";

    drv_nvs_get(OTARST_IDX, otarst);

    update_reset = !strcmp(otarst, "reboot");

    /*DEBUG*/ESP_LOGI(TAG, "~setReboot()");
}

static void task_run_offline(void) {
    //ESP_LOGI(TAG, "LOL");
}

uint8_t read_firmware_version(void) {
    const esp_partition_t *running = esp_ota_get_running_partition();

    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
        strlcpy(firmware_version,running_app_info.version,strlen(running_app_info.version) + 1);
        data_to_report[firmware_data] = TRUE;
        set_hub_report();
    } else {
        ESP_LOGE(TAG, "Could not fetch fw version");
    }
    return 0;
}

bool update_init(void) {
    /*DEBUG*/ESP_LOGI(TAG, "init()");

    read_firmware_version();
    char otarst[6] = "";
    drv_nvs_get(UPDKEY_IDX, loaded_cert_pem);
    drv_nvs_get(OTARST_IDX, otarst);
    update_reset = !strcmp(otarst, "reboot");

    /*DEBUG*/ESP_LOGI(TAG, "~init()");

    return update_reset;
}

void TEST_update(char * url){
  update_trigger(url);
}

bool update_trigger(char *url) {
    /*DEBUG*/ESP_LOGI(TAG, "trigger()");

    strcpy(update_url, url);

    update_setReady(true);

    /*DEBUG*/ESP_LOGI(TAG, "~trigger()");

    return true;
}

void force_reboot(void * arg) {
    /*DEBUG*/ESP_LOGI(TAG, "FORCE reboot()");

    esp_restart();

    /*DEBUG*/ESP_LOGI(TAG, "~reboot()");

}

esp_timer_handle_t hub_update_timer_handler;

bool update_fetch(void) {
    /*DEBUG*/ESP_LOGI(TAG, "update()");

    if (!loaded_cert_pem[0]) {
        drv_nvs_get(UPDKEY_IDX, loaded_cert_pem);
    }
    esp_http_client_config_t config = {
        .url = (char *) update_url,
        //.cert_pem = (char *)_cert_pem,
        .cert_pem = (char *) loaded_cert_pem,
        .event_handler = _event_handler,
        .timeout_ms = 10000,
    };

    esp_timer_create_args_t oneshot_timer_args = {
        .callback = &force_reboot,
        /* argument specified here will be passed to timer callback function */
        .arg = (void*) NULL,
        .name = "one-shot"
    };
    esp_timer_create(&oneshot_timer_args, &hub_update_timer_handler);
    esp_timer_start_once(hub_update_timer_handler,120000000);
    ESP_LOGW(TAG,"Timer triggered");

    esp_err_t ret = esp_https_ota(&config);

    ESP_LOGI(TAG, "OTA STATUS %d", ret);

    /*DEBUG*/ESP_LOGI(TAG, "~update()");

    return (ret == ESP_OK);
}



bool update_reboot(void * arg) {
    /*DEBUG*/ESP_LOGI(TAG, "reboot()");

    esp_restart();

    /*DEBUG*/ESP_LOGI(TAG, "~reboot()");

    return true;
}

void update_subscribe(void) {
    /*DEBUG*/ESP_LOGI(TAG, "subscribe()");

    msg_fmt_install(Update, update_success_cb);

    /*DEBUG*/ESP_LOGI(TAG, "~subscribe()");
}

void update_invalid(void) {
    /*DEBUG*/ESP_LOGI(TAG, "invalid()");

    //msg_fmt_load(Update);
    //msg_fmt_edit_str(Update, (char *) msg_fields[hubIdField], (char *) params_get(HUBID_PARAM));
    //msg_fmt_edit_str(Update, (char *) msg_fields[statusField], updateStatus[UPDATE_INVALID]);
    //msg_fmt_send(Update);

    /*DEBUG*/ESP_LOGI(TAG, "~invalid()");
}

void update_pending(void) {
    /*DEBUG*/ESP_LOGI(TAG, "pending()");

    //msg_fmt_load(Update);
    //msg_fmt_edit_str(Update, (char *) msg_fields[hubIdField], (char *) params_get(HUBID_PARAM));
    //msg_fmt_edit_str(Update, (char *) msg_fields[statusField], updateStatus[UPDATE_PENDING]);
    //msg_fmt_send(Update);

    /*DEBUG*/ESP_LOGI(TAG, "~pending()");
}

void update_success(void) {
    /*DEBUG*/ESP_LOGI(TAG, "success()");

    //msg_fmt_load(Update);
    //msg_fmt_edit_str(Update, (char *) msg_fields[hubIdField], (char *) params_get(HUBID_PARAM));
    //msg_fmt_edit_str(Update, (char *) msg_fields[statusField], updateStatus[UPDATE_SUCCESS]);
    //msg_fmt_send(Update);

    /*DEBUG*/ESP_LOGI(TAG, "~success()");
}

void update_failure(void) {
    /*DEBUG*/ESP_LOGI(TAG, "failure()");

    //msg_fmt_load(Update);
    //msg_fmt_edit_str(Update, (char *) msg_fields[hubIdField], (char *) params_get(HUBID_PARAM));
    //msg_fmt_edit_str(Update, (char *) msg_fields[statusField], updateStatus[UPDATE_FAILURE]);
    //msg_fmt_send(Update);

    /*DEBUG*/ESP_LOGI(TAG, "~failure()");
}

void update_success_cb(void) {
    /*DEBUG*/ESP_LOGI(TAG, "update_success_cb()");
    cJSON *root = cJSON_Parse(msg_fmt_message_static(Update));
    char *action = cJSON_GetObjectItemCaseSensitive(
            root,
            "action"
            )->valuestring; // Get action

    char *url = cJSON_GetObjectItemCaseSensitive(
            root,
            "url"
            )->valuestring; // Get url

    if (!strcmp(action, "init")) { // Whether action is "update" as expected
        update_trigger(url); // Trigger update process
    } else {
        update_invalid(); // Send an "invalid" MQTT message to /hubs
    }

    ESP_LOGI(TAG, "%s", update_url);
    fsm_q_evt(trgup_Mevent);
    stash_jSON_msg(root);
    /*DEBUG*/ESP_LOGI(TAG, "~update_success_cb()");
}

esp_err_t _event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

uint8_t update_syntax_check(cJSON * object) {
    if (!cJSON_GetObjectItemCaseSensitive(object, "url")) {
        ESP_LOGE(TAG, "Received invalid update message! Missing: url.");
        stash_jSON_msg(object);
        return FAILURE;
    }

    if (!cJSON_GetObjectItemCaseSensitive(object, "action")) {
        ESP_LOGE(TAG, "Received invalid update message! Missing: action.");
        stash_jSON_msg(object);
        return FAILURE;
    }
    return SUCCESS;
}
