#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include "esp_system.h"

/*Default SSID nad PSWD keys*/
#define APSSID_IDX "apssid"//
#define APPSWD_IDX "appswd"//
/****************************/

/***
 *     _____         _
 *    |_   _|       | |
 *      | | __ _ ___| | __
 *      | |/ _` / __| |/ /
 *      | | (_| \__ \   <
 *      \_/\__,_|___/_|\_\
 */
/*FUNCTION****************************************/
/*connection_task ompatible freeRTOS executable task. This task survayles internet connection
with AP*/
void connection_task(void *pvParameters);

/***
 *     _____              __ _
 *    /  __ \            / _(_)
 *    | /  \/ ___  _ __ | |_ _  __ _
 *    | |    / _ \| '_ \|  _| |/ _` |
 *    | \__/\ (_) | | | | | | | (_| |
 *     \____/\___/|_| |_|_| |_|\__, |
 *                              __/ |
 *                             |___/
 */
/*FUNCTION****************************************/
/*connection_engage_ap Stop STA mode, engage AP mode*/
void connection_engage_ap(void);

/*FUNCTION****************************************/
/*connection_engage_ap Stop AP mode, engage STA mode*/
void connection_re_engage_sta(void);

/*<DEPRECATED> Now REGISTRATION handles connection commands*/
uint8_t connection_handleNextCommand();

/*FUNCTION****************************************/
/*connection_update Stop STA mode, engage AP mode*/
uint8_t connection_update();

/*FUNCTION****************************************/
/*connection_reset Reset STA stack for re-trying connection. Not necessary for
reconnection attempt*/
void    connection_reset(void);

/*FUNCTION****************************************/
/*<DEPRECATED> Now this command was merged to registration*/
void    connection_logHelp();
/// -----------------------------

#endif
