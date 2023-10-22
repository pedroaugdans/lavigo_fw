//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____   ____ _    __ ______ ____              __  ___ ____  ______ ______  //
//    / __ \ / __ \ /  _/| |  / // ____// __ \            /  |/  // __ \/_  __//_  __/  //
//   / / / // /_/ / / /  | | / // __/  / /_/ /  ______   / /|_/ // / / / / /    / /     //
//  / /_/ // _, _/_/ /   | |/ // /___ / _, _/  /_____/  / /  / // /_/ / / /    / /      //
// /_____//_/ |_|/___/   |___//_____//_/ |_|           /_/  /_/ \___\_\/_/    /_/       //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "drv_mqtt.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "aws_iot_config.h"
#include "aws_iot_error.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "drv_locks.h"
#include "string.h"

#include "params.h"
#include "drv_nvs.h"
#include "drv_wifi.h"
#include "msg_raw.h"
#include "launcher.h"

#define FOREVER  while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00
#define MQTT_TOTAL_KEYS 5

#define DRV_MQTT_PACKET_TIMEOUT 100
#define DRV_MQTT_DEFAULT_PORT 8883

/*** VARIABLES **************************************************************************/

static const char *TAG = "[DRV-MQTT]";

/*Callbacks holder*/                                            /*Conn, Disc, Msg, Failed */
static drv_mqtt_callback_t drv_mqtt_callbacks[NOF_MQTT_EVENTS] = {NULL, NULL,NULL,NULL};
/******************Key memory holders*******************************************/
static char _ioturl[IOTURL_SECRET_LENGTH] = {0};  /*IoT memory holder, 50 by*/
static char _hubzid[HUBZID_SECRET_LENGTH] = {0};  /*hubId memory holder, 50 by*/
static char _ecckey[ECCKEY_SECRET_LENGTH] = {0};  /*eccKey memory holder, 500 by*/
static char _ecccrt[ECCCRT_SECRET_LENGTH] = {0};  /*eccCrt memory holder, 1800 by*/
static char _awscrt[AWSCRT_SECRET_LENGTH] = {0};  /*awsCRT memory holder, 1800 by*/

/*registration_command_t will be used all along the key modification functions*/
static const registration_command_t mqtt_keys_list[MQTT_TOTAL_KEYS] = {IOTURL_CMD,
HUBZID_CMD, ECCKEY_CMD, ECCCRT_CMD, AWSCRT_CMD};

/*Iterator to load / print the key holders*/
static char * mqtt_keys_pointers[] = {_ioturl,
  _hubzid,_ecckey,_ecccrt,_awscrt};

static char _payload[256];

static AWS_IoT_Client drv_mqtt_client;

static IoT_Publish_Message_Params _qosParams = {};
static IoT_Client_Connect_Params _connectParams = {};
static IoT_Client_Init_Params _initParams = {};
SemaphoreHandle_t mqtt_lock = NULL;
static uint8_t drv_mqtt_online = FALSE;

/*** DECLARATIONS ***********************************************************************/

static void drv_mqtt_sleep(uint16_t ticks);

static bool drv_mqtt_isActive(void);
static bool drv_mqtt_isConnected(void) __attribute__((unused));

static void drv_mqtt_subscription_cb(AWS_IoT_Client *pClient, char *topic, uint16_t tlen,
        IoT_Publish_Message_Params *params, void *data);
static void drv_mqtt_disconnection_cb(AWS_IoT_Client *pClient, void *data);
static void drv_mqtt_wait(uint16_t ticks);

/*** DEFINITIONS ************************************************************************/

void drv_mqtt_init(void) {
    /*DEBUG*/ESP_LOGI(TAG, "init()");
    sph_create(&mqtt_lock);
}

void drv_mqtt_reset(void){
  aws_iot_mqtt_disconnect (&drv_mqtt_client);
  //aws_iot_mqtt_free(&drv_mqtt_client);
}

