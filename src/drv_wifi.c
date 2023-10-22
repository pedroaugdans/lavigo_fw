//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____   ____ _    __ ______ ____              _       __ ____ ______ ____  //
//    / __ \ / __ \ /  _/| |  / // ____// __ \            | |     / //  _// ____//  _/  //
//   / / / // /_/ / / /  | | / // __/  / /_/ /  ______    | | /| / / / / / /_    / /    //
//  / /_/ // _, _/_/ /   | |/ // /___ / _, _/  /_____/    | |/ |/ /_/ / / __/  _/ /     //
// /_____//_/ |_|/___/   |___//_____//_/ |_|              |__/|__//___//_/    /___/     //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "drv_wifi.h"

#include "esp_wifi.h"

#include "esp_err.h"
#include "freertos/event_groups.h"
#include "hub_error.h"
#include "string.h"
#include "esp_netif.h"

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define WIFI_WAIT_TIMEOUT  (3000 / portTICK_RATE_MS)
#define WIFI_CONNECTED_BIT BIT0

/*** VARIABLES **************************************************************************/

static const char *TAG = "[DRV-WIFI]";

static EventGroupHandle_t wifi_events;

wifi_config_t wifi_config_sta = {
    .sta =
    {
        .ssid = WIFI_DEFAULT_SSID,
        .password = WIFI_DEFAULT_PSWD
    }
};

wifi_config_t wifi_config_ap = {
    .ap =
    {
        .ssid = WIFI_DEFAULT_SSID,
        .ssid_len = strlen(WIFI_DEFAULT_SSID),
        .password = WIFI_DEFAULT_PSWD,
        .max_connection = MAX_STA_CONN,
        .authmode = WIFI_AUTH_WPA_WPA2_PSK
    },
};

static drv_wifi_callback_t wifi_callbacks[NOF_WIFI_EVENTS] = {0};
static server_callback_t server_callback = NULL;
static server_callback_t server_uncallback = NULL;
static uint8_t wifi_online;

static esp_netif_t *sta_netif = NULL;
/*** DECLARATIONS ***********************************************************************/
static void wifi_next(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

/*** DEFINITIONS ************************************************************************/

/***
 *                      __ _
 *                     / _(_)
 *      ___ ___  _ __ | |_ _  __ _
 *     / __/ _ \| '_ \|  _| |/ _` |
 *    | (_| (_) | | | | | | | (_| |
 *     \___\___/|_| |_|_| |_|\__, |
 *                            __/ |
 *                           |___/
 */

void drv_wifi_server_installl(drv_wifi_event_t callback, server_callback_t event) {
    if (callback == apDown) {
        server_uncallback = (server_callback_t) event;
    }
    if (callback == Apset) {
        server_callback = (server_callback_t) event;
    }
}

hub_error_t drv_ap_event_install(void * generic_arg) {
    hub_error_t err;
    err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_START, &wifi_next, generic_arg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[wifi_connect() - handle_register gotip] error [0x%X]", err);
        return err; // ERROR_GENERIC;
    }
    err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STOP, &wifi_next, generic_arg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[wifi_connect() - handle_register gotip] error [0x%X]", err);
        return err; // ERROR_GENERIC;
    }
    return ESP_OK;
}

void drv_wifi_configure(char *ssid, char *pswd) {
#ifndef WIFI_USE_DEFAULT
    uint8_t i;

    for (i = 0; ssid[i] != '\0'; i++) {
        wifi_config_sta.sta.ssid[i] = ssid[i];
    }

    wifi_config_sta.sta.ssid[i] = '\0';

    for (i = 0; pswd[i] != '\0'; i++) {
        wifi_config_sta.sta.password[i] = pswd[i];
    }

    wifi_config_sta.sta.password[i] = '\0';
#endif
}

/***
 *      ___
 *     / _ \
 *    / /_\ \_ __    ______   ___  ___ _ ____   _____ _ __
 *    |  _  | '_ \  |______| / __|/ _ \ '__\ \ / / _ \ '__|
 *    | | | | |_) |          \__ \  __/ |   \ V /  __/ |
 *    \_| |_/ .__/           |___/\___|_|    \_/ \___|_|
 *          | |
 *          |_|
 */

hub_error_t drv_wifi_AP(void) {
    esp_err_t err;

    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[wifi_connect() - setmod_mode] error [0x%X]", err);
        return HUB_ERROR_COMM_CONN_GEN;
    }
    err = esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config_ap);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[wifi_connect() - setconfig] error [0x%X]", err);
        return HUB_ERROR_COMM_CONN_GEN;
    }
    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[wifi_connect() - wifistart] error [0x%X]", err);
        return HUB_ERROR_COMM_CONN_GEN;
    }

    return HUB_OK;
}

