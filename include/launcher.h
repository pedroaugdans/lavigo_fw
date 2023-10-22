/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   launcher.h
 * Author: independent contractor
 *
 * Created on August 7, 2019, 6:59 PM
 */

#ifndef MASTERFSMLAUNCHER_H
#define MASTERFSMLAUNCHER_H


#include "lavigoMasterFSM.h"

typedef enum {
    no_one_running = 0,
    idle_mode = 9,
    running_mode = 15,
    fallback_mode = 75

} desired_running_t;

void setCurrent(desired_running_t newCurrent);
desired_running_t getDesired(void);
void setDesired(desired_running_t newDesired);
desired_running_t getCurrent(void);
void launcher_task(void *pvParameters);
uint8_t launch_update(void);
uint8_t stop_update(void);


#endif /* MASTERFSMLAUNCHER_H */
