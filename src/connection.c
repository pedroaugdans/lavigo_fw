#include "connection.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "cJSON.h"

#include "drv_nvs.h"
#include "drv_uart.h"
#include "drv_wifi.h"
#include "drv_mqtt.h"
#include "launcher.h"

#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "lwip/dns.h"
#include "ping/ping.h"
#include "esp_ping.h"
#include "ping/ping_sock.h"

#include "sdkconfig.h"
#include "esp_log.h"
#include "argtable3/argtable3.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "mdns.h"
#include "hub_error.h"
#include "params.h"

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define APSSID_CONFIG_MIN_LENGTH 4
#define APPSWD_CONFIG_MIN_LENGTH 4

#define WIFI_MIN_LEN 4

#define CONFIG_LENGTH (APSSID_CONFIG_MIN_LENGTH *10)

/*** VARIABLES **************************************************************************/

static const char *TAG = "[CNNCT]";



char config[CONFIG_LENGTH + 6] = {0};
char *config_p = &config[0];

/*AP SSID information holder*/
char ssid[50] = {0};
/*AP PSWD information holder*/
char pswd[50] = {0};
/*This counter serves to avoid double stack initialization AND */
static uint32_t successful_connections = 0;
static bool isStageFInished = FALSE;

/*** DECLARATIONS ***********************************************************************/
/***
 *     _____         _
 *    |_   _|       | |
 *      | | __ _ ___| | __
 *      | |/ _` / __| |/ /
 *      | | (_| \__ \   <
 *      \_/\__,_|___/_|\_\
 */
/*Will trigger the task, if theres no connection AND task is enabled by masterFSM*/
bool connection_isActive();
/*Check if hub is ONLINE*/
uint8_t connection_isIncomplete();
/*Upon a successful connection, this function will be called to inform MASTER FSM that the hub is connected.
currently unused*/
void connection_publishEvent(bool state);
/*task sleep, ticks in millseconds*/
void connection_sleep(uint8_t ticks);
/*callback to be called from WIFI driver layer upon successful connection attempt*/
void connection_success_cb();
/*callback to be called from WIFI driver layer upon disconnection*/
void connection_reset_cb();
/*<Deprecated>*/
uint8_t connection_handleNextCommand();

/***
 *     _____                             _   _                                __ _
 *    /  __ \                           | | (_)                              / _(_)
 *    | /  \/ ___  _ __  _ __   ___  ___| |_ _  ___  _ __     ___ ___  _ __ | |_ _  __ _
 *    | |    / _ \| '_ \| '_ \ / _ \/ __| __| |/ _ \| '_ \   / __/ _ \| '_ \|  _| |/ _` |
 *    | \__/\ (_) | | | | | | |  __/ (__| |_| | (_) | | | | | (_| (_) | | | | | | | (_| |
 *     \____/\___/|_| |_|_| |_|\___|\___|\__|_|\___/|_| |_|  \___\___/|_| |_|_| |_|\__, |
 *                                                                                  __/ |
 *                                                                                 |___/
 */
 /*Get SSID or PSWD configuration from memory*/
char *connection_getConfig(uint32_t configMinLen, char *terminator);
/*<Deprecated>*/
void connection_printACK();
/*Internal re-load of SSID, from flash to RAM*/
void connection_getAPSSID();
/*Internal re-load of PSWD, from flash to RAM*/
void connection_getAPPSWD();
/*print tool to log FLASH information of SSID or PSWD*/
void connection_logConfig(const char *configId);
/*<DEPRECATED> now help command is handled from registration*/
void connection_logHelp();

/*<Deprecated>*/
uint8_t connection_trigger(void);
static void connection_ping_check()__attribute__ ((unused));
uint8_t connection_attempt(void);

/*** DEFINITIONS ************************************************************************/

void connection_reset(void){
  drv_mqtt_reset();
  drv_wifi_stop();
  successful_connections = 0;
}

void connection_recover(void){
  successful_connections = 0;
}

void connection_success_cb(void) {
    ESP_LOGI(TAG, "success_cb()");

    /*DEBUG*/ESP_LOGI(TAG, "~success_cb()");
}

void connection_reset_cb(void) {
    //ESP_LOGI(TAG, "reset_cb()");

    hub_isOnline = FALSE;

    ///*DEBUG*/ESP_LOGI(TAG, "~reset_cb()");
}

