//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____ _   __            _____  ______ ____                                 //
//    / __ \ /  _// | / /           / ___/ / ____// __ \                                //
//   / /_/ / / / /  |/ /  ______    \__ \ / __/  / / / /                                //
//  / ____/_/ / / /|  /  /_____/   ___/ // /___ / /_/ /                                 //
// /_/    /___//_/ |_/            /____//_____/ \___\_\                                 //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __PIN_SEQ_H__
#define __PIN_SEQ_H__

#include "esp_system.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "machines.h"

#define PIN_SEQ_REFRESH_TIME              50  // milliseconds

#define PIN_SEQ_MAX_SEQUENCES            128  // 4 per [resource + retrofit]
#define PIN_SEQ_MAX_CHARS_PER_SEQUENCE    64
#define PIN_SEQ_MAX_SIGNALS_PER_SEQUENCE   8

typedef struct {
  machines_machine_t  *machine_p;   //Machine responsible for the sequence
  machines_sequence_t *sequence_p;  //Sequence in the queue
  uint8_t index;                    //Index of the current event of the sequence
  uint8_t status;                   //Status of the sequence, Pending, Active, Inactive or Done
} pin_seq_sequence_t;
/***
 *     _____
 *    |  _  |
 *    | | | |_   _  ___ _   _  ___  ___
 *    | | | | | | |/ _ \ | | |/ _ \/ __|
 *    \ \/' / |_| |  __/ |_| |  __/\__ \
 *     \_/\_\\__,_|\___|\__,_|\___||___/
 *
 *
 */

 /*FUNCTION****************************************/
 /*queues_flush task delete all remaining sequencies in the all the queue*/
void queues_flush(void);

/*FUNCTION****************************************/
/*pin_seq_init task initialization, empty for now*/
void pin_seq_init(void);

/*FUNCTION****************************************/
/*pin_seq_task compatible freeRTOS executable task*/
/*Parameters*************************************
param: standarf freertos param*/
void pin_seq_task(void *params);
/***
 *                               _
 *                              | |
 *     _ __ ___ _ __   ___  _ __| |_
 *    | '__/ _ \ '_ \ / _ \| '__| __|
 *    | | |  __/ |_) | (_) | |  | |_
 *    |_|  \___| .__/ \___/|_|   \__|
 *             | |
 *             |_|
 */

 /*FUNCTION****************************************/
 /*pin_seq_report_get  get the data of the sequence in the seq_index space
 of the buffer*/
 /*Parameters**************************************/
 /*seq_index: Index of the sequence to fetch*/
 /*seq_to_report: Data holder from sequence roport*/
 /*Returns**************************************/
 /*TRUE if fulfillment successful*/
uint8_t pin_seq_report_get(uint16_t seq_index,pin_seq_sequence_t* seq_to_report);

/*FUNCTION****************************************/
/*pin_seq_report_get_next  get the data of the next index needed*/
/*Parameters**************************************/
/*seq_to_report: Data holder from sequence roport*/
/*Returns**************************************/
/*TRUE if there is a necessary fetch*/
uint8_t pin_seq_report_get_next(pin_seq_sequence_t* seq_to_report);

/*FUNCTION****************************************/
/*pin_seq_report_seq_unset  Remove sequence in index space to avoid reporting it*/
/*Parameters**************************************/
/*uint16_t: Index to disable reporting*/
uint8_t pin_seq_report_seq_unset(uint16_t seq_index);

/*FUNCTION****************************************/
/*pin_seq_report_machine_seq_set  Set information request to reports
of the machine indicated in macine_name*/
/*Parameters**************************************/
/*machine_name: Machine to report sequencies*/
uint8_t pin_seq_report_machine_seq_set(uint8_t machine_name);

/*FUNCTION****************************************/
/*any_sequence_need_report_get  Check if any sequence needs reporting*/
/*Returns**************************************/
/*TRUE if there is any sequence that needs reporting*/
uint8_t any_sequence_need_report_get(void);

/***
 *
 *
 *     _ __ ___   __ _ _ __   __ _  __ _  ___
 *    | '_ ` _ \ / _` | '_ \ / _` |/ _` |/ _ \
 *    | | | | | | (_| | | | | (_| | (_| |  __/
 *    |_| |_| |_|\__,_|_| |_|\__,_|\__, |\___|
 *                                  __/ |
 *                                 |___/
 */

 /*FUNCTION****************************************/
 /*queues_flush task delete all remaining sequencies in the sequencies queue*/
 void pin_seq_flush(void);

 /*FUNCTION****************************************/
 /*pin_seq_push Pushes a new Active Sequence in the sequence queue*/
 /*Parameters**************************************/
 /*machine_p: Machine responsible for the sequence*/
 /*sequence_p: Sequence to be pushed*/
 /*Returns**************************************/
 /*TRUE if sequence was successfully pushed*/
 bool pin_seq_push(machines_machine_t *machine_p, machines_sequence_t* sequence_p);

 /*FUNCTION****************************************/
 /*pin_seq_next Dispatches and absorves next Done sequence.*/
 /*Parameters**************************************/
 /*sequence: Holder for the dispatched sequence*/
 /*Returns**************************************/
 /*TRUE if a sequence was dispatched or absorved*/
 uint8_t pin_seq_next(pin_seq_sequence_t *sequence);
#endif
