//////////////////////////////////////////////////////////////////////////////////////////
//     __  ___ _____  ______            ______ __  ___ ______                           //
//    /  |/  // ___/ / ____/           / ____//  |/  //_  __/                           //
//   / /|_/ / \__ \ / / __   ______   / /_   / /|_/ /  / /                              //
//  / /  / / ___/ // /_/ /  /_____/  / __/  / /  / /  / /                               //
// /_/  /_/ /____/ \____/           /_/    /_/  /_/  /_/                                //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __MSG_FMT_H__
#define __MSG_FMT_H__

#include "esp_system.h"
#include "esp_log.h"
#include "hub_error.h"

#include "cJSON.h"

#include "messages.h"

#define MINIMUN_JSON_PAYLOAD 14
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

 /*FUNCTION****************************************/
 /*msg_fmt_init Initializes msg raw interfaces, ssigns input buffers for
 differential sizing and starts up semaphores*/
void msg_fmt_init(void);

/*FUNCTION****************************************/
/*msg_fmt_i_task compatible freeRTOS executable task for input message managing*/
/*Parameters*************************************
param: standarf freertos param*/
void msg_fmt_i_task(void *params);

/*FUNCTION****************************************/
/*msg_fmt_o_task compatible freeRTOS executable task for output message managing*/
/*Parameters*************************************
param: standarf freertos param*/
void msg_fmt_o_task(void *params);

/*FUNCTION****************************************/
/*msg_fmt_configure Initializes msg raw interfaces, ssigns input buffers for
differential sizing and starts up semaphores*/
void msg_fmt_configure(void);

/*FUNCTION****************************************/
/*msg_fmt_install Sets the callback to be called upon receiving a message from the
corrispondent channel*/
/*Parameters**************************************/
/*topic: target Channel for the callback installation*/
/*cb: function to be called upon receiving a message from that channel*/
/*Returns**************************************/
/*Success if callback was successfully called*/
typedef void (*msg_fmt_callback_t)(void);
uint8_t msg_fmt_install(messages_channel_t topic, msg_fmt_callback_t cb);

/***
 *     _            __  __                                                                        _
 *    | |          / _|/ _|                                                                      | |
 *    | |__  _   _| |_| |_ ___ _ __   _ __ ___   __ _ _ __   __ _  __ _  ___ _ __ ___   ___ _ __ | |_
 *    | '_ \| | | |  _|  _/ _ \ '__| | '_ ` _ \ / _` | '_ \ / _` |/ _` |/ _ \ '_ ` _ \ / _ \ '_ \| __|
 *    | |_) | |_| | | | ||  __/ |    | | | | | | (_| | | | | (_| | (_| |  __/ | | | | |  __/ | | | |_
 *    |_.__/ \__,_|_| |_| \___|_|    |_| |_| |_|\__,_|_| |_|\__,_|\__, |\___|_| |_| |_|\___|_| |_|\__|
 *                                                                 __/ |
 *                                                                |___/
 */

 /*FUNCTION****************************************/
 /*msg_fmt_message_static Read the latest message received in the indicated
 channel*/
 /*Parameters**************************************/
 /*channel: Channel of the target message*/
 /*Returns**************************************/
 /*Pointer to the message, as an unformated string*/
char  *msg_fmt_message_static(messages_channel_t channel);

/*FUNCTION****************************************/
/*msg_fmt_raw_cb is the default buffer to be installed in msg_raw for msg
dispatching*/
/*Parameters**************************************/
/*channel: msg raw will indicate here the channel of the received message*/
/*payload: Payload of the callback*/
void    msg_fmt_raw_cb(messages_channel_t channel, char *payload);

/*FUNCTION****************************************/
/*TEST_reset_iassignment ire-assign original buffers*/
void TEST_reset_iassignment(void);

/***
 *                     _             _                                                                 _
 *                    | |           | |                                                               | |
 *      ___ ___  _ __ | |_ ___ _ __ | |_   _ __ ___   __ _ _ __   __ _  __ _  ___ _ __ ___   ___ _ __ | |_
 *     / __/ _ \| '_ \| __/ _ \ '_ \| __| | '_ ` _ \ / _` | '_ \ / _` |/ _` |/ _ \ '_ ` _ \ / _ \ '_ \| __|
 *    | (_| (_) | | | | ||  __/ | | | |_  | | | | | | (_| | | | | (_| | (_| |  __/ | | | | |  __/ | | | |_
 *     \___\___/|_| |_|\__\___|_| |_|\__| |_| |_| |_|\__,_|_| |_|\__,_|\__, |\___|_| |_| |_|\___|_| |_|\__|
 *                                                                      __/ |
 *                                                                     |___/
 */

 /*FUNCTION****************************************/
 /*_check_jSONField Checks if a certain key is present in the provided object*/
 /*Parameters**************************************/
 /*object: Object to check*/
 /*key: Key to look for*/
 /*Returns**************************************/
 /*HUB_OK If field was found*/