void connection_task(void *pvParameters) {
    drv_wifi_install(connection_success_cb, Online);
    drv_wifi_install(connection_reset_cb, Offline);
    uint8_t what_to_sleep = 10;

    FOREVER{
        if (connection_isActive()) {
            if(connection_attempt() == SUCCESS){
            for (int i = 0; i < MAX_CONNECTION_RETRIES; i++) {
                if(!drv_wifi_check()){
                  ESP_LOGE(TAG,"Disconnected! reconnecting");
                  break;
                }
                if (drv_mqtt_check()) {
                    ESP_LOGI(TAG, "Done.");
                    ESP_LOGI(TAG, "Online.");
                    hub_isOnline = TRUE;
                    what_to_sleep = 50;
                    connection_publishEvent(TRUE);
                    break;
                  } else {
                    set_ram_usage(uxTaskGetStackHighWaterMark(NULL), connection_task_idx);
                    ESP_LOGI(TAG, "Waiting...");
                    connection_sleep(1);
                  }
              }
            } else {
              hub_isOnline = FALSE;
              hub_no_internet = TRUE;
              if(successful_connections>16000){
                  ESP_LOGW(TAG,"Exceeded 16 000 reconnections");
              } else {
                  what_to_sleep=10;
              }

              ESP_LOGI(TAG, "Retrying later.");
              connection_sleep(what_to_sleep);
            }
        } else {
            connection_sleep(what_to_sleep);
        }
        connection_sleep(1);
      }
}

void connection_engage_ap(void) {
    hub_error_t err = drv_wifi_stop();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[ENGANGE AP] Couldnt stop");
    }
    err = drv_wifi_AP();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[ENGANGE AP] Couldnt AP");
    }
}

void connection_re_engage_sta(void) {
    hub_error_t err = drv_wifi_stop();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[DISENG AP] Couldnt stop");
    }
    hub_isOnline = FALSE;
}

bool connection_isActive() {
    return check_activation_flags[internet_check_flag] && !hub_isOnline;
}

void connection_printConfig(char *buffer) {
    /*DEBUG*/ESP_LOGI(TAG, "printConfig()");

    for (unsigned int j = 0; buffer[j] != 00; j++) {
        printf("%c", (char) buffer[j]);
    }
    printf("\n");

    for (unsigned int j = 0; buffer[j] != 00; j++) {
        printf("-");
    }
    printf("\n");

    /*DEBUG*/ESP_LOGI(TAG, "~printConfig()");
}

void connection_getAPSSID() {
    ///*DEBUG*/ESP_LOGI(TAG, "getAPSSID()");

    drv_nvs_get(APSSID_IDX, ssid);
    ESP_LOGI(TAG, "Saved AP SSID: %s", ssid);

    ///*DEBUG*/ESP_LOGI(TAG, "~getAPSSID()");
}

void connection_getAPPSWD() {
    ///*DEBUG*/ESP_LOGI(TAG, "getAPPSWD()");

    drv_nvs_get(APPSWD_IDX, pswd);
    ESP_LOGI(TAG, "Saved AP PSWD: %s", pswd);

    ///*DEBUG*/ESP_LOGI(TAG, "~getAPPSWD()");
}

uint8_t connection_update() {
    ///*DEBUG*/ESP_LOGI(TAG, "updateAP()");

    connection_getAPSSID();
    connection_getAPPSWD();

    /*DEBUG*/ESP_LOGI(TAG, "Configuring WIFI...");
    drv_wifi_configure(ssid, pswd);

    if (drv_wifi_connect() != HUB_OK) {
        drv_wifi_disconnect();
        /*DEBUG*/ESP_LOGE(TAG, "NOT CONNECTED TO WIFI");
        return FAILURE;
    }
    if (drv_mqtt_configure() != SUCCESS) {
        drv_wifi_disconnect();
        /*DEBUG*/ESP_LOGE(TAG, "MQTT NOT CONFIGURED");
        return FAILURE;
    }

    if (drv_mqtt_connect() != SUCCESS) {
        drv_wifi_disconnect();
        /*DEBUG*/ESP_LOGE(TAG, "MQTT NOT CONNECTED");
        return FAILURE;
    }
    return SUCCESS;
    ///*DEBUG*/ESP_LOGI(TAG, "~updateAP()");
}

