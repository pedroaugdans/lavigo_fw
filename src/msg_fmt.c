//////////////////////////////////////////////////////////////////////////////////////////
//     __  ___ _____  ______            ______ __  ___ ______                           //
//    /  |/  // ___/ / ____/           / ____//  |/  //_  __/                           //
//   / /|_/ / \__ \ / / __   ______   / /_   / /|_/ /  / /                              //
//  / /  / / ___/ // /_/ /  /_____/  / __/  / /  / /  / /                               //
// /_/  /_/ /____/ \____/           /_/    /_/  /_/  /_/                                //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "msg_fmt.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "string.h"
#include "params.h"
#include "msg_raw.h"
#include "esp_attr.h"
#include "freertos/semphr.h"
#include "drv_locks.h"


#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define MQTT_PUBLISH_MAX_DELAY 1000

/***VARIABLES **************************************************************************/

static const char *TAG = "[MSG-FMT]";

static msg_fmt_callback_t msg_fmt_callbacks[NOF_MSG_CHANNELS];
static cJSON * msg_fmt_arrays[NOF_MSG_CHANNELS] = {NULL};

static uint32_t time_taken_by_cb[NOF_MSG_CHANNELS] = {0};

static char *msg_fmt_imessages_static[NOF_MSG_CHANNELS] = {0};
char deploy_imessage[SIZEOF_DEPLOY_IMESSAGE];
char execute_imessage[SIZEOF_EXECUTE_IMESSAGE];
char validate_imessage[SIZEOF_VALIDATION_IMESSAGE];
char monitor_imessage[SIZEOF_MONITOR_IMESSAGE];
char update_imessage[SIZEOF_UPDATE_IMESSAGE];
size_t msg_fmt_input_sizes[] = {SIZEOF_VALIDATION_IMESSAGE,
    SIZEOF_MONITOR_IMESSAGE,
    SIZEOF_DEPLOY_IMESSAGE,
    SIZEOF_EXECUTE_IMESSAGE,
    SIZEOF_UPDATE_IMESSAGE};
#define EXECUTION_LOOPBACK_CONTAINER_LEN 128
static char execution_loopback_container[EXECUTION_LOOPBACK_CONTAINER_LEN] = {0};


static uint8_t msg_fmt_istatus [NOF_MSG_CHANNELS] = {FALSE};
static uint8_t msg_fmt_ostatus [NOF_MSG_CHANNELS] = {FALSE};
// TODO: ADD LOCK

SemaphoreHandle_t msg_fmt_imsg_lock[NOF_MSG_CHANNELS] = {NULL};
SemaphoreHandle_t msg_fmt_omsg_lock[NOF_MSG_CHANNELS] = {NULL};

static uint8_t msg_fmt_i_idx = Validation;
static uint8_t msg_fmt_o_idx = Validation;

/***DECLARATIONS ***********************************************************************/

void msg_fmt_i_update(void);
void msg_fmt_o_update(void);
uint8_t validate_hubId(char * hubId);
static void msg_fmt_validation_cb(char *payload);
static void msg_fmt_monitoring_cb(char *payload);
static void msg_fmt_deployment_cb(char *payload);
static void msg_fmt_execution_cb(char *payload);
static void msg_fmt_update_cb(char *payload);

/***DEFINITIONS ************************************************************************/

void msg_fmt_imessage_assign(void) {
    msg_fmt_imessages_static[Validation] = validate_imessage;
    msg_fmt_imessages_static[Monitoring] = monitor_imessage;
    msg_fmt_imessages_static[Deployment] = deploy_imessage;
    msg_fmt_imessages_static[Execution] = execute_imessage;
    msg_fmt_imessages_static[Update] = update_imessage;
}

void msg_fmt_sph_start(void) {
    for (uint8_t k = 0; k < NOF_MSG_CHANNELS; k++) {
        sph_create(&msg_fmt_imsg_lock[k]);
        sph_create(&msg_fmt_omsg_lock[k]);
        if (msg_fmt_imsg_lock[k] == NULL) {
            ESP_LOGE(TAG, "Cannot create semaphore for channel [%d]", k);
        } else {
            //ESP_LOGI(TAG, "Created semaphore");
        }
    }

}

void msg_fmt_init(void) {
    /*DEBUG*/ESP_LOGI(TAG, "init()");
    msg_fmt_sph_start();
    msg_fmt_imessage_assign();
    msg_raw_init();
    ///*DEBUG*/ESP_LOGI(TAG, "~init()");
}

void TEST_reset_iassignment(void){
  msg_fmt_imessage_assign();
}

