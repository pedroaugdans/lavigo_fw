//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____   ____ _    __ ______ ____              _       __ ____ ______ ____  //
//    / __ \ / __ \ /  _/| |  / // ____// __ \            | |     / //  _// ____//  _/  //
//   / / / // /_/ / / /  | | / // __/  / /_/ /  ______    | | /| / / / / / /_    / /    //
//  / /_/ // _, _/_/ /   | |/ // /___ / _, _/  /_____/    | |/ |/ /_/ / / __/  _/ /     //
// /_____//_/ |_|/___/   |___//_____//_/ |_|              |__/|__//___//_/    /___/     //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __DRV_WIFI_H__
#define __DRV_WIFI_H__

//////////////////////////////////////////////////////////////////////////////////////////
// REQUIRES drv_nvs_init();                                                             //
//////////////////////////////////////////////////////////////////////////////////////////

#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "hub_error.h"

//#define WIFI_USE_DEFAULT
#define WIFI_DEFAULT_SSID "lavigo"
#define WIFI_DEFAULT_PSWD "chaussettes"

#define MAX_STA_CONN    5

typedef enum {
    Online = 0,       // event called upon receiving an IP adress from the AP
    Offline = 1,      // event called upon an unsuccessful connection attempt or an
    //unsuccessful pin response from target AP
    Apset,            //unused
    apDown,           //unused
    NOF_WIFI_EVENTS,
} drv_wifi_event_t;

typedef void (*server_callback_t) (void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data);

/***
 *     _____ _        _   _                                   _
 *    /  ___| |      | | (_)                                 | |
 *    \ `--.| |_ __ _| |_ _  ___  _ __    _ __ ___   ___   __| | ___
 *     `--. \ __/ _` | __| |/ _ \| '_ \  | '_ ` _ \ / _ \ / _` |/ _ \
 *    /\__/ / || (_| | |_| | (_) | | | | | | | | | | (_) | (_| |  __/
 *    \____/ \__\__,_|\__|_|\___/|_| |_| |_| |_| |_|\___/ \__,_|\___|
 *
 *
 */
 /*FUNCTION****************************************/
hub_error_t drv_wifi_connect(void);
 /*drv_wifi_disconnect forces a disconnection from the AP*/
hub_error_t drv_wifi_disconnect(void);
/*drv_wifi_stop Dissassembles the WiFi stack, with the DHCP client stack in it*/
hub_error_t drv_wifi_stop(void);
/*drv_wifi_check returns SUCCESS if wifi is connected*/
uint8_t drv_wifi_check(void);

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
 /*drv_wifi_init configures STA mode and stablishes main events, assembles DHCP
 and LWiP TCP/IP Stack*/
 void drv_wifi_init(void);
/*FUNCTION****************************************/
/*drv_wifi_configure Configures keys which the device will use in STA mode
to attempt a connection*/
/*Parameters**************************************/
/*ssid: target Ap Name*/
/*pswd: Target AP password, WPA2 security by default*/
void drv_wifi_configure(char *ssid, char *pswd);
/*FUNCTION****************************************/
/*drv_wifi_install sets a callback which will be called upon the indicated event
drv_wifi_event_t*/
/*Parameters**************************************/
/*callback: callback to call*/
/*event: Event which will happen immediately before calling the callback*/
typedef void (*drv_wifi_callback_t)(void);
void drv_wifi_install(drv_wifi_callback_t callback, drv_wifi_event_t event);

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
 /*<Deprecated>*/
hub_error_t drv_wifi_AP(void);
hub_error_t drv_ap_event_install(void * generic_arg);
void drv_wifi_server_installl(drv_wifi_event_t callback,server_callback_t event);


#endif