hub_error_t _check_jSONField(cJSON *object, char *key);

/*FUNCTION****************************************/
/*_get_jSONField Get a certain element in the provided object*/
/*Parameters**************************************/
/*object: Object to check*/
/*key: Key of element to get*/
/*Returns**************************************/
/*Object if found, null otherwise*/
cJSON *_get_jSONField(cJSON *object, char *key);

/*FUNCTION****************************************/
/*check_ifString Checks if the object of a certain key in the provided root object
is a string*/
/*Parameters**************************************/
/*root: Object to look for key*/
/*key: Key of element to check if it is a string*/
/*Returns**************************************/
/*HUB OK If element was a string*/
hub_error_t check_ifString(cJSON * root,char * key);

/*FUNCTION****************************************/
/*check_field_content Check if a string corresponds with any of the ones in the
reference list*/
/*Parameters**************************************/
/*str_tocheck: String to check for similarity*/
/*reference_list: List of permitted strings*/
/*total_elems: Total elements in reference list*/
/*Returns**************************************/
/*HUB OK If element matches any of the one of the list*/
hub_error_t check_field_content(char * str_tocheck, char ** reference_list, uint8_t total_elems);

/*FUNCTION****************************************/
/*stash_jSON_msg If msg still holds an object, this function will delete it*/
/*Parameters**************************************/
/*msg: Object to delete if possible*/
void stash_jSON_msg(cJSON * msg);
/***
 *     _                   _                _
 *    | |                 | |              | |
 *    | | ___   ___  _ __ | |__   __ _  ___| | __
 *    | |/ _ \ / _ \| '_ \| '_ \ / _` |/ __| |/ /
 *    | | (_) | (_) | |_) | |_) | (_| | (__|   <
 *    |_|\___/ \___/| .__/|_.__/ \__,_|\___|_|\_\
 *                  | |
 *                  |_|
 */

 /*FUNCTION****************************************/
 /*msg_fmt_test_cb Pushes a message in the channel buffer*/
 /*Parameters**************************************/
 /*topic: Test target channel*/
 /*msg: Test message*/
 void msg_fmt_test_cb(messages_channel_t topic,char * msg);

 /*FUNCTION****************************************/
 /*msg_fmt_loopback_execution Pushes a message in execution task, and sets the
 received flag ON*/
 void msg_fmt_loopback_execution(void);

 /*FUNCTION****************************************/
 /*msg_fmt_loopback_execution_prepare Loads a message with an intended action for the
 resource*/
 /*Parameters**************************************/
 /*resource: Resource that will execute the action*/
 /*action: Action to push*/
 void msg_fmt_loopback_execution_prepare(uint8_t resource, char * action);

 /*FUNCTION****************************************/
 /*msg_fmt_loopback_deployment is a test function to push a deployment payload in the buffer*/
 void msg_fmt_loopback_deployment(void);

/***
 *                                                        _     _
 *                                                       | |   | |
 *     _ __ ___  ___  __ _    __ _ ___ ___  ___ _ __ ___ | |__ | |_   _
 *    | '_ ` _ \/ __|/ _` |  / _` / __/ __|/ _ \ '_ ` _ \| '_ \| | | | |
 *    | | | | | \__ \ (_| | | (_| \__ \__ \  __/ | | | | | |_) | | |_| |
 *    |_| |_| |_|___/\__, |  \__,_|___/___/\___|_| |_| |_|_.__/|_|\__, |
 *                    __/ |                                        __/ |
 *                   |___/                                        |___/
 */

 /*FUNCTION****************************************/
 /*msg_fmt_load Reserves a channel to start a JSON message to publish in the indicated topic*/
 /*Parameters**************************************/
 /*topic: Target topic to start the message*/
 /*Returns**************************************/
 /*Success If the topic buffer is available*/
 uint8_t msg_fmt_load(messages_channel_t topic);

 /*FUNCTION****************************************/
 /*msg_fmt_edit_str Edits a string element in the JSON object  of a certaib channel*/
 /*Parameters**************************************/
 /*topic: Target channel to edit or append the string*/
