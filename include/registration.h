/***
 *     /$$$$$$$                      /$$             /$$                          /$$     /$$
 *    | $$__  $$                    |__/            | $$                         | $$    |__/
 *    | $$  \ $$  /$$$$$$   /$$$$$$  /$$  /$$$$$$$ /$$$$$$    /$$$$$$  /$$$$$$  /$$$$$$   /$$  /$$$$$$  /$$$$$$$
 *    | $$$$$$$/ /$$__  $$ /$$__  $$| $$ /$$_____/|_  $$_/   /$$__  $$|____  $$|_  $$_/  | $$ /$$__  $$| $$__  $$
 *    | $$__  $$| $$$$$$$$| $$  \ $$| $$|  $$$$$$   | $$    | $$  \__/ /$$$$$$$  | $$    | $$| $$  \ $$| $$  \ $$
 *    | $$  \ $$| $$_____/| $$  | $$| $$ \____  $$  | $$ /$$| $$      /$$__  $$  | $$ /$$| $$| $$  | $$| $$  | $$
 *    | $$  | $$|  $$$$$$$|  $$$$$$$| $$ /$$$$$$$/  |  $$$$/| $$     |  $$$$$$$  |  $$$$/| $$|  $$$$$$/| $$  | $$
 *    |__/  |__/ \_______/ \____  $$|__/|_______/    \___/  |__/      \_______/   \___/  |__/ \______/ |__/  |__/
 *                         /$$  \ $$
 *                        |  $$$$$$/
 *                         \______/
 */

#ifndef __REGISTRATION_H__
#define __REGISTRATION_H__

#include "esp_system.h"

/***
 *     _____         _
 *    |_   _|       | |
 *      | | __ _ ___| | __
 *      | |/ _` / __| |/ /
 *      | | (_| \__ \   <
 *      \_/\__,_|___/_|\_\
 *
 *
 */

 /*FUNCTION****************************************/
 /*clk_pit_task compatible freeRTOS executable task, will handle keys, parameters and
 hub information initialization*/
 /*Parameters*************************************
 param: standarf freertos param*/
void  registration_task(void *params);

/*FUNCTION****************************************/
/*registration_isIncomplete This function will check if all keys and necessary configuration parameters
are ready for the next stage*/
typedef void (*msg_validation_callback_t)(void);
uint8_t registration_isIncomplete(void);

/*FUNCTION****************************************/
/*registration_handleNextCommand when operating, this function will search
for registration commands on startup until all keys are logged*/
uint8_t registration_handleNextCommand();

/***
 *                      __ _
 *                     / _(_)
 *      ___ ___  _ __ | |_ _  __ _
 *     / __/ _ \| '_ \|  _| |/ _` |
 *    | (_| (_) | | | | | | | (_| |
 *     \___\___/|_| |_|_| |_|\__, |
 *                            __/ |
 *                           |___/
 */
/*FUNCTION****************************************/
/*registration_logHelp Help function is called on "helpme" command*/
void    registration_logHelp();

/*FUNCTION****************************************/
/*registration_set_layout_version sets the hardware layout version to follow when doing
any machine-wise operation*/
/*Parameters*************************************
version: Version to set.*/
void    registration_set_layout_version(char * version);

/*FUNCTION****************************************/
/*registration_set_validation_callback will set the function to be called upon receiving
the "validate" action from the cloud platform, in confirmation that the device was
successfully connected*/
/*Parameters*************************************
new_validation_callbacl: Callback to call*/
void    registration_set_validation_callback(msg_validation_callback_t new_validation_callbacl);

/***
 *                               _
 *                              | |
 *      ___ ___  _ __  ___  ___ | | ___
 *     / __/ _ \| '_ \/ __|/ _ \| |/ _ \
 *    | (_| (_) | | | \__ \ (_) | |  __/
 *     \___\___/|_| |_|___/\___/|_|\___|
 *
 *
 */
 /*FUNCTION****************************************/
 /*delete_layout will delete the layout key from flash, thus forcing next registration
 to seek for it*/
void    delete_layout(void);

/*FUNCTION****************************************/
/*delete_all_keys will delete some of the keys on flash, thus forcing next registration
to seek for them*/
void    delete_all_keys(void);

/*FUNCTION****************************************/
/*TEST_execute_regisration_callback will delete some of the keys on flash, thus forcing next registration
to seek for them*/
void TEST_execute_regisration_callback(void);

#endif