void drv_mqtt_task(void *params) {
    /*DEBUG*/ESP_LOGI(TAG, "task()");
    drv_mqtt_init();

    IoT_Error_t err = -1;

    FOREVER{
        if (drv_mqtt_isActive()) {
            do {
                if (sph_step_retries(&mqtt_lock) == TRUE) {
                    //ESP_LOGI(TAG,"[YIELD LOCK]");
                    err = aws_iot_mqtt_yield(&drv_mqtt_client, 3000);
                    //ESP_LOGI(TAG,"~[YIELD LOCK]");
                    sph_give(&mqtt_lock);
                    if (err != SUCCESS) {
                        ESP_LOGE(TAG, "Yield error [%d]", err);
                    } else {
                        //ESP_LOGI(TAG, "[YIELD] success yield");
                    }
                } else {
                    ESP_LOGE(TAG, "[yield] LOCK Untaken");
                }
                drv_mqtt_wait(1);
            } while (err == MQTT_CLIENT_NOT_IDLE_ERROR);
        } else {
            drv_mqtt_sleep(5);
        }
      }
    /*DEBUG*/ESP_LOGI(TAG, "~task()");
}

void drv_mqtt_install(drv_mqtt_callback_t callback, drv_mqtt_event_t event) {
    /*DEBUG*/ESP_LOGI(TAG, "install() [%d]", event);

    if (event < NOF_MQTT_EVENTS) {
        drv_mqtt_callbacks[event] = callback;
    }

    ///*DEBUG*/ESP_LOGI(TAG, "~install()");
}

static void clean_name_buffer(char * buffer_to_clean, uint8_t length){
  for(uint8_t k = 0; k<length;k++){
    buffer_to_clean[k]=0;
  }
}

uint8_t drv_mqtt_load_keys(void){
  char key_name_buffer[12] = {0};
  drv_keys_buffer_t next_buffer_pointer;
  for(uint8_t k = 0 ; k < MQTT_TOTAL_KEYS; k++){
    clean_name_buffer(key_name_buffer,12);

    registration_command_t next_storage_key_idx = mqtt_keys_list[k];
    next_buffer_pointer = current_used_keys[next_storage_key_idx].key_flash_idx;
    assemble_key_name(key_name_buffer,registration_commands[next_storage_key_idx],next_buffer_pointer);

    drv_nvs_get(key_name_buffer, mqtt_keys_pointers[k]);
    if(mqtt_keys_list[k]==IOTURL_CMD){
      ESP_LOGI(TAG,"IOTURL is [%s]",mqtt_keys_pointers[k]);
    }
    vTaskDelay(100 / portTICK_RATE_MS);
  }
  return HUB_OK;
}

uint8_t drv_mqtt_configure(void) {
    /*DEBUG*/ESP_LOGI(TAG, "configure()");
    drv_mqtt_load_keys();
    _initParams = iotClientInitParamsDefault;

    _initParams.pHostURL = _ioturl;
    _initParams.port = DRV_MQTT_DEFAULT_PORT;
    _initParams.pRootCALocation = _awscrt;
    _initParams.pDeviceCertLocation = _ecccrt;
    _initParams.pDevicePrivateKeyLocation = _ecckey;
    _initParams.disconnectHandler = drv_mqtt_disconnection_cb;
    _initParams.mqttPacketTimeout_ms = DRV_MQTT_PACKET_TIMEOUT;

    _qosParams.qos = QOS1;
    _qosParams.payload = (void *) _payload;
    _qosParams.isRetained = 0;

    IoT_Error_t err = -1;
    if (sph_step_retries(&mqtt_lock) == TRUE) {
        err = aws_iot_mqtt_init(&drv_mqtt_client, &_initParams);
        sph_give(&mqtt_lock);
    } else {
        ESP_LOGE(TAG, "[CONFIGURE] Lock untaken");
    }
    if (err != SUCCESS) {
        ESP_LOGE(TAG, "aws_iot_mqtt_init error:[0x%X] ", err);

        return FAILURE;
    }

    /*DEBUG*/ESP_LOGI(TAG, "~configure()");

    return SUCCESS;
}