void msg_fmt_i_task(void *params) {
    /*DEBUG*/ESP_LOGI(TAG, "task()");
    FOREVER{
        msg_fmt_i_update();
        vTaskDelay(10 / portTICK_RATE_MS);
      }
    /*DEBUG*/ESP_LOGI(TAG, "~task()");
}

void msg_fmt_o_task(void *params) {
    /*DEBUG*/ESP_LOGI(TAG, "task()");

    FOREVER{
        msg_fmt_o_update();
        vTaskDelay(10 / portTICK_RATE_MS);
      }
    /*DEBUG*/ESP_LOGI(TAG, "~task()");
}

static void msg_fmt_empty_buffer(uint8_t msg_fmt_idx) {
    size_t array_size = msg_fmt_input_sizes[msg_fmt_idx];
    for (uint16_t k = 0; k < array_size; k++) {
        msg_fmt_imessages_static[msg_fmt_idx][k] = 0;
    }
}

void msg_fmt_o_update(void){
  if (msg_fmt_ostatus[msg_fmt_o_idx] == TRUE) {
      ESP_LOGI(TAG, "%s", msg_raw_topic(msg_fmt_o_idx, FROM_HUB));
      msg_raw_send(msg_raw_topic(msg_fmt_o_idx, FROM_HUB),
              msg_raw_message(msg_fmt_o_idx, FROM_HUB)
              );
      msg_fmt_ostatus[msg_fmt_o_idx] = FALSE;
      sph_give(&msg_fmt_omsg_lock[msg_fmt_o_idx]);
  }
  msg_fmt_o_idx = (msg_fmt_o_idx + 1) % NOF_MSG_CHANNELS;
}

void msg_fmt_i_update(void) {
    /*DEBUG*///ESP_LOGI(TAG, "update()");
    if (msg_fmt_istatus[msg_fmt_i_idx] == TRUE) {
        ESP_LOGI(TAG, "[%d][%d]", msg_fmt_i_idx, msg_fmt_istatus[msg_fmt_i_idx]);
        msg_fmt_callbacks[msg_fmt_i_idx]();
        msg_fmt_istatus[msg_fmt_i_idx] = FALSE;
        msg_fmt_empty_buffer(msg_fmt_i_idx);
        ESP_LOGE(TAG,"lock given task [%d] @[%d]",msg_fmt_i_idx,get_hub_timestamp());
        sph_give(&msg_fmt_imsg_lock[msg_fmt_i_idx]);
    }

    msg_fmt_i_idx = (msg_fmt_i_idx + 1) % NOF_MSG_CHANNELS;
}

void msg_fmt_configure(void) {
    msg_raw_install(msg_fmt_raw_cb);
}

void msg_fmt_raw_cb(messages_channel_t channel, char *payload) {
    if (sph_step_retries(&msg_fmt_imsg_lock[channel]) == pdTRUE) {
        /*debug*/ ESP_LOGI(TAG, "Received msg for [%d]", channel);
        //ESP_LOGE(TAG,"lock taken task [%d] @[%d]",channel,get_hub_timestamp());
        cJSON * msg = cJSON_Parse(payload);
        if ((!payload) || (strlen(payload) < MINIMUN_JSON_PAYLOAD)) {
            ESP_LOGE(TAG, "Empty cjson message for  [%d]", channel);
            cJSON_Delete(msg);
            sph_give(&msg_fmt_imsg_lock[channel]);
            return;
        }
        if (!msg) {
            ESP_LOGE(TAG, "Invalid CJSON for channel[%d]", channel);
            cJSON_Delete(msg);
            sph_give(&msg_fmt_imsg_lock[channel]);
            return;
        }
        if ((_check_jSONField(msg, msg_fields[hubIdField]) != HUB_OK)
                || (validate_hubId((_get_jSONField(msg, msg_fields[hubIdField]))->valuestring) == FAILURE)) {
            ESP_LOGE(TAG, "Invalid HUB ID for channel [%d]", channel);
            cJSON_Delete(msg);
            sph_give(&msg_fmt_imsg_lock[channel]);
            return;
        }
        if (msg) {
            cJSON_Delete(msg);
        }
        strlcpy((char*) msg_fmt_imessages_static[channel], payload, strlen(payload) + 1);
    } else {
        ESP_LOGE(TAG, "Task [%d] locked , skipping.", channel);
        return;
    }

    time_taken_by_cb[channel] = get_hub_timestamp();
    switch (channel) {
        case Validation:
            msg_fmt_validation_cb(payload);
            break;
        case Deployment:
            msg_fmt_deployment_cb(payload);
            break;
        case Execution:
            msg_fmt_execution_cb(payload);
            break;
        case Update:
            msg_fmt_update_cb(payload);
            break;
        case Monitoring:
            msg_fmt_monitoring_cb(payload);
            break;
        default:
            break;
    }

}

