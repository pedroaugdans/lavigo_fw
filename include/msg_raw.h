//////////////////////////////////////////////////////////////////////////////////////////
//     __  ___ _____  ______            ____   ___  _       __                          //
//    /  |/  // ___/ / ____/           / __ \ /   || |     / /                          //
//   / /|_/ / \__ \ / / __   ______   / /_/ // /| || | /| / /                           //
//  / /  / / ___/ // /_/ /  /_____/  / _, _// ___ || |/ |/ /                            //
// /_/  /_/ /____/ \____/           /_/ |_|/_/  |_||__/|__/                             //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __MSG_RAW_H__
#define __MSG_RAW_H__

#include "esp_system.h"
#include "esp_log.h"

#include "messages.h"

#define TO_HUB   1
#define FROM_HUB 0

/*FUNCTION****************************************/
/*msg_raw_init Intializes buffers pointers (to have an array with elements of
differente sizes) and installs the main msg ccallback on mqtt msg*/
void msg_raw_init(void);

/***
 *     _       _             __
 *    (_)     | |           / _|
 *     _ _ __ | |_ ___ _ __| |_ __ _  ___ ___
 *    | | '_ \| __/ _ \ '__|  _/ _` |/ __/ _ \
 *    | | | | | ||  __/ |  | || (_| | (_|  __/
 *    |_|_| |_|\__\___|_|  |_| \__,_|\___\___|
 *
 *
 */

/*FUNCTION****************************************/
/*msg_raw_topic returns a pointer to a string cointaining the target topic for a specified
channel in the specified data flow direction (downlink - from hub, or to hub)*/
/*Parameters**************************************/
/*channel: target Channel to aim with the topic*/
/*downlink: data flow to complete the topi*/
/*Returns**************************************/
/*POinter to the string containing the topic*/
char *msg_raw_topic  (messages_channel_t channel, bool downlink);

/*FUNCTION****************************************/
/*msg_raw_message returns a pointer to an unformatted string, which holds the
last message in the buffer correspondent to the channel and data flow indicated*/
/*Parameters**************************************/
/*channel: target Channel of the requested message*/
/*downlink: data flow direction of the requested channel*/
/*Returns**************************************/
/*POinter to requested message*/
char *msg_raw_message(messages_channel_t channel, bool downlink);

/***
 *                     _
 *                    | |
 *     _ __   ___  ___| |_ _ __ ___   __ _ _ __
 *    | '_ \ / _ \/ __| __| '_ ` _ \ / _` | '_ \
 *    | |_) | (_) \__ \ |_| | | | | | (_| | | | |
 *    | .__/ \___/|___/\__|_| |_| |_|\__,_|_| |_|
 *    | |
 *    |_|
 */

 /*FUNCTION****************************************/
 /*msg_raw_install assigns a centralized callback which will dispatch all the messages
 received from MQTT channel of the hub*/
 /*Parameters**************************************/
 /*callback: Callback to be called upon receiving a message. The callback will receive
 the channel and payload of the message*/
typedef void (*msg_raw_callback_t) (messages_channel_t channel, char *payload);
uint8_t msg_raw_install(msg_raw_callback_t callback);

/*FUNCTION****************************************/
/*msg_raw_send will push a msg to the MQTT publication buffer, to the specified topic*/
/*Parameters**************************************/
/*topic: Target topic for the message*/
/*message: Message to be published*/
/*Returns**************************************/
/*Success if message was successfully pushed*/
uint8_t msg_raw_send (char *topic, char *message);

#endif