uint8_t drv_mqtt_connect(void) {
    /*DEBUG*/ESP_LOGI(TAG, "connect()");
    uint8_t online = FALSE;

    drv_nvs_get("warm-up", NULL);
    vTaskDelay(100 / portTICK_RATE_MS);
    drv_nvs_get(HUBZID_IDX, _hubzid);

    _connectParams = iotClientConnectParamsDefault;
    _connectParams.keepAliveIntervalInSec = 5;
    _connectParams.isCleanSession = true;
    _connectParams.pClientID = (const char *) _hubzid;
    _connectParams.clientIDLen = HUBZID_SECRET_LENGTH;

    for (uint8_t i = 0; i < 10; i++) {
        if (drv_wifi_check()) {
            online = TRUE;
            break;
        }
        //ESP_LOGI(TAG, "waiting...");
        drv_mqtt_sleep(1);
    }
    if (online == FALSE) {
        ESP_LOGE(TAG, "online = FALSE 1");
        return FAILURE;
    }

    IoT_Error_t err = FAILURE;
    online = FALSE;

    for (uint8_t i = 0; i < 10; i++) {
        if (sph_step_retries(&mqtt_lock) == TRUE) {
            err = aws_iot_mqtt_connect(&drv_mqtt_client, &_connectParams);
            sph_give(&mqtt_lock);
        } else {
            ESP_LOGE(TAG, "[CONNECT] Lock untaken");
        }
        if (err == SUCCESS) {
            online = TRUE;
            break;
        }
        //ESP_LOGI(TAG, "waiting... [0x%X]", err);
        drv_mqtt_sleep(1);
    }

    if (online == FALSE) {
        ESP_LOGE(TAG, "online = FALSE 2");
        return FAILURE;
    }

    while (err != SUCCESS) {
        if (sph_step_retries(&mqtt_lock) == TRUE) {
            err = aws_iot_mqtt_connect(&drv_mqtt_client, &_connectParams);
            sph_give(&mqtt_lock);
        } else {
            ESP_LOGE(TAG, "[COnnect] LOCK Untaken");
        }

        if (SUCCESS != err) {
            ESP_LOGE(TAG, "aws_iot_mqtt_connect error:[%d]", err);
            drv_mqtt_sleep(1);
        }
    }
    if (sph_step_retries(&mqtt_lock) == TRUE) {
        aws_iot_mqtt_autoreconnect_set_status(&drv_mqtt_client, false);
        err = aws_iot_mqtt_is_client_connected(&drv_mqtt_client);
        sph_give(&mqtt_lock);
    } else {
        ESP_LOGE(TAG, "[SET RECONNECT] Lock untaken");
    }

    if (err == SUCCESS || err == MQTT_CONNACK_CONNECTION_ACCEPTED) {
        if (drv_mqtt_callbacks[Connected] != NULL) {
            //ESP_LOGI(TAG, "drv_mqtt_callbacks[Connected](NULL);");
            drv_mqtt_callbacks[Connected](NULL);
            //ESP_LOGI(TAG, "~drv_mqtt_callbacks[Connected](NULL);");

        }
        err = SUCCESS;
        ESP_LOGI(TAG, "Connection ACCEPTED [0x%X]", err);
        drv_mqtt_online = TRUE;
    } else {
        ESP_LOGE(TAG, "aws_iot_mqtt_isdrv_mqtt_client_connected error:[0x%X]", err);
    }

    /*DEBUG*/ESP_LOGI(TAG, "~connect()");
    ESP_LOGI(TAG, "returning [0x%X]", err);
    return err;
}

