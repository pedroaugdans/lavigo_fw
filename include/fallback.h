/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
 /***
  *     /$$$$$$$$ /$$$$$$  /$$       /$$       /$$$$$$$   /$$$$$$   /$$$$$$  /$$   /$$
  *    | $$_____//$$__  $$| $$      | $$      | $$__  $$ /$$__  $$ /$$__  $$| $$  /$$/
  *    | $$     | $$  \ $$| $$      | $$      | $$  \ $$| $$  \ $$| $$  \__/| $$ /$$/
  *    | $$$$$  | $$$$$$$$| $$      | $$      | $$$$$$$ | $$$$$$$$| $$      | $$$$$/
  *    | $$__/  | $$__  $$| $$      | $$      | $$__  $$| $$__  $$| $$      | $$  $$
  *    | $$     | $$  | $$| $$      | $$      | $$  \ $$| $$  | $$| $$    $$| $$\  $$
  *    | $$     | $$  | $$| $$$$$$$$| $$$$$$$$| $$$$$$$/| $$  | $$|  $$$$$$/| $$ \  $$
  *    |__/     |__/  |__/|________/|________/|_______/ |__/  |__/ \______/ |__/  \__/
  *
  *
  *
  */
/*
 * File:   fallback.h
 * Author: independent contractor
 *
 * Created on February 7, 2020, 3:19 PM
 */

#ifndef FALLBACK_H
#define FALLBACK_H

/*FUNCTION****************************************/
/*drv_fallback_task compatible freeRTOS executable task, handles fallback copying
and pasting signals*/
/*Parameters**************************************/
/*params: standarf freertos param*/
void drv_fallback_task(void *pvParameters);

/*FUNCTION****************************************/
/*fallback_msg_q compatible freeRTOS executable task, handles dequeueing
of fallback messages and publication*/
/*Parameters**************************************/
/*params: standarf freertos param*/
/*<Function was deprecated>*/
void fallback_msg_q(void *pvParameters);

#endif /* FALLBACK_H */
