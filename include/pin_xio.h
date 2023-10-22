//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____ _   __            _  __  ____ ____                                   //
//    / __ \ /  _// | / /           | |/ / /  _// __ \                                  //
//   / /_/ / / / /  |/ /  ______    |   /  / / / / / /                                  //
//  / ____/_/ / / /|  /  /_____/   /   | _/ / / /_/ /                                   //
// /_/    /___//_/ |_/            /_/|_|/___/ \____/                                    //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __PIN_XIO_H__
#define __PIN_XIO_H__

#include "esp_system.h"
#include "esp_log.h"
#include "drv_locks.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
/*Timestamp index for ping pong buffer model*/
typedef enum {
  timestamp_0 = 0,
  timestamp_1,
  timestamp_2,
  timestamp_3,
  timestamp_4,
  timestamp_5,
  MAX_INPUT_TIMESTAMP
} input_timestamp_t;

/*Buffer index for ping pong buffer model*/
typedef enum {
  ping_pong_0=0,
  ping_pong_1,
  PING_PONG_ID
} xio_ping_pong_t;

#define PORT_TO_PIN(port)     ((port     ) & 0x0F)
#define PORT_TO_ADAPTER(port) ((port >> 4) & 0x07)

#define PIN_XIO_REFRESH_TIME     10 // milliseconds

/*Low level - hardware related configurations*/
#define PIN_XIO_MAX_ADAPTERS     8
#define PIN_XIO_PINS_PER_ADAPTER 8
#define PIN_XIO_IODIRA_MASK      0x00
#define PIN_XIO_IODIRB_MASK      0xFF

/*Main GPIO expander IC - MCP23017 controls the
IO boards*/
#define I2C_MCP23017_ADDRESS     0b0100000
#define I2C_MCP23017_IODIRA      0x00
#define I2C_MCP23017_IODIRB      0X01
#define I2C_MCP23017_GPIOA       0x12
#define I2C_MCP23017_GPIOB       0x13

#define I2C_PCF8574_ADDRESS      0b0111000

/***
 *     _            _
 *    | |          | |
 *    | |_ __ _ ___| | __
 *    | __/ _` / __| |/ /
 *    | || (_| \__ \   <
 *     \__\__,_|___/_|\_\
 *
 *
 */
 /*FUNCTION****************************************/
 /*pin_xio_init Prepares task semaphores, and reads the available connected expanders.
 Detects the current status of the expanders, and fixes whichever expander is not
 ready for propper operation*/
void pin_xio_init(void);
/*FUNCTION****************************************/
/*pin_in_task ompatible freeRTOS executable task for reading input signals*/
void pin_in_task(void *params);
/*FUNCTION****************************************/
/*pin_in_task ompatible freeRTOS executable task for reading output signals*/
void pin_out_task(void *params);

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
 /*assign_refresh_cv Installs the indicated callback, which will act whenever
 the input reading task finishes filling a buffer, therefore toggling the buffer
 to fill next. This means a new buffer is available to read*/
 /*Parameters**************************************/
 /*new_refresh_cv: Callback called upon buffer toggling.*/
typedef void (*xio_refresh_cb)(void);
void assign_refresh_cv(xio_refresh_cb new_refresh_cv);

/*FUNCTION****************************************/
/*pin_xio_set Sets the value of a specific port, determined by adapter and pin,
which will be printed in the device on the next available timestam of the output
buffer*/
/*Parameters**************************************/
/*adapter: Target port adapter*/
/*pin: Target port adapter pin*/
/*status: Status to be printed*/
void pin_xio_set(uint8_t adapter, uint8_t pin, bool  status);

/*FUNCTION****************************************/
/*pin_xio_get Reads the value of a specific timestamp of a port, determined
by adapter and pin*/
/*Parameters**************************************/
/*adapter: Target port adapter*/
/*pin: Target port adapter pin*/
/*status: Container to save the read status*/
/*t_idx: specific timestamp within the last filled buffer to read*/
void pin_xio_get(uint8_t adapter, uint8_t pin, bool *status, uint8_t t_idx);

extern SemaphoreHandle_t xio_timestamp_lock[MAX_INPUT_TIMESTAMP];

#endif