uint8_t connection_retry() {
    ///*DEBUG*/ESP_LOGI(TAG, "updateAP()");

    connection_getAPSSID();
    connection_getAPPSWD();

    /*DEBUG*/ESP_LOGI(TAG, "Configuring WIFI...");
    drv_wifi_configure(ssid, pswd);

    if (drv_wifi_connect() != HUB_OK) {
        drv_wifi_disconnect();
        /*DEBUG*/ESP_LOGE(TAG, "NOT CONNECTED TO WIFI");
        return FAILURE;
    }

    if (drv_mqtt_reconnect() != SUCCESS) {
        drv_wifi_disconnect();
        /*DEBUG*/ESP_LOGE(TAG, "MQTT NOT CONNECTED");
        return FAILURE;
    }
    return SUCCESS;
    ///*DEBUG*/ESP_LOGI(TAG, "~updateAP()");
}

uint8_t connection_attempt(void) {
    set_ram_usage(uxTaskGetStackHighWaterMark(NULL), connection_task_idx);
    if (successful_connections == 0) {
        ESP_LOGI(TAG, "Attempting first connection");
        successful_connections++;
        return connection_update();
    } else {
        ESP_LOGI(TAG, "Attempting connection [%d]", successful_connections);
        return connection_retry();
    }
}

uint8_t connection_trigger(void) {
    if (drv_mqtt_check()) {

        return SUCCESS;
    } else {
        return FAILURE;
    }
}

void connection_publishEvent(bool state) {
    /*DEBUG*/ESP_LOGI(TAG, "publishEvent()");

    if (state == TRUE) {
        isStageFInished = TRUE;
        fsm_q_evt(cnnctd_Mevent);
    } else {
        fsm_q_evt(cnter_Mevent);
    }

    /*DEBUG*/ESP_LOGI(TAG, "~publishEvent()");
}

void connection_sleep(uint8_t ticks) {
    set_ram_usage(uxTaskGetStackHighWaterMark(NULL), connection_task_idx);
    if (isStageFInished) {
        vTaskDelay(suspendedThreadDelay[connection_flag] * ticks * 5/ portTICK_RATE_MS);
    } else {
        vTaskDelay(suspendedThreadDelay[connection_flag] * ticks / portTICK_RATE_MS);
    }
}

void connection_logHelp() {
    /*DEBUG*/ESP_LOGI(TAG, "logHelp()");

    ESP_LOGI(TAG, "Now on connection console, press 'q' for exit");

    /*DEBUG*/ESP_LOGI(TAG, "~logHelp()");
}

esp_err_t pingResults(ping_target_id_t msgType, esp_ping_found * pf) {
    ESP_LOGI(TAG, "[%d] remaining heap", uxTaskGetStackHighWaterMark(NULL));
    ESP_LOGI(TAG, "[%d] weird param", msgType);
    if ((pf->err_count == 0) && (pf->timeout_count == 0)) {
        ESP_LOGI(TAG, "Ping successful");
    } else {
        ESP_LOGE(TAG, "Ping unsuccessful");
    }
    return ESP_OK;
}

static void connection_ping_check() {
    uint32_t ping_count = 3, ping_timeout = CONNECTION_PING_TIMEOUT, ping_delay = 1000, ping_dir = ipaddr_addr("8.8.8.8");
    size_t data_len = 8;

    ESP_LOGI(TAG, "[%d] remaining heap", uxTaskGetStackHighWaterMark(NULL));

    esp_ping_set_target(PING_TARGET_IP_ADDRESS_COUNT, &ping_count, sizeof (uint32_t));
    esp_ping_set_target(PING_TARGET_RCV_TIMEO, &ping_timeout, sizeof (uint32_t));
    esp_ping_set_target(PING_TARGET_DELAY_TIME, &ping_delay, sizeof (uint32_t));
    esp_ping_set_target(PING_TARGET_IP_ADDRESS, &ping_dir, sizeof (uint32_t));
    esp_ping_set_target(PING_TARGET_RES_FN, &pingResults, sizeof (pingResults));
    esp_ping_set_target(PING_TARGET_DATA_LEN, &data_len, sizeof (size_t));
    //ping_init();
}