void msg_fmt_loopback_execution_prepare(uint8_t resource, char * action) {
    cJSON * root = cJSON_Parse(loopback_messages_templates[Execution]);

    cJSON_DeleteItemFromObject(root, msg_fields[hubIdField]);
    cJSON_AddStringToObject(root, msg_fields[hubIdField], params_get(HUBID_PARAM));

    cJSON_DeleteItemFromObject(root, msg_fields[actionField]);
    cJSON_AddStringToObject(root, msg_fields[actionField], action);

    cJSON_GetObjectItem(root, msg_fields[resourceField])->valuedouble = resource;

    char * new_msg = cJSON_Print(root);
    strlcpy(
            execution_loopback_container,
            new_msg,
            strlen(new_msg) + 1
            );
    if (new_msg) {
        free(new_msg);
    }
    if (root) {
        cJSON_Delete(root);
    }
}

void msg_fmt_loopback_deployment(void) {
    msg_fmt_raw_cb(Deployment, loopback_messages_templates[Deployment]);
}

void msg_fmt_loopback_execution(void) {
    msg_fmt_raw_cb(Execution, execution_loopback_container);
}

char *msg_fmt_message_static(messages_channel_t channel) {
    return (char*) msg_fmt_imessages_static[channel];
}

uint8_t msg_fmt_install(messages_channel_t channel, msg_fmt_callback_t callback) {
    /*DEBUG*/ESP_LOGI(TAG, "install() [%d]", channel);

    if (callback != NULL) {
        msg_fmt_callbacks[channel] = callback;
    }

    ///*DEBUG*/ESP_LOGI(TAG, "~install()");

    return SUCCESS;
}

uint8_t msg_fmt_load(messages_channel_t channel) {
    if (channel >= NOF_MSG_CHANNELS) {
        return FAILURE;
    }
    if(!sph_check(&msg_fmt_omsg_lock[channel])){
      ESP_LOGW(TAG,"Channel [%d] sending message",channel);
      return FAILURE;
    }

    strlcpy(
            msg_raw_message(channel, FROM_HUB),
            messages_templates[channel],
            strlen(messages_templates[channel]) + 1
            );

    return SUCCESS;
}

uint8_t msg_fmt_start_object_item(messages_channel_t channel) {
    msg_fmt_arrays[channel] = cJSON_CreateObject();
    ESP_LOGI(TAG, "[ dump MSG_FMT 0] dumping...");
    if (msg_fmt_arrays[channel] != NULL) {
        ESP_LOGI(TAG, "Creating new array item");
        return SUCCESS;
    } else {
        return FAILURE;
    }

}

uint8_t msg_format_add_object(messages_channel_t channel, char * key){
    cJSON *root = cJSON_Parse(msg_raw_message(channel, FROM_HUB));
    cJSON * array_element = msg_fmt_arrays[channel];
    ESP_LOGI(TAG, "Appending new array");
    if (root == NULL) {
        ESP_LOGE(TAG, "Unloaded message to append array");
        return FAILURE;
    } else if (array_element == NULL) {
        ESP_LOGE(TAG, "No array to append");
        return FAILURE;
    } else if (key == NULL){
        ESP_LOGE(TAG,"Key invalid");
        return FAILURE;
    }
    cJSON_AddItemToObject(root,key,array_element);
    char * new_msg = cJSON_Print(root);
    /*Debug*///ESP_LOGI(TAG, "%s", new_msg);
    strlcpy(
            msg_raw_message(channel, FROM_HUB),
            new_msg,
            strlen(new_msg) + 1
            );
    if (new_msg) {
        free(new_msg);
    }

    if (root) {
        cJSON_Delete(root);
    }

    return SUCCESS;
}

