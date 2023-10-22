//////////////////////////////////////////////////////////////////////////////////////////
//     __  ___ _____  ______            ____   ___  _       __                          //
//    /  |/  // ___/ / ____/           / __ \ /   || |     / /                          //
//   / /|_/ / \__ \ / / __   ______   / /_/ // /| || | /| / /                           //
//  / /  / / ___/ // /_/ /  /_____/  / _, _// ___ || |/ |/ /                            //
// /_/  /_/ /____/ \____/           /_/ |_|/_/  |_||__/|__/                             //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "msg_raw.h"

#include "stdio.h"
#include "string.h"
#include "drv_locks.h"

#include "drv_mqtt.h"
#include "params.h"
#include "hub_error.h"

#define MFG_FMT_ITOPIC_LENGTH     128
#define MFG_FMT_OTOPIC_LENGTH     128
#define MFG_FMT_IMESSAGE_LENGTH  1200
#define MFG_FMT_DFLT_OMESSAGE_LENGTH   200

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

/*** VARIABLES **************************************************************************/

static const char *TAG = "[MSG-RAW]";

/*TOPIC ASSEMBLY STRUCTURE*/
/*The topic assembles as follows:
{ROOT}/{STAGE}/{HUB-ID FOR DOWNLINK}/LEAVE*/

static const char *_roots[] = {
    "hubs",         // Validation
    "hubs",         // Monitoring
    "resources",    // Deployment
    "resources",    // Execution
    "hubs",         // Update
};

static const char *_leaves[] = {
    "registration", // Validation
    "monitoring",   // Monitoring
    "deployment",   // Deployment
    "execution",    // Execution
    "update",       // Update
};

/*Topic string holders*/
static char msg_raw_itopics [NOF_MSG_CHANNELS][MFG_FMT_ITOPIC_LENGTH] = {0};
static char msg_raw_otopics [NOF_MSG_CHANNELS][MFG_FMT_OTOPIC_LENGTH] = {0};

/*Input buffers: They are splitted and then assigned to have different sizes*/
char *msg_raw_imessages_static[NOF_MSG_CHANNELS] = {0};
char msg_raw_deploy_ibuffer[MFG_FMT_IMESSAGE_LENGTH] = {0};
char msg_raw_execute_ibuffer[MFG_FMT_DFLT_OMESSAGE_LENGTH] = {0};
char msg_raw_validation_ibuffer[MFG_FMT_DFLT_OMESSAGE_LENGTH] = {0};
char msg_raw_monitoring_ibuffer[MFG_FMT_DFLT_OMESSAGE_LENGTH] = {0};
char msg_raw_update_ibuffer[MFG_FMT_DFLT_OMESSAGE_LENGTH] = {0};

/*Output buffers: They are splitted and then assigned to have different sizes*/
char *msg_raw_omessages[NOF_MSG_CHANNELS] = {0};
char msg_raw_deploy_obuffer[MFG_FMT_DFLT_OMESSAGE_LENGTH] = {0};
char msg_raw_execute_obuffer[MFG_FMT_DFLT_OMESSAGE_LENGTH] = {0};
char msg_raw_validation_obuffer[MFG_FMT_DFLT_OMESSAGE_LENGTH] = {0};
char msg_raw_monitoring_obuffer[MFG_FMT_IMESSAGE_LENGTH] = {0};
char msg_raw_update_obuffer[MFG_FMT_DFLT_OMESSAGE_LENGTH] = {0};

SemaphoreHandle_t msg_raw_lock = NULL;
//static uint8_t _inputMessageStatus[NOF_MSG_CHANNELS] = { FALSE };

/*** DECLARATIONS ***********************************************************************/

static void msg_raw_default_cb(messages_channel_t channel, char *payload);
static void msg_raw_receive(drv_mqtt_message_t *message);

static msg_raw_callback_t _callbacks = msg_raw_default_cb;

/*** DEFINITIONS ************************************************************************/
void msg_raw_buffers_init(void) {
    msg_raw_imessages_static[Validation] = msg_raw_validation_ibuffer;
    msg_raw_imessages_static[Monitoring] = msg_raw_monitoring_ibuffer;
    msg_raw_imessages_static[Deployment] = msg_raw_deploy_ibuffer;
    msg_raw_imessages_static[Execution] = msg_raw_execute_ibuffer;
    msg_raw_imessages_static[Update] = msg_raw_update_ibuffer;

    msg_raw_omessages[Validation] = msg_raw_validation_obuffer;
    msg_raw_omessages[Monitoring] = msg_raw_monitoring_obuffer;
    msg_raw_omessages[Deployment] = msg_raw_deploy_obuffer;
    msg_raw_omessages[Execution] = msg_raw_execute_obuffer;
    msg_raw_omessages[Update] = msg_raw_update_obuffer;
}

