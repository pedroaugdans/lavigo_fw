/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   locks.h
 * Author: independent contractor
 *
 * Created on December 13, 2019, 12:22 AM
 */

#ifndef LOCKS_H
#define LOCKS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define LOCK_IMMEDIATE_INTERVAL 1
#define LOCK_ZERO_INTERVAL 10
#define LOCK_FIRST_INTERVAL 300
#define LOCK_SECOND_INTERVAL 800
#define LOCK_MSG_INTERVAL 2000

bool sph_create(SemaphoreHandle_t       *semaphore_to_take);
bool sph_give(SemaphoreHandle_t         *semaphore_to_take);
bool sph_step_retries(SemaphoreHandle_t *semaphore_to_take);
bool sph_take_imm(SemaphoreHandle_t     *semaphore_to_take);
bool sph_take_delayed(SemaphoreHandle_t *semaphore_to_take, uint16_t delay_ms);
bool sph_check    (SemaphoreHandle_t *semaphore_to_check);

#endif /* LOCKS_H */
