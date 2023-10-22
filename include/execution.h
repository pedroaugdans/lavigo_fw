/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
 /***
  *     /$$$$$$$$                                          /$$     /$$
  *    | $$_____/                                         | $$    |__/
  *    | $$      /$$   /$$  /$$$$$$   /$$$$$$$ /$$   /$$ /$$$$$$   /$$  /$$$$$$  /$$$$$$$
  *    | $$$$$  |  $$ /$$/ /$$__  $$ /$$_____/| $$  | $$|_  $$_/  | $$ /$$__  $$| $$__  $$
  *    | $$__/   \  $$$$/ | $$$$$$$$| $$      | $$  | $$  | $$    | $$| $$  \ $$| $$  \ $$
  *    | $$       >$$  $$ | $$_____/| $$      | $$  | $$  | $$ /$$| $$| $$  | $$| $$  | $$
  *    | $$$$$$$$/$$/\  $$|  $$$$$$$|  $$$$$$$|  $$$$$$/  |  $$$$/| $$|  $$$$$$/| $$  | $$
  *    |________/__/  \__/ \_______/ \_______/ \______/    \___/  |__/ \______/ |__/  |__/
  *
  *
  *
  */
/*
 * File:   execution.h
 * Author: independent contractor
 *
 * Created on August 25, 2019, 5:34 PM
 */

#ifndef EXECUTION_H
#define EXECUTION_H
#include "msg_fmt.h"
#include "pin_seq.h"

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
/*execution_msg_q compatible freeRTOS executable task, handles dequeueing
of execution messages and publication*/
/*Parameters**************************************/
/*params: standarf freertos param*/
void execution_msg_q(void *pvParameters);
/*FUNCTION****************************************/
/*execution_task compatible freeRTOS executable task, handles execution events
and triggers*/
/*Parameters**************************************/
/*params: standarf freertos param*/
void execution_task(void *pvParameters);


uint8_t _test_performAction(cJSON *root, uint16_t resource);
void TEST_execution_cb(void);
void trigger_test(void);


#endif /* EXECUTION_H */