uint8_t msg_fmt_append_array(messages_channel_t channel, char * key) {
    cJSON *root = cJSON_Parse(msg_raw_message(channel, FROM_HUB));
    cJSON * array_element = msg_fmt_arrays[channel];
    ESP_LOGI(TAG, "Appending new array");
    if (root == NULL) {
        ESP_LOGE(TAG, "Unloaded message to append array");
        return FAILURE;
    } else if (array_element == NULL) {
        ESP_LOGE(TAG, "No array to append");
        return FAILURE;
    }
    if (!cJSON_GetObjectItem(root, key)) {
        cJSON_AddArrayToObject(root, key);
        cJSON * existant_array = cJSON_GetObjectItem(root, key);
        cJSON_AddItemToArray(existant_array, array_element);

        ESP_LOGI(TAG, "Added arrays to object");
    } else {
        cJSON * existant_array = cJSON_GetObjectItem(root, key);
        cJSON_AddItemToArray(existant_array, array_element);
        ESP_LOGI(TAG, "Added new item to array");
    }
    char * new_msg = cJSON_Print(root);
    /*Debug*///ESP_LOGI(TAG, "%s", new_msg);
    strlcpy(
            msg_raw_message(channel, FROM_HUB),
            new_msg,
            strlen(new_msg) + 1
            );
    if (new_msg) {
        free(new_msg);
    }

    if (root) {
        cJSON_Delete(root);
    }

    return SUCCESS;
}

uint8_t msg_fmt_add_str_toarray(messages_channel_t channel, char * key, char * value) {
    if (!value) {
        ESP_LOGE(TAG, "Trying to assign empty string");
        return FAILURE;
    }

    if (channel >= NOF_MSG_CHANNELS) {
        return FAILURE;
    }

    cJSON * root = msg_fmt_arrays[channel];

    if (!root) {
        ESP_LOGE(TAG, "Output buffer not loaded");
    }

    cJSON_AddStringToObject(root, key, value);
    return SUCCESS;
}

uint8_t msg_fmt_add_int_toarray(messages_channel_t channel, char *key, uint32_t value) {
    if (channel >= NOF_MSG_CHANNELS) {
        return FAILURE;
    }

    cJSON * root = msg_fmt_arrays[channel];

    if (!root) {
        ESP_LOGE(TAG, "Output buffer not loaded");
    }

    cJSON_AddNumberToObject(root, key, (double) value);

    return SUCCESS;
}

uint8_t msg_fmt_edit_int(messages_channel_t channel, char *key, uint32_t value) {
    if (channel >= NOF_MSG_CHANNELS) {
        return FAILURE;
    }

    cJSON *root = cJSON_Parse(msg_raw_message(channel, FROM_HUB));

    if (!root) {
        ESP_LOGE(TAG, "Output buffer not loaded");
    }
    if (cJSON_GetObjectItem(root, key)) {
        cJSON_GetObjectItem(root, key)->valuedouble = value;
    } else {
        ESP_LOGE(TAG, "field %s not found, channel [%d], key [%s]", key, channel, key);
        return FAILURE;
    }
    char * new_msg = cJSON_Print(root);
    strlcpy(
            msg_raw_message(channel, FROM_HUB),
            new_msg,
            strlen(new_msg) + 1
            );
    if (new_msg) {
        free(new_msg);
    }

    if (root) {
        cJSON_Delete(root);
    }
    return SUCCESS;
}

uint8_t msg_fmt_append_int(messages_channel_t channel, char *key, uint32_t value) {
    if (channel >= NOF_MSG_CHANNELS) {
        return FAILURE;
    }

    cJSON *root = cJSON_Parse(msg_raw_message(channel, FROM_HUB));

    if (!root) {
        ESP_LOGE(TAG, "Output buffer not loaded");
    }

    cJSON_AddNumberToObject(root, key, (double) value);
    char * new_msg = cJSON_Print(root);
    strlcpy(
            msg_raw_message(channel, FROM_HUB),
            new_msg,
            strlen(new_msg) + 1
            );
    if (new_msg) {
        free(new_msg);
    }

    if (root) {
        cJSON_Delete(root);
    }
    return SUCCESS;
}

uint8_t msg_fmt_edit_str(messages_channel_t channel, char *key, char *value) {
    if (!value) {
        ESP_LOGE(TAG, "Trying to assign empty string");
        return FAILURE;
    }

    if (channel >= NOF_MSG_CHANNELS) {
        return FAILURE;
    }

    cJSON *root = cJSON_Parse(msg_raw_message(channel, FROM_HUB));

    if (!root) {
        ESP_LOGE(TAG, "Output buffer not loaded");
    }

    if (cJSON_GetObjectItem(root, key) &&
            cJSON_GetObjectItem(root, key)->valuestring) {

        cJSON_DeleteItemFromObject(root, (const char *) key);
    } else {
        //ESP_LOGE(TAG, "field %s not found, channel [%d], key [%s]", key, channel, key);
        //return FAILURE;
    }
    cJSON_AddStringToObject(root, key, value);
    char * new_msg = cJSON_PrintUnformatted(root);
    strlcpy(
            msg_raw_message(channel, FROM_HUB),
            new_msg,
            strlen(new_msg) + 1
            );

    if (new_msg != NULL) {
        free(new_msg);
    }

    if (root != NULL) {
        cJSON_Delete(root);
    }
    return SUCCESS;
}