void msg_raw_init(void) {
    /*DEBUG*/ESP_LOGI(TAG, "init()");
    msg_raw_buffers_init();
    drv_mqtt_install(&msg_raw_receive, Messaged);

    ///*DEBUG*/ESP_LOGI(TAG, "~init()");
}

char *msg_raw_topic(messages_channel_t channel, bool downlink) {
    if (channel >= NOF_MSG_CHANNELS) {
        ESP_LOGE(TAG, "(404) channel not found [%d]", channel);
        return NULL;
    }

    if (!downlink) {
        if (msg_raw_otopics[channel][0] == '\0') {
            snprintf(
                    msg_raw_otopics[channel],
                    MFG_FMT_OTOPIC_LENGTH,
                    "%s/%s/%s",
                    _roots[channel],
                    params_get(STAGE_PARAM),
                    _leaves[channel]
                    );
        }

        return msg_raw_otopics[channel];
    } else {
        if (msg_raw_itopics[channel][0] == '\0') {
            snprintf(
                    msg_raw_itopics[channel],
                    MFG_FMT_OTOPIC_LENGTH,
                    "%s/%s/%s/%s",
                    _roots[channel],
                    params_get(STAGE_PARAM),
                    params_get(HUBID_PARAM),
                    _leaves[channel]
                    );
        }

        return msg_raw_itopics[channel];
    }

    return NULL;
}

char *msg_raw_message(messages_channel_t channel, bool downlink) {
    if (channel >= NOF_MSG_CHANNELS) {
        ESP_LOGE(TAG, "(404) channel not found [%d]", channel);
        return NULL;
    }
    if (!downlink) {
        return msg_raw_omessages[channel];
    } else {
        return msg_raw_imessages_static[channel];
    }
}

uint8_t msg_raw_install(msg_raw_callback_t callback) {
    if (callback == NULL) {
        _callbacks = msg_raw_default_cb;
    } else {
        _callbacks = callback;
    }
    for (uint8_t k = 0; k < NOF_MSG_CHANNELS; k++) {
      drv_mqtt_subscribe(msg_raw_topic(k, TO_HUB));
    }
    ESP_LOGI(TAG, "~Subscribe");
    return FAILURE;
}

uint8_t msg_raw_send(char *topic, char *payload) {
    //ESP_LOGI(TAG, "send() [%s]\n%s", topic, payload);
    if(get_hub_Online_status()){
      hub_error_t result = drv_mqtt_publish(topic, payload);
      if(result != SUCCESS){
        ESP_LOGE(TAG,"ERROS SENDING");
      }else {
        ESP_LOGI(TAG,"Sent successfully");
      }
      return result;
    } else {
      ESP_LOGE(TAG,"No internet connection");
      return FAILURE;
    }
}

static void msg_raw_receive(drv_mqtt_message_t *message) {
    ESP_LOGI(TAG, "receive() [%s][%d]\n%s", message->topic, message->plen, message->payload);

    /*char *root = */ strtok((char *) message->topic, "/");
    char *stage = strtok(NULL, "/");
    (void) stage;
    char *hub = strtok(NULL, "/");
    (void) hub;
    char *leaf = strtok(NULL, "/{");

    if (leaf == NULL) {
        ESP_LOGE(TAG, "(400) missing leaf...");
        return;
    }

    messages_channel_t channel;

    //ESP_LOGI(TAG, "! leaf:[%s]", leaf);
    for (channel = Validation; channel <= NOF_MSG_CHANNELS; channel++) {
        //ESP_LOGI(TAG, "? leaf:[%s]", _leaves[channel]);
        if (!strcmp(leaf, _leaves[channel])) {
            break;
        }
    }

    if (channel > NOF_MSG_CHANNELS) {
        ESP_LOGE(TAG, "(404) channel not found [%d]", channel);
        return;
    }


    message->payload[0] = '{';

    for (uint16_t i = 0; i < message->plen + 1; i++) {
        //ESP_LOGI(TAG, "[%d:%c]", i, message->payload[i]);
        msg_raw_imessages_static[channel][i] = message->payload[i];
    }
    (*_callbacks)(channel, msg_raw_imessages_static[channel]);


    for (uint16_t i = 0; i < message->plen + 2; i++) {
        msg_raw_imessages_static[channel][i] = 0;
    }
}

static void msg_raw_default_cb(messages_channel_t channel, char *payload) {
    ESP_LOGE(TAG, "default() [%s] for [%d]", payload, channel);
}
