//////////////////////////////////////////////////////////////////////////////////////////
//    ______ __     __ __             ____   ____ ______                                //
//   / ____// /    / //_/            / __ \ /  _//_  __/                                //
//  / /    / /    / ,<     ______   / /_/ / / /   / /                                   //
// / /___ / /___ / /| |   /_____/  / ____/_/ /   / /                                    //
// \____//_____//_/ |_|           /_/    /___/  /_/                                     //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLK_PIT_H__
#define __CLK_PIT_H__

#include "esp_system.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define CLK_PIT_REFRESH_TIME  20  // milliseconds
#define CLK_PIT_MIN_INTERVAL 100  // milliseconds

#define CLK_PIT_MAX_EVENTS    64  // 2 per [resource + retrofit]

/***********EVENT MANAGER DATA BANK********************/

typedef struct {
  uint16_t ticks;     /*Physical amount of click ticks cycles remaining*/
  uint16_t ref;       /*<unused> reference of trazability and printability*/
  uint8_t status;     /*DONE, ACTIVE and INACTIVE*/
} clk_pit_event_t;

/*FUNCTION****************************************/
/*clk_pit_init task initialization*/
void clk_pit_init(void);

/*FUNCTION****************************************/
/*clk_pit_flush task delete all remaining events in the clock PIT queue*/
void clk_pit_flush(void);

/*FUNCTION****************************************/
/*clk_pit_task compatible freeRTOS executable task*/
/*Parameters*************************************
param: standarf freertos param*/
void clk_pit_task(void *params);

/*FUNCTION****************************************/
/*clk_pit_push Pushes a new clock event in the clock pit queue*/
/*Parameters**************************************/
/*ref: Set a tracable identification number for the event*/
/*hms: Amount of time to tick in Hundreds of MIli Seconds*/
/*Returns**************************************/
/*TRUE if event was successfully pushed*/
uint8_t clk_pit_push(uint16_t ref, uint16_t hms);

/*FUNCTION****************************************/
/*clk_pit_next Searches for the next ACTIVE event on the clk pit buffer*/
/*Parameters**************************************/
/*event: Event holder to be filled with next finished event*/
/*Returns**************************************/
/*TRUE if a finished event was found. False otherwise*/
bool clk_pit_next(clk_pit_event_t *event);

#endif