uint8_t drv_mqtt_reconnect(void) {
    /*DEBUG*/ESP_LOGI(TAG, "reconnect()");
    uint8_t online = FALSE;
    drv_mqtt_load_keys();

    for (uint8_t i = 0; i < 10; i++) {
        if (drv_wifi_check()) {
            online = TRUE;
            break;
        }
        //ESP_LOGI(TAG, "waiting...");
        drv_mqtt_sleep(1);
    }
    if (online == FALSE) {
        ESP_LOGE(TAG, "Hub still not online");
        return FAILURE;
    }
    IoT_Error_t err = FAILURE;
    online = FALSE;

    for (uint8_t i = 0; i < 10; i++) {
        if (sph_step_retries(&mqtt_lock) == TRUE) {
            err = aws_iot_mqtt_attempt_reconnect(&drv_mqtt_client);
            sph_give(&mqtt_lock);
        } else {
            ESP_LOGE(TAG, "[Reconnect] lock untaken");
        }
        if ((err == SUCCESS) || (err == NETWORK_ALREADY_CONNECTED_ERROR)) {
            online = TRUE;
            break;
        }
        //ESP_LOGI(TAG, "waiting... [0x%X]", err);
        drv_mqtt_sleep(1);
    }

    if (online == FALSE) {
        ESP_LOGE(TAG, "Hub still not online, error [%d]",err);
        if ((drv_mqtt_callbacks[ConnFailed] != NULL) && (err != SUCCESS)) {
            drv_mqtt_callbacks[ConnFailed](NULL);
        }
        return FAILURE;
    }

    while ((err != SUCCESS) && (err != NETWORK_ALREADY_CONNECTED_ERROR)) {
        if (sph_step_retries(&mqtt_lock) == TRUE) {
            err = aws_iot_mqtt_attempt_reconnect(&drv_mqtt_client);
            sph_give(&mqtt_lock);
        } else {
            ESP_LOGE(TAG, "[Reconnect] lock untaken");
        }
        if (SUCCESS != err) {
            ESP_LOGE(TAG, "Hub still disconnected, error:[%d]", err);
            drv_mqtt_sleep(1);
        }
    }
    if (sph_step_retries(&mqtt_lock) == TRUE) {
        err = aws_iot_mqtt_is_client_connected(&drv_mqtt_client);
        sph_give(&mqtt_lock);
    } else {
        ESP_LOGE(TAG, "[Reconnect] lock untaken");
    }

    if ((err == SUCCESS) || (err == MQTT_CONNACK_CONNECTION_ACCEPTED)
            || (err == NETWORK_ALREADY_CONNECTED_ERROR)) {
        if (drv_mqtt_callbacks[Connected] != NULL) {
            //ESP_LOGI(TAG, "drv_mqtt_callbacks[Connected](NULL);");
            drv_mqtt_callbacks[Connected](NULL);
            //ESP_LOGI(TAG, "~drv_mqtt_callbacks[Connected](NULL);");

        }
        err = SUCCESS;
        ESP_LOGI(TAG, "Connection ACCEPTED [0x%X]", err);
        drv_mqtt_online = TRUE;
    } else {
        ESP_LOGE(TAG, "aws_iot_mqtt_isdrv_mqtt_client_connected error:[0x%X]", err);
    }

    if (sph_step_retries(&mqtt_lock) == TRUE) {
        aws_iot_mqtt_autoreconnect_set_status(&drv_mqtt_client, false);
        sph_give(&mqtt_lock);
    } else {
        ESP_LOGE(TAG, "[SET RECONNECT] Lock untaken");
    }

    /*DEBUG*/ESP_LOGI(TAG, "~reconnect()");
    ESP_LOGI(TAG, "returning [0x%X]", err);
    if ((drv_mqtt_callbacks[ConnFailed] != NULL) && (drv_mqtt_online != TRUE)) {
        drv_mqtt_callbacks[ConnFailed](NULL);
    }
    return err;
}

uint8_t drv_mqtt_check(void) {
    return drv_mqtt_online;
}

