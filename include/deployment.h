//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ______ ____   __    ____ __  __ __  ___ ______ _   __ ______              //
//    / __ \ / ____// __ \ / /   / __ \\ \/ //  |/  // ____// | / //_  __/              //
//   / / / // __/  / /_/ // /   / / / / \  // /|_/ // __/  /  |/ /  / /                 //
//  / /_/ // /___ / ____// /___/ /_/ /  / // /  / // /___ / /|  /  / /                  //
// /_____//_____//_/    /_____/\____/  /_//_/  /_//_____//_/ |_/  /_/                   //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __DEPLOYMENT_H__
#define __DEPLOYMENT_H__

#include "esp_system.h"
#include "esp_log.h"

/*FUNCTION****************************************/
/*TEST_deployment_init_cb forces initiailization of
 the injected msg on the msg format queue*/
 /*Returns**************************************/
 /*SUCCESS If deployment was successful*/
uint8_t TEST_deployment_init_cb(void);

/*FUNCTION****************************************/
/*TEST_deployment_fix_cb forces fixing of
 the injected msg on the msg format queue*/
 /*Returns**************************************/
 /*SUCCESS If deployment was successful*/
uint8_t TEST_deployment_fix_cb(void);

/*FUNCTION****************************************/
/*TEST_deployment_clear_cb forces clearing of
 the injected msg on the msg format queue*/
 /*Returns**************************************/
 /*SUCCESS If deployment was successful*/
uint8_t TEST_deployment_clear_cb(void);

/*FUNCTION****************************************/
/*deployment_task compatible freeRTOS executable task*/
/*Parameters**************************************/
/*params: standarf freertos param*/
void deployment_task(void *params);

/*FUNCTION****************************************/
/*deployment_machine_4_test Test to be ran from the console*/
/*CURRENTLY DEPRECATED*/
/*Parameters**************************************/
/*params: standarf freertos param*/
void deployment_machine_4_test(void);

/*FUNCTION****************************************/
/*deployment_erase_last eRASES LAST INSTALLED Machine*/
void deployment_erase_last(void);

#endif