void drv_wifi_install(drv_wifi_callback_t callback, drv_wifi_event_t event) {
    if (event < NOF_WIFI_EVENTS) {
        wifi_callbacks[event] = callback;
    }
}

/***
 *         _        _   _                                    _
 *        | |      | | (_)                                  | |
 *     ___| |_ __ _| |_ _  ___  _ __     _ __ ___   ___   __| | ___
 *    / __| __/ _` | __| |/ _ \| '_ \   | '_ ` _ \ / _ \ / _` |/ _ \
 *    \__ \ || (_| | |_| | (_) | | | |  | | | | | | (_) | (_| |  __/
 *    |___/\__\__,_|\__|_|\___/|_| |_|  |_| |_| |_|\___/ \__,_|\___|
 *
 *
 */

void drv_wifi_init(void) {
    /*DEBUG*/ESP_LOGI(TAG, "init()");

    ESP_ERROR_CHECK(esp_netif_init());
    wifi_events = xEventGroupCreate();
    esp_err_t err;
    //tcpip_adapter_init();
    err = esp_event_loop_create_default();
    sta_netif = esp_netif_create_default_wifi_sta();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[wifi_connect() - loop_create] error [0x%X]", err);
    }

    err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_next, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[wifi_connect() - handle_register wifi] error [0x%X]", err);
        return; // ERROR_GENERIC;
    }

    err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_next, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[wifi_connect() - handle_register wifi] error [0x%X]", err);
        return; // ERROR_GENERIC;
    }
    err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_next, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[wifi_connect() - handle_register gotip] error [0x%X]", err);
        return; // ERROR_GENERIC;
    }
    wifi_init_config_t drv_wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    //ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    err = esp_wifi_init(&drv_wifi_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[wifi_connect() - wifi_init] error [0x%X]", err);
        return; // ERROR_GENERIC;
    }
    wifi_online = FALSE;
    ///*DEBUG*/ESP_LOGI(TAG, "~init()");
}



hub_error_t drv_wifi_connect(void) {
    esp_err_t err;
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[wifi_connect() - setmod_mode] error [0x%X]", err);
        return HUB_ERROR_COMM_CONN_GEN;
    }
    err = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[wifi_connect() - setconfig] error [0x%X]", err);
        return HUB_ERROR_COMM_CONN_GEN;
    }
    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[wifi_connect() - wifistart] error [0x%X]", err);
        return HUB_ERROR_COMM_CONN_GEN;
    }
    return HUB_OK;
}


hub_error_t drv_wifi_stop(void) {
    return esp_wifi_stop();
}

uint8_t drv_wifi_check(void) {
    return wifi_online;
}

hub_error_t drv_wifi_disconnect(void){
  esp_err_t err;
  err = esp_wifi_disconnect();
  if (err != ESP_OK) {
      ESP_LOGE(TAG, "[wifi_disconnect() - wifistop] error [0x%X]", err);
      return HUB_ERROR_COMM_CONN_GEN;
  }
  err = esp_wifi_stop();
    if (err != ESP_OK) {
    ESP_LOGE(TAG, "[wifi_stop() - wifistop] error [0x%X]", err);
    return HUB_ERROR_COMM_CONN_GEN;
  }
  return SUCCESS;
}

static void wifi_next(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();

        xEventGroupClearBits(wifi_events, WIFI_CONNECTED_BIT);

        ESP_LOGI(TAG, "event - offline");

        wifi_online = FALSE;

        if (wifi_callbacks[Offline] != NULL) {
            wifi_callbacks[Offline]();
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;

        xEventGroupSetBits(wifi_events, WIFI_CONNECTED_BIT);

        ESP_LOGI(TAG, "event - online [%s]", ip4addr_ntoa((ip4_addr_t *)&event->ip_info.ip));

        if (wifi_callbacks[Online] != NULL) {
            wifi_callbacks[Online]();
        }
        wifi_online = TRUE;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
        ESP_LOGI(TAG,"[AP] event - online");
        if (wifi_callbacks[Apset] != NULL) {
            wifi_callbacks[Apset]();
        }

        if (server_callback != NULL) {
            server_callback(arg, event_base, event_id, event_data);
        }



    } else if (event_base == WIFI_EVENT &&event_id == WIFI_EVENT_AP_STOP) {
        ESP_LOGI(TAG,"[AP] event - Offline");
        if (wifi_callbacks[apDown] != NULL) {
            wifi_callbacks[apDown]();
        }
        if (server_uncallback != NULL) {
            server_uncallback(arg, event_base, event_id, event_data);
        }
    }
}
