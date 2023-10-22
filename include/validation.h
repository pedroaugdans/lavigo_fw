#ifndef __VALIDATION_H__
#define __VALIDATION_H__
#include "esp_system.h"
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

 /*FUNCTION****************************************/
 /*validation_task compatible freeRTOS executable task, survayles connectivity to cloud
 platform*/
 /*Parameters**************************************/
 /*params: standarf freertos param*/
void validation_task(void *pvParameters);

/*FUNCTION****************************************/
/*validation_failure_cb handles the disconnection of MQTT layer, in the cloud - platform layer*/
void validation_failure_cb();

/*ISOLATION test callback for connection FSM*/
void TEST_validation_success_cb(void);

/*Isolation test callback for auto-validating*/
uint8_t check_validation_pending(void);

#endif