uint8_t drv_mqtt_subscribe(const char *topic) {
    /*DEBUG*/ESP_LOGI(TAG, "subscribe() [%s]", topic);

    IoT_Error_t err = -1;

    do {
        if (sph_step_retries(&mqtt_lock) == TRUE) {
            err = aws_iot_mqtt_subscribe(&drv_mqtt_client, topic, strlen(topic), QOS1, drv_mqtt_subscription_cb, NULL);
            sph_give(&mqtt_lock);
        } else {
            ESP_LOGE(TAG, "[SUSCRIBE] Lock untaken");
        }
        vTaskDelay(5 / portTICK_RATE_MS);

        if (err != SUCCESS) {
            //ESP_LOGE(TAG, "error:[0x%X] state:[0x%X]", err, aws_iot_mqtt_get_client_state(&drv_mqtt_client));
        }

        //ESP_LOGE(TAG, "aws_iot_mqtt_publish error:[0x%X]", err);
    } while (MUTEX_LOCK_ERROR == err || MQTT_CLIENT_NOT_IDLE_ERROR == err); //|| NETWORK_DISCONNECTED_ERROR == err);

    if (SUCCESS != err) {
        ESP_LOGE(TAG, "aws_iot_mqtt_publish error:[0x%X]", err);

        do {
            if (sph_step_retries(&mqtt_lock) == TRUE) {
                err = aws_iot_mqtt_subscribe(&drv_mqtt_client, topic, strlen(topic), QOS1, drv_mqtt_subscription_cb, NULL);
                sph_give(&mqtt_lock);
            } else {
                ESP_LOGE(TAG, "[Suscribe] LOCK UNTAKEN");
            }
            vTaskDelay(5 / portTICK_RATE_MS);

            if (err != SUCCESS) {
                //ESP_LOGE(TAG, "error:[0x%X] state:[0x%X]", err, aws_iot_mqtt_get_client_state(&drv_mqtt_client));
            }

            //ESP_LOGE(TAG, "aws_iot_mqtt_publish error:[0x%X]", err);
        } while (MUTEX_LOCK_ERROR == err || MQTT_CLIENT_NOT_IDLE_ERROR == err); //|| NETWORK_DISCONNECTED_ERROR == err);

        if (SUCCESS != err) {
            ESP_LOGE(TAG, "aws_iot_mqtt_publish error:[0x%X]", err);
            return FAILURE;
        }
    }
    /*DEBUG*/ESP_LOGI(TAG, "~subscribe()");
    return SUCCESS;
}

uint8_t drv_mqtt_publish(const char *topic, const char *message) {
    /*DEBUG*/ESP_LOGI(TAG, "publish() [%s]", topic);
    int8_t retries_counter = 32;
    snprintf(_payload, strlen(message) + 1, "%s", message);

    _qosParams.payloadLen = strlen(_payload);

    IoT_Error_t err = -1;

    do {
        if (sph_step_retries(&mqtt_lock) == TRUE) {
            //ESP_LOGI(TAG,"[PUBLISH LOCK]");
            err = aws_iot_mqtt_publish(&drv_mqtt_client, topic, strlen(topic), &_qosParams);
            //ESP_LOGI(TAG,"~[PUBLISH LOCK]");
            sph_give(&mqtt_lock);
            if(err == SUCCESS){
              break;
            }
        }
        vTaskDelay(5 / portTICK_RATE_MS);
        retries_counter--;
    } while (MUTEX_LOCK_ERROR == err || MQTT_CLIENT_NOT_IDLE_ERROR == err || (retries_counter>0 && NETWORK_DISCONNECTED_ERROR != err)); //|| NETWORK_DISCONNECTED_ERROR == err);


    retries_counter = 32;
    if (SUCCESS != err) {
      ESP_LOGW(TAG,"Publish retrying! after at least 32 retries");
        do {
            if (sph_step_retries(&mqtt_lock) == TRUE) {
                err = aws_iot_mqtt_publish(&drv_mqtt_client, topic, strlen(topic), &_qosParams);
                sph_give(&mqtt_lock);
                if(err == SUCCESS){
                  break;
                }
            }
            vTaskDelay(5 / portTICK_RATE_MS);
            retries_counter--;
        } while (MUTEX_LOCK_ERROR == err || MQTT_CLIENT_NOT_IDLE_ERROR == err|| (retries_counter>0 && NETWORK_DISCONNECTED_ERROR != err)); //|| NETWORK_DISCONNECTED_ERROR == err);

        if (SUCCESS != err) {
            ESP_LOGE(TAG, "publish error:[0x%X]", err);
            return FAILURE;
        }
        //aws_iot_mqtt_yield(&drv_mqtt_client, 50);
    }

    if(err == NETWORK_DISCONNECTED_ERROR){
      ESP_LOGE(TAG,"Network disconnected!");
    }

    /*DEBUG*/ESP_LOGI(TAG, "~publish()");

    return SUCCESS;
}