uint8_t msg_fmt_send(messages_channel_t channel) {
    if (channel >= NOF_MSG_CHANNELS) {
        return FAILURE;
    }
    if (sph_take_delayed(&msg_fmt_omsg_lock[channel], MQTT_PUBLISH_MAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "send()");
        msg_fmt_ostatus[channel] = TRUE;
        return SUCCESS;
    } else {
        ESP_LOGE(TAG, "Lock untacken at [%d]", channel);
        return FAILURE;
    }
}

uint8_t msg_fmt_append_error(messages_channel_t channel, uint16_t value) {
    if (channel >= NOF_MSG_CHANNELS) {
        return FAILURE;
    }

    cJSON *root = cJSON_Parse(msg_raw_message(channel, FROM_HUB));

    if (!root) {
        ESP_LOGE(TAG, "Output buffer not loaded");
    }
    cJSON_AddNumberToObject(root, msg_fields[errorField], value);

    char * new_msg = (char*) cJSON_Print(root);
    strlcpy(
            msg_raw_message(channel, FROM_HUB),
            (char*) new_msg,
            strlen(new_msg) + 1
            );
    if (new_msg) {
        free(new_msg);
    }

    if (root) {
        cJSON_Delete(root);
    }
    return SUCCESS;
}

static void msg_fmt_validation_cb(char *payload) {
    //ESP_LOGI(TAG, "receive() %s", payload);

    //msg_fmt_imessages[Validation] = cJSON_Parse(payload);
    msg_fmt_istatus[Validation] = TRUE;
}

static void msg_fmt_monitoring_cb(char *payload) {
    //ESP_LOGI(TAG, "receive() %s", payload);
    msg_fmt_istatus[Monitoring] = TRUE;
}

static void msg_fmt_deployment_cb(char *payload) {
    //ESP_LOGI(TAG, "receive() %s", payload);



    //msg_fmt_imessages[Deployment] = cJSON_Parse(payload);
    msg_fmt_istatus[Deployment] = TRUE;
}

static void msg_fmt_execution_cb(char *payload) {
    //ESP_LOGI(TAG, "receive() %s", payload);


    //msg_fmt_imessages[Execution] = cJSON_Parse(payload);
    msg_fmt_istatus[Execution] = TRUE;
}

static void msg_fmt_update_cb(char *payload) {
    //ESP_LOGI(TAG, "receive() %s", payload);


    //msg_fmt_imessages[Update] = cJSON_Parse(payload);
    msg_fmt_istatus[Update] = TRUE;
}

void msg_fmt_test_cb(messages_channel_t topic, char * msg) {
    msg_fmt_imessages_static[topic] = msg;
}

uint8_t validate_hubId(char * hubId_tocheck) {
    if (!strcmp(hubId_tocheck, params_get(HUBID_PARAM))) {
        ESP_LOGI(TAG, "Successful HUB ID check");
        return SUCCESS;
    }
    return FAILURE;
}

hub_error_t _check_jSONField(cJSON *object, char *key) {
    cJSON * check_cjson = cJSON_GetObjectItemCaseSensitive(object, key);
    if (check_cjson == NULL) {
        ESP_LOGE(TAG, "(400) missing key [%s]!", key);
        return HUB_ERROR_COMM_MSG_NOJSON;
    }
    return HUB_OK;
}

cJSON *_get_jSONField(cJSON *object, char *key) {
    return cJSON_GetObjectItemCaseSensitive(object, key);
}

hub_error_t check_ifString(cJSON * root, char * key) {
    if (!cJSON_GetObjectItemCaseSensitive(root, key)->valuestring) {
        ESP_LOGE(TAG, "[%s] field is not a string.", key);
        return HUB_ERROR_COMM_MSG_NOJSON;
    }
    return HUB_OK;
}

hub_error_t check_field_content(char * str_tocheck, char ** reference_list, uint8_t total_elems) {
    for (uint8_t k = 0; k < total_elems; k++) {
        if (!strcmp(reference_list[k], str_tocheck)) {
            return HUB_OK;
        }
    }
    ESP_LOGE(TAG, "[%s] field not found on list.", str_tocheck);
    return HUB_ERROR_COMM_MSG_NOFIELD;
}

void stash_jSON_msg(cJSON * msg) {
    if (msg != NULL) {
        cJSON_Delete(msg);
    }
}
