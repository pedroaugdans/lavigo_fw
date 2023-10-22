/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
 /***
  *     /$$$$$$$  /$$$$$$$$ /$$$$$$$  /$$        /$$$$$$  /$$     /$$ /$$      /$$ /$$$$$$$$ /$$   /$$ /$$$$$$$$
  *    | $$__  $$| $$_____/| $$__  $$| $$       /$$__  $$|  $$   /$$/| $$$    /$$$| $$_____/| $$$ | $$|__  $$__/
  *    | $$  \ $$| $$      | $$  \ $$| $$      | $$  \ $$ \  $$ /$$/ | $$$$  /$$$$| $$      | $$$$| $$   | $$
  *    | $$  | $$| $$$$$   | $$$$$$$/| $$      | $$  | $$  \  $$$$/  | $$ $$/$$ $$| $$$$$   | $$ $$ $$   | $$
  *    | $$  | $$| $$__/   | $$____/ | $$      | $$  | $$   \  $$/   | $$  $$$| $$| $$__/   | $$  $$$$   | $$
  *    | $$  | $$| $$      | $$      | $$      | $$  | $$    | $$    | $$\  $ | $$| $$      | $$\  $$$   | $$
  *    | $$$$$$$/| $$$$$$$$| $$      | $$$$$$$$|  $$$$$$/    | $$    | $$ \/  | $$| $$$$$$$$| $$ \  $$   | $$
  *    |_______/ |________/|__/      |________/ \______/     |__/    |__/     |__/|________/|__/  \__/   |__/
  *
  *
  *
  */

 #include "hubware.h"

 #ifdef __TESTING_DEPLOYMENT__

 #define FOREVER while (1)
 #define SUCCESS 0x00
 #define FAILURE 0xFF
 #define TRUE    0x01
 #define FALSE   0x00

 #define ADAPTER 2

 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"

 #include "machines.h"
 #include "deployment.h"
 #include "string.h"
 #include "messages.h"
 #include "msg_fmt.h"
 #include "drv_nvs.h"
 #include "drv_spiffs.h"
 #include "registration.h"

 static const char *TAG = "[TEST-DEPLOY_INTERNAL]";

/*EXAMPLE OF DEPLOYMENT PAYLOAD*/
/*PROTOCOL: D, PULSE LENGTH: 10 SECONDS*/
/*RESOURCE NAME: 56*/
/*PLEASE CHANGE <hub-GIRK3XBJE2YA> FOR THE ACTUAL HUB ID*/
 static char * init_machine_action = "{\"hub\":\"hub-GIRK3XBJE2YA\",\"resource\":56,\"action\":\"init\",\"template\":{\"signals\":[{\"name\":\"S\",\"port\":0,\"direction\":0},{\"name\":\"A\",\"port\":0,\"direction\":1}],\"ports\":[{\"color\":\"vert\",\"retrofit\":1,\"letter\":\"A\"}],\"version\":1,\"actions\":[{\"name\":\"cycle\",\"pattern\":\"A:l|S:L|S:H\",\"idle\":1,\"enable\":1,\"target\":0,\"status\":\"running\"},{\"name\":\"reset\",\"pattern\":\"S:L\",\"target\":0,\"status\":\"success\"},{\"name\":\"deny\",\"pattern\":\"A:H\",\"target\":1,\"status\":\"success\"},{\"name\":\"allow\",\"pattern\":\"A:L\",\"target\":1,\"status\":\"success\"}],\"events\":[{\"name\":\"cycle\",\"pattern\":\"S:l|S:h|#:10|S:L|#:1\",\"idle\":1,\"enable\":1,\"target\":0,\"status\":\"success\"},{\"name\":\"cycle\",\"pattern\":\"S:l|S:h\",\"idle\":1,\"enable\":1,\"target\":1,\"status\":\"trigger\"}]}}";

static char * fix_machine_action = "{\r\n\"hub\":\"hub-GIRK3XBJE2YA\",\r\n\"resource\":56,\r\n\"action\":\"fix\"\r\n}";

static char * clear_machine_action = "{\r\n\"hub\":\"hub-GIRK3XBJE2YA\",\r\n\"resource\":56,\r\n\"action\":\"clear\"\r\n}";


 void hw_init(void);
 void sw_init(void);
 void fw_run (void);

 void fsm_run (void *params);

 void hubware_init(void){
   hw_init();
   sw_init();
 }

 void hubware_run (void) {
   fw_run();
 }

 void hw_init(void) {
   /*DEBUG*/ESP_LOGI(TAG, "hw_init()");

   drv_nvs_init();
   /*DEBUG*/ESP_LOGI(TAG, "~hw_init()");
 }
 void sw_init(void) {
   /*DEBUG*/ESP_LOGI(TAG, "sw_init()");
   params_init();

   /*LAYOUT VERSION is necessary for any machine operation*/
   registration_set_layout_version(VERSION_0_5_0);
   set_layout_version(version_5_0);

   machines_init();
   machines_load();

   /*DEBUG*/ESP_LOGI(TAG, "~sw_init()");
 }

 void fw_run(void) {
   /*DEBUG*/ESP_LOGI(TAG, "fw_run()");

  while(1){
    msg_fmt_test_cb(Deployment,init_machine_action);
    if(!TEST_deployment_init_cb()){
      ESP_LOGI(TAG,"init successful");
    } else {
      ESP_LOGW(TAG,"There was a problem with the deployment init");
    }
    vTaskDelay(1000  / portTICK_RATE_MS);

ESP_LOGE(TAG,"/*********************************Fixing********************************/\n\n");
    msg_fmt_test_cb(Deployment,fix_machine_action);
    if(TEST_deployment_fix_cb() == HUB_OK){
      ESP_LOGI(TAG,"fix successful");
    } else {
      ESP_LOGW(TAG,"There was a problem with the deployment fix");
    }
    vTaskDelay(1000  / portTICK_RATE_MS);

ESP_LOGE(TAG,"/*********************************Clear********************************/\n\n");
    msg_fmt_test_cb(Deployment,clear_machine_action);
    if(TEST_deployment_clear_cb() == HUB_OK){
      ESP_LOGI(TAG,"clear successful");
    } else {
      ESP_LOGW(TAG,"There was a problem with the deployment clear");
    }
    vTaskDelay(10000  / portTICK_RATE_MS);
  }


   /*DEBUG*/ESP_LOGI(TAG, "~fw_run()");
 }

 #endif
