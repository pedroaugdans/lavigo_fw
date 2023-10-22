#ifndef __MONITORING_H__
#define __MONITORING_H__

/***
 *     /$$      /$$  /$$$$$$  /$$   /$$ /$$$$$$ /$$$$$$$$ /$$$$$$  /$$$$$$$  /$$$$$$ /$$   /$$ /$$$$$$$$
 *    | $$$    /$$$ /$$__  $$| $$$ | $$|_  $$_/|__  $$__//$$__  $$| $$__  $$|_  $$_/| $$$ | $$|__  $$__/
 *    | $$$$  /$$$$| $$  \ $$| $$$$| $$  | $$     | $$  | $$  \ $$| $$  \ $$  | $$  | $$$$| $$   | $$
 *    | $$ $$/$$ $$| $$  | $$| $$ $$ $$  | $$     | $$  | $$  | $$| $$$$$$$/  | $$  | $$ $$ $$   | $$
 *    | $$  $$$| $$| $$  | $$| $$  $$$$  | $$     | $$  | $$  | $$| $$__  $$  | $$  | $$  $$$$   | $$
 *    | $$\  $ | $$| $$  | $$| $$\  $$$  | $$     | $$  | $$  | $$| $$  \ $$  | $$  | $$\  $$$   | $$
 *    | $$ \/  | $$|  $$$$$$/| $$ \  $$ /$$$$$$   | $$  |  $$$$$$/| $$  | $$ /$$$$$$| $$ \  $$   | $$
 *    |__/     |__/ \______/ |__/  \__/|______/   |__/   \______/ |__/  |__/|______/|__/  \__/   |__/
 *
 *
 *
 */

 /*FUNCTION****************************************/
 /*monitoring_task ompatible freeRTOS executable task. This task will survail every other task for
 available messages to send to the cloud platform related to the device, that is to say,
  unrelated to the controlled devices*/
void monitoring_task(void *pvParameters);

/*FUNCTION****************************************/
/*dump_recovery_test trigger dump and recovery from console, for integration tests*/
void dump_recovery_test(void);

#include "drv_locks.h"

extern SemaphoreHandle_t monitoring_msg_lock;
#define HEARTBEAT_MONITOR   1024
#define REPORT_MONITOR   1024



/*Monitoring callback, enquired from platform*/
typedef enum {
    fallback_enquiry,
    running_enquiry
} msg_enquiry_t;

/*4*/
#define FALLBACK_MODE "fallback"
#define SILENCE_MODE "silence"
#define BOOTUP_MODE "bootup"
#define REPORT_MODE "report"



#endif