static bool drv_mqtt_isActive() {
    /*DEBUG*///ESP_LOGI(TAG, "isActive()");

    return run_activation_flags[mqtt_run_flag];
}

static bool drv_mqtt_isConnected() {
    /*DEBUG*///ESP_LOGI(TAG, "isConnected()");

    return 1;//drv_wifi_check(); //hub_isOnline && aws_iot_mqtt_is_client_connected(&drv_mqtt_client);
}

static void drv_mqtt_sleep(uint16_t ticks) {

    /*DEBUG*///ESP_LOGI(TAG, "sleep()");
    vTaskDelay(suspendedThreadDelay[mqtt_flag] * ticks / portTICK_RATE_MS);

    /*DEBUG*///ESP_LOGI(TAG, "~sleep()");
}

static void drv_mqtt_wait(uint16_t ticks) {
    /*DEBUG*///ESP_LOGI(TAG, "sleep()");

    vTaskDelay((drv_wifi_check() ? 100 * ticks : 2000)/ portTICK_RATE_MS);

    /*DEBUG*///ESP_LOGI(TAG, "~sleep()");
}

static void drv_mqtt_subscription_cb(AWS_IoT_Client *pClient, char *topic, uint16_t tlen,
        IoT_Publish_Message_Params *params, void *data) {
    /*DEBUG*/ESP_LOGI(TAG, "subscription_cb() [%s][%d] id:[%d] retained:[%d]", topic, params->isDup, params->id, params->isRetained);
    drv_mqtt_message_t message;


    message.topic = (char *) topic;
    message.tlen = tlen;
    message.payload = (char *) params->payload;
    message.plen = params->payloadLen;
    if (drv_mqtt_callbacks[Messaged] != NULL) {
        ESP_LOGI(TAG, "drv_mqtt_callbacks[Messaged](&message);");
        drv_mqtt_callbacks[Messaged](&message);
        ESP_LOGI(TAG, "~drv_mqtt_callbacks[Messaged](&message);");
    }

    for (uint16_t i = 0; i < tlen; i++) {
        ((char *) topic)[i] = 0;
    }

    for (uint16_t i = 0; i < params->payloadLen; i++) {
        ((char *) params->payload)[i] = 0;
    }

    /*DEBUG*/ESP_LOGI(TAG, "~subscription_cb()");
}

void force_disconnection(void){
  drv_mqtt_disconnection_cb(NULL,NULL);
}

static void drv_mqtt_disconnection_cb(AWS_IoT_Client *pClient, void *data) {
    /*DEBUG*/ESP_LOGI(TAG, "disconnection_cb()");
    drv_mqtt_online = FALSE;
    if (drv_mqtt_callbacks[Disconnected] != NULL) {
        drv_mqtt_callbacks[Disconnected](NULL);
    }

    /*DEBUG*/ESP_LOGI(TAG, "~disconnection_cb()");
}
