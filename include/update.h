#ifndef __UPDATE_H__
#define __UPDATE_H__
/***
 *     /$$   /$$ /$$$$$$$  /$$$$$$$   /$$$$$$  /$$$$$$$$ /$$$$$$$$
 *    | $$  | $$| $$__  $$| $$__  $$ /$$__  $$|__  $$__/| $$_____/
 *    | $$  | $$| $$  \ $$| $$  \ $$| $$  \ $$   | $$   | $$
 *    | $$  | $$| $$$$$$$/| $$  | $$| $$$$$$$$   | $$   | $$$$$
 *    | $$  | $$| $$____/ | $$  | $$| $$__  $$   | $$   | $$__/
 *    | $$  | $$| $$      | $$  | $$| $$  | $$   | $$   | $$
 *    |  $$$$$$/| $$      | $$$$$$$/| $$  | $$   | $$   | $$$$$$$$
 *     \______/ |__/      |_______/ |__/  |__/   |__/   |________/
 *
 *
 *
 */

/*FUNCTION****************************************/
/*update_task compatible freeRTOS executable task for update logic application*/
/*Parameters*************************************
param: standarf freertos param*/
void update_task(void *pvParameters);

/*FUNCTION****************************************/
/*TEST_update Trigger an update test. Flags handled by FSM effects should be handled
by test suite*/
/*Parameters*************************************
url: test target URL to trigger an update*/
void TEST_update(char * url);
#endif
