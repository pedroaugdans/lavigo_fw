//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____   ____ _    __ ______ ____              __  ___ ____  ______ ______  //
//    / __ \ / __ \ /  _/| |  / // ____// __ \            /  |/  // __ \/_  __//_  __/  //
//   / / / // /_/ / / /  | | / // __/  / /_/ /  ______   / /|_/ // / / / / /    / /     //
//  / /_/ // _, _/_/ /   | |/ // /___ / _, _/  /_____/  / /  / // /_/ / / /    / /      //
// /_____//_/ |_|/___/   |___//_____//_/ |_|           /_/  /_/ \___\_\/_/    /_/       //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __DRV_MQTT_H__
#define __DRV_MQTT_H__

#include "esp_system.h"
#include "esp_log.h"

typedef enum {
  Connected    = 0, // event called upon a successful attempt of connection
  Disconnected = 1, // event called upon a disconnection
  Messaged     = 2, // event called upon message received
  ConnFailed   = 3, // event in which a connection attempt fails
  NOF_MQTT_EVENTS
} drv_mqtt_event_t;

typedef struct {
  char *topic;
  uint8_t tlen;
  char *payload;
  uint16_t plen;
} drv_mqtt_message_t;


/***
 *     _            _                                                                   _
 *    | |          | |                                                                 | |
 *    | |_ __ _ ___| | __   _ __ ___   __ _ _ __   __ _  __ _  ___ _ __ ___   ___ _ __ | |_
 *    | __/ _` / __| |/ /  | '_ ` _ \ / _` | '_ \ / _` |/ _` |/ _ \ '_ ` _ \ / _ \ '_ \| __|
 *    | || (_| \__ \   <   | | | | | | (_| | | | | (_| | (_| |  __/ | | | | |  __/ | | | |_
 *     \__\__,_|___/_|\_\  |_| |_| |_|\__,_|_| |_|\__,_|\__, |\___|_| |_| |_|\___|_| |_|\__|
 *                                                       __/ |
 *                                                      |___/
 */
 /*FUNCTION****************************************/
 /*drv_mqtt_init creates semaphore for mqtt task management*/
void drv_mqtt_init(void);
/*FUNCTION****************************************/
/*drv_mqtt_task ompatible freeRTOS executable task. This task will call the Yield operation.
Yield operation  will complete subscriptions, will internaly return ACK [Q0S1] and will call
the topic subscription callback upon receiving a message*/
void drv_mqtt_task(void *params);
/*FUNCTION****************************************/
/*drv_mqtt_reset Disconnects MQTT from server*/
void drv_mqtt_reset(void);

/***
 *                                     _   _
 *                                    | | (_)
 *      ___ ___  _ __  _ __   ___  ___| |_ _  ___  _ __
 *     / __/ _ \| '_ \| '_ \ / _ \/ __| __| |/ _ \| '_ \
 *    | (_| (_) | | | | | | |  __/ (__| |_| | (_) | | | |
 *     \___\___/|_| |_|_| |_|\___|\___|\__|_|\___/|_| |_|
 *
 *
 */
 /*FUNCTION****************************************/
 /*drv_mqtt_configure Configures MQTT behaviour, sets the keys from flash
 to RAM and creates initial MQTT stack*/
uint8_t drv_mqtt_configure(void);

/*FUNCTION****************************************/
/*drv_mqtt_configure Attempts a connection with the loaded information in
drv_mqtt_configure()*/
uint8_t drv_mqtt_connect(void);

/*drv_mqtt_reconnect Attempts a reconnection with the loaded information in
drv_mqtt_configure, called by connection task (manually)*/
uint8_t drv_mqtt_reconnect(void);

/*returns MQTT CONNECTION STATUS*/
uint8_t drv_mqtt_check(void);
void force_disconnection(void);

/***
 *     _____ _                            _
 *    /  __ \ |                          | |
 *    | /  \/ |__   __ _ _ __  _ __   ___| |
 *    | |   | '_ \ / _` | '_ \| '_ \ / _ \ |
 *    | \__/\ | | | (_| | | | | | | |  __/ |
 *     \____/_| |_|\__,_|_| |_|_| |_|\___|_|
 *
 *
 */

 /*FUNCTION****************************************/
 /*drv_mqtt_subscribe Attempts a subscription a the specified topic. Upon message
 reception, the registered single message callback will be called.*/
 /*Parameters**************************************/
 /*topic: topic in which the messages will be fetched*/
 /*Returns**************************************/
 /*SUCCESS if subscription request was successful. The subscription fulfillment
 will be done in the next YIELD cycle*/
uint8_t drv_mqtt_subscribe(const char *topic);

/*FUNCTION****************************************/
/*drv_mqtt_publish Attempts a publication a the specified topic.*/
/*Parameters**************************************/
/*topic: topic in which the message will be published*/
/*message: unformated STRING to publish.*/
/*Returns**************************************/
/*SUCCESS if message publish is successful*/
uint8_t drv_mqtt_publish  (const char *topic, const char *message);

typedef void (*drv_mqtt_callback_t)(drv_mqtt_message_t *);
/*FUNCTION****************************************/
/*drv_mqtt_install Installs a callback to be called upon the indicated*/
/*drv_mqtt_event_t event.*/
/*Parameters**************************************/
/*topic: topic in which the message will be published*/
/*message: unformated STRING to publish.*/
/*Returns**************************************/
/*SUCCESS if message publish is successful*/
void drv_mqtt_install(drv_mqtt_callback_t callback, drv_mqtt_event_t event);

#endif