/*key: Key of the string to edit or append*/
/*value: String to edit or append*/
 /*Returns**************************************/
 /*Success If the the request was fulfilled*/
 uint8_t msg_fmt_edit_str(messages_channel_t topic, char *key, char *value);

 /*FUNCTION****************************************/
 /*msg_fmt_edit_int Edits an int element in the JSON object  of a certaib channel*/
 /*Parameters**************************************/
 /*topic: Target channel to edit or append the int*/
/*key: Key of the int to edit or append*/
/*value: int to edit or append*/
 /*Returns**************************************/
 /*Success If the the request was fulfilled*/
 uint8_t msg_fmt_edit_int(messages_channel_t channel, char *key, uint32_t value);

 /*FUNCTION****************************************/
 /*msg_fmt_append_error Edits an int - error element in the JSON object  of a certaib channel*/
 /*Parameters**************************************/
 /*topic: Target channel to edit or append the int - error*/
/*key: Key of the int - error to edit or append*/
/*value: int - error to edit or append*/
 /*Returns**************************************/
 /*Success If the the request was fulfilled*/
 uint8_t msg_fmt_append_error(messages_channel_t channel, uint16_t value);

 /*FUNCTION****************************************/
 /*msg_fmt_append_int appends an int element in the JSON object  of a certaib channel*/
 /*Parameters**************************************/
 /*topic: Target channel to append the int*/
/*key: Key of the int to  append*/
/*value: int to  append*/
 /*Returns**************************************/
 /*Success If the the request was fulfilled*/
 uint8_t msg_fmt_append_int(messages_channel_t channel, char *key, uint32_t value);

 /*FUNCTION****************************************/
 /*msg_fmt_send pushes a message to be sent by msg raw. The command will push the
 default channel buffer in the default channel topic of the indicated channel*/
 /*Parameters**************************************/
 /*topic: Target topic to send msg*/
 /*Returns**************************************/
 /*Success If the the request was fulfilled*/
 uint8_t msg_fmt_send(messages_channel_t topic);



 /***
 *      ___        _     _       _   _
 *     / _ \      (_)   | |     | | (_)
 *    / /_\ \_ __  _  __| | __ _| |_ _  ___  _ __
 *    |  _  | '_ \| |/ _` |/ _` | __| |/ _ \| '_ \
 *    | | | | | | | | (_| | (_| | |_| | (_) | | | |
 *    \_| |_/_| |_|_|\__,_|\__,_|\__|_|\___/|_| |_|
 *
 *
 */
 /*FUNCTION****************************************/
 /*msg_fmt_start_object_item will create an object to be anidated in the specified channel.
 For example, if I use this function in Monitoring, I will get something like:
 {
 HUb: <>
 lol: <>
   Object : {
   ...
  }
}*/
 /*Parameters**************************************/
 /*channel: Target mssage to append the object*/
  uint8_t msg_fmt_start_object_item(messages_channel_t channel);

  /*FUNCTION****************************************/
  /*msg_fmt_add_int_toarray appends an int element in the json object
  to be anidated in the channel message*/
  /*Parameters**************************************/
  /*topic: Target channel of the anidated object to append the int*/
 /*key: Key of the int to  append*/
 /*value: int to  append*/
  /*Returns**************************************/
  /*Success If the the request was fulfilled*/
 uint8_t msg_fmt_add_int_toarray(messages_channel_t channel, char *key, uint32_t value);

 /*FUNCTION****************************************/
 /*msg_fmt_add_str_toarray appends an string element in the json object
 to be anidated in the channel message*/
 /*Parameters**************************************/
 /*topic: Target channel of the anidated object to append the string*/
/*key: Key of the array to  append*/
/*value: string to  append*/
 /*Returns**************************************/
 /*Success If the the request was fulfilled*/
 uint8_t msg_fmt_add_str_toarray(messages_channel_t channel, char * key, char * value) ;

 /*FUNCTION****************************************/
 /*msg_fmt_append_array appends an array element in the json object
 to be anidated in the channel message*/
 /*Parameters**************************************/
 /*topic: Target channel of the anidated object to append the array*/
/*key: Key of the array to  append*/
/*value: array to  append*/
 /*Returns**************************************/
 /*Success If the the request was fulfilled*/
 uint8_t msg_fmt_append_array(messages_channel_t channel, char * key);

 /*FUNCTION****************************************/
 /*msg_format_add_object will effectively push the object to the channel msg.
 should be called after finishing assembling the object*/
 /*Parameters**************************************/
 /*channel: Target mssage to append the object*/
  /*key: Key of the object to append*/
 uint8_t msg_format_add_object(messages_channel_t channel, char * key);

#endif
