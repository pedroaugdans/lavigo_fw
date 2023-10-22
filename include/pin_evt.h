//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____ _   __            ______ _    __ ______                              //
//    / __ \ /  _// | / /           / ____/| |  / //_  __/                              //
//   / /_/ / / / /  |/ /  ______   / __/   | | / /  / /                                 //
//  / ____/_/ / / /|  /  /_____/  / /___   | |/ /  / /                                  //
// /_/    /___//_/ |_/           /_____/   |___/  /_/                                   //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __PIN_EVT_H__
#define __PIN_EVT_H__

#include "esp_system.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*Lapse between each event reading refresh*/
#define PIN_EVT_REFRESH_TIME 40 // milliseconds

/*Size of pin event buffer*/
#define PIN_EVT_MAX_EVENTS  256

#define IDIR  1
#define ODIR  0

/***********EVENT MANAGER DATA BANK********************/
typedef struct {
  uint16_t ref;     // Index given for event follow up & trasability
  uint8_t port;     // Event port
  uint8_t pin;      // Event pin
  uint8_t dir;      // Event to be output or input
  uint8_t level;    // Event logic level
  uint8_t status;   // Event queue status [Active, Pending, Inactive]
} pin_evt_event_t;

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
/*pin_evt_init Pin event initialization function (Emmpty for now)*/
void pin_evt_init(void);
/*FUNCTION****************************************/
/*clk_pit_task compatible freeRTOS executable task*/
/*Parameters*************************************
param: standarf freertos param*/
void pin_evt_task(void *params);
/*FUNCTION****************************************/
/*pin_evt_flush Disable all installed, pending and active events on queue*/
void pin_evt_flush(void);

/***
 *     _____             _             _
 *    /  __ \           | |           | |
 *    | /  \/ ___  _ __ | |_ _ __ ___ | |
 *    | |    / _ \| '_ \| __| '__/ _ \| |
 *    | \__/\ (_) | | | | |_| | | (_) | |
 *     \____/\___/|_| |_|\__|_|  \___/|_|
 *
 *
 */

 /*FUNCTION****************************************/
 /*pin_evt_register Register a new target port to install events. Notice that
 if a single port has several installed events, it will not be registered twice.*/
 /*Parameters**************************************/
 /*port: Target port for future events*/
void pin_evt_register(uint8_t port);

/*FUNCTION****************************************/
/*pin_evt_push Installs a new event on the event queue*/
/*Parameters**************************************/
/*ref: et a tracable identification number for the event*/
/*port: Target port for the events*/
/*dir: Target direction (input or output) for the event*/
/*level: Target level for the event*/
/*Returns**************************************/
/*TRUE if event was successfully pushed*/
bool pin_evt_push(uint16_t ref, uint8_t port, uint8_t dir, uint8_t level);

/*FUNCTION****************************************/
/*pin_evt_next Searches for the next ACTIVE event on the pin evt buffer*/
/*Parameters**************************************/
/*event: Event holder to be filled with next finished event*/
/*Returns**************************************/
/*TRUE if a finished event was found. False otherwise*/
bool pin_evt_next(pin_evt_event_t *event);

#endif
