/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   indication.h
 * Author: independent contractor
 *
 * Created on January 22, 2020, 11:40 AM
 */

#ifndef INDICATION_H
#define INDICATION_H
#include "stdio.h"
#include "stdbool.h"


typedef enum  {
    indication_0 = 0,
    indication_1,
    indication_2,
    indication_3,
    indication_4,
    indication_5,
    indication_6,
    indication_7,
    indication_8,
    indication_9,
    MAX_LED_INDICATIONS
}led_indication_t;

#define INDICATOR_STARTING1 {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}
#define INDICATOR_STARTING2 {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}
#define INDICATOR_STARTING3 {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}
#define INDICATOR_STARTING4 {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}
#define INDICATOR_STARTING5 {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}
#define INDICATOR_STARTING6 {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}
#define INDICATOR_STARTING7 {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}
#define INDICATOR_STARTING8 {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}
#define INDICATOR_STARTING9 {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}
#define INDICATOR_STARTING10 {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}

#define LED_INDICATION_SPACES 10

void indication_task(void *params);

#endif /* INDICATION_H */

