
#ifndef __PORT_EVT_H__
#define __PORT_EVT_H__

/***
 *     /$$$$$$$   /$$$$$$  /$$$$$$$  /$$$$$$$$       /$$   /$$ /$$$$$$  /$$$$$$
 *    | $$__  $$ /$$__  $$| $$__  $$|__  $$__/      | $$  / $$|_  $$_/ /$$__  $$
 *    | $$  \ $$| $$  \ $$| $$  \ $$   | $$         |  $$/ $$/  | $$  | $$  \ $$
 *    | $$$$$$$/| $$  | $$| $$$$$$$/   | $$          \  $$$$/   | $$  | $$  | $$
 *    | $$____/ | $$  | $$| $$__  $$   | $$           >$$  $$   | $$  | $$  | $$
 *    | $$      | $$  | $$| $$  \ $$   | $$          /$$/\  $$  | $$  | $$  | $$
 *    | $$      |  $$$$$$/| $$  | $$   | $$         | $$  \ $$ /$$$$$$|  $$$$$$/
 *    |__/       \______/ |__/  |__/   |__/         |__/  |__/|______/ \______/
 *
 *
 *
 */

#include "esp_system.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PIN_EVT_MAX_EVENTS  256 // 8 per [resource + retrofit]
#define PIN_EVT_MAX_PINS    128

#define IDIR  1
#define ODIR  0

/***********EVENT MANAGER DATA BANK********************/

typedef struct {
  uint8_t port;         //Port to be characterized
  uint8_t status;       //Active or Low
  uint8_t has_been_read;//True if it's information has been feteched by any service
  uint8_t last_idx;     //Last low-level buffer index harvested (abstracted by this layer)
  uint32_t timestamp;   //Timestamp of last harvesting event
} port_status_t;

/*hub_machines_input_ports_list is an iterable in which each element will be
a physical machine input port, according to the layout*/
extern uint8_t hub_machines_input_ports_list[32];

/*hub_machines_output_ports_list is an iterable in which each element will be
a physical machine output port, according to the layout*/
extern uint8_t hub_machines_output_ports_list[32];

/*hub_retrofit_input_ports_list is an iterable in which each element will be
a physical retrofit input port, according to the layout*/
extern uint8_t hub_retrofit_input_ports_list[32];

/*hub_retrofit_output_ports_list is an iterable in which each element will be
a physical retrofit output port, according to the layout*/
extern uint8_t hub_retrofit_output_ports_list[32];

/*<UNUSED>. This arrays where meant to implement the machine-wise fallback
feature*/
extern uint8_t fallback_for_machines[32];
extern uint8_t fallback_for_retrofit[32];

/*port_xio_init****************************************/
/*pin_xio_init Prepares task semaphores,and  fillsthe iterable port arrays*/
void port_xio_init(void);
/*FUNCTION****************************************/
/*pin_in_task ompatible freeRTOS executable task for refreshing ports*/
void port_xio_task(void *params);

/*FUNCTION****************************************/
/*port_look_for_status Will look for the indicated status. If found, it will
mark the port as read to avoid re-reading.*/
/*Parameters**************************************/
/*port: Target port to seek status*/
/*status: Status being looked for*/
/*Returns successs if status was found, FAILURE otherwise*/
uint8_t port_look_for_status(uint8_t port,bool status);

/*FUNCTION****************************************/
/*port_set Sets the value of a specific port which will be
 printed in the device on the next available timestam of the output
buffer*/
/*Parameters**************************************/
/*port: Target port*/
/*status: Status to be printed*/
void port_set(uint8_t port, bool new_status)__attribute__((unused));

/*FUNCTION****************************************/
/*get_next_xio_msg  Fills the provided memory space (port_status_to_return)
with the next available to log information.*/
/*Parameters**************************************/
/*port_status_Memory space to fill*/
/*Returns**************************************/
/*Success if there is a necessary fetch*/
uint8_t get_next_xio_msg(port_status_t * port_status_to_return);

/*FUNCTION****************************************/
/*set_logging Enables or desables logging functions*/
/*Parameters**************************************/
/*should_i_log: Value of logging variable*/
uint8_t set_logging(uint8_t should_i_log);

/*FUNCTION****************************************/
/*get_logging CH=hecks if logging is enabled*/
/*Returns SUCCESS if logging is enabled*/
uint8_t get_logging(void);

/*FUNCTION****************************************/
/*is_any_msg checks if there is any message to log*/
/*Returns TRUE if a message was found*/
bool is_any_msg(void);

#endif
