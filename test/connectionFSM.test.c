/***
 *      /$$$$$$   /$$$$$$  /$$   /$$ /$$   /$$ /$$$$$$$$  /$$$$$$  /$$$$$$$$ /$$$$$$  /$$$$$$  /$$   /$$
 *     /$$__  $$ /$$__  $$| $$$ | $$| $$$ | $$| $$_____/ /$$__  $$|__  $$__/|_  $$_/ /$$__  $$| $$$ | $$
 *    | $$  \__/| $$  \ $$| $$$$| $$| $$$$| $$| $$      | $$  \__/   | $$     | $$  | $$  \ $$| $$$$| $$
 *    | $$      | $$  | $$| $$ $$ $$| $$ $$ $$| $$$$$   | $$         | $$     | $$  | $$  | $$| $$ $$ $$
 *    | $$      | $$  | $$| $$  $$$$| $$  $$$$| $$__/   | $$         | $$     | $$  | $$  | $$| $$  $$$$
 *    | $$    $$| $$  | $$| $$\  $$$| $$\  $$$| $$      | $$    $$   | $$     | $$  | $$  | $$| $$\  $$$
 *    |  $$$$$$/|  $$$$$$/| $$ \  $$| $$ \  $$| $$$$$$$$|  $$$$$$/   | $$    /$$$$$$|  $$$$$$/| $$ \  $$
 *     \______/  \______/ |__/  \__/|__/  \__/|________/ \______/    |__/   |______/ \______/ |__/  \__/
 *
 *
 *
 */

 /*NOTE: For this test to work, the hub-id must either be registered and available for automatic platform validation, or else
this message:
 {
  "hub": "hub-BN4MAPSDH939",
  "action":"validate"
}

must be manually pushed to hubs/{STAGE}/{HUB-ID}/registration
after validation request message was published

*/

#include "hubware.h"

#ifdef __TESTING_CONNECION_FSM__

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lavigoMasterFSM.h"
#include "drv_nvs.h"
#include "drv_wifi.h"
#include "drv_mqtt.h"
#include "msg_raw.h"
#include "msg_fmt.h"
#include "params.h"
#include "connection.h"
#include "validation.h"

/*TEST DEFINITIONS*/
/*************************************VALIDATION CONFIG****************************************************/
#define TEST_STAGE "develop"

#define VALIDATION_MESSAGE "{\r\n  \"hub\": \"hub-BN4MAPSDH939\",\r\n  \"action\":\"validate\"\r\n}"
/******************************************************************************************************/
/*************************************MQTT CONFIG****************************************************/
#define TEST_SSID "Aloha"
#define TEST_PSWD "chaussettes"
/******************************************************************************************************/
/*************************************MQTT CONFIG****************************************************/
/***************REPLACE THIS KEYS WITH EACH PERTINENT INDIVIDUAL EXAMPLE******************************/
/*HUB ID: This is a unique ID to identify the HUB as a HUB entity*/
#define TEST_HUBID "hub-BN4MAPSDH939"

#define TEST_IOTURL "ak8du324yg54e.iot.us-east-1.amazonaws.com"

#define TEST_ECC_CRT "-----BEGIN CERTIFICATE-----\nMIIC3TCCAcWgAwIBAgIUOQUUkY+XWPyDeHCygdgHYPowuhwwDQYJKoZIhvcNAQEL\nBQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\nSW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTE5MTAyMTEwNDY1\nMFoXDTQ5MTIzMTIzNTk1OVowbTELMAkGA1UEBhMCRlIxDjAMBgNVBAcMBVBhcmlz\nMQ8wDQYDVQQKDAZMYXZpZ28xDDAKBgNVBAsMA0lvVDESMBAGA1UEAwwJbGF2aWdv\nLmlvMRswGQYJKoZIhvcNAQkBFgxqcEBsYXZpZ28uaW8wWTATBgcqhkjOPQIBBggq\nhkjOPQMBBwNCAATuBBP/gUdjJo2LGexKqm2/EOzUeY4R3OVbWhIvLnEbGoPEO8T/\nZGtV6JdCdSTQUBgesbIBcv7D6NmHkzCeHUKto2AwXjAfBgNVHSMEGDAWgBQ7FpgB\nsEXALhd2wpbjHUDYiZGfojAdBgNVHQ4EFgQUM/pxq8pS7wLLfKg8kksoCO+tFS0w\nDAYDVR0TAQH/BAIwADAOBgNVHQ8BAf8EBAMCB4AwDQYJKoZIhvcNAQELBQADggEB\nAJ52D0ON92ZZ4V2VsIxTe0HqoS12gTG+T3v/akEP6cbf1T1uBdKpEZxk48dEzZ65\nM9sRgWEOAb7hh9QrDSTc9c0sHCrEvMr6xseJdgzTSI6+u0v5i7/7tpNHfzWpRny0\nu31Hwh1/v3BIpk9Ea2VN2sypQcHvwGp7jN+S03tSwG0LFrui2rjigg0TS0whDtF7\nmuCpa5sDxPtHKlZpsNOLZN1r5CVjoQrZzMktXmu+Q7CH2/uXpv8FbZLLeN9PEId5\nRTqF1KgWzIysYH6WjHTrOntxp0LBjfszEQscnPM0aUrjSj82WEGlsuK4ZnDKzUjm\nSPrShFcyzLwIYPKqezLJ9Ow=\n-----END CERTIFICATE-----\n"

#define TEST_ECC_KEY "-----BEGIN EC PARAMETERS-----\nBggqhkjOPQMBBw==\n-----END EC PARAMETERS-----\n-----BEGIN EC PRIVATE KEY-----\nMHcCAQEEIP4igguSSz+7NRlPTqLTzaDwm2Y7NM19snfHFBj/luFYoAoGCCqGSM49\nAwEHoUQDQgAE7gQT/4FHYyaNixnsSqptvxDs1HmOEdzlW1oSLy5xGxqDxDvE/2Rr\nVeiXQnUk0FAYHrGyAXL+w+jZh5Mwnh1CrQ==\n-----END EC PRIVATE KEY-----\n"

#define TEST_AWS_CRT     "-----BEGIN CERTIFICATE-----\nMIIE0zCCA7ugAwIBAgIQGNrRniZ96LtKIVjNzGs7SjANBgkqhkiG9w0BAQUFADCB\nyjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\nExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJp\nU2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxW\nZXJpU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0\naG9yaXR5IC0gRzUwHhcNMDYxMTA4MDAwMDAwWhcNMzYwNzE2MjM1OTU5WjCByjEL\nMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZW\nZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJpU2ln\nbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxWZXJp\nU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9y\naXR5IC0gRzUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCvJAgIKXo1\nnmAMqudLO07cfLw8RRy7K+D+KQL5VwijZIUVJ/XxrcgxiV0i6CqqpkKzj/i5Vbex\nt0uz/o9+B1fs70PbZmIVYc9gDaTY3vjgw2IIPVQT60nKWVSFJuUrjxuf6/WhkcIz\nSdhDY2pSS9KP6HBRTdGJaXvHcPaz3BJ023tdS1bTlr8Vd6Gw9KIl8q8ckmcY5fQG\nBO+QueQA5N06tRn/Arr0PO7gi+s3i+z016zy9vA9r911kTMZHRxAy3QkGSGT2RT+\nrCpSx4/VBEnkjWNHiDxpg8v+R70rfk/Fla4OndTRQ8Bnc+MUCH7lP59zuDMKz10/\nNIeWiu5T6CUVAgMBAAGjgbIwga8wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8E\nBAMCAQYwbQYIKwYBBQUHAQwEYTBfoV2gWzBZMFcwVRYJaW1hZ2UvZ2lmMCEwHzAH\nBgUrDgMCGgQUj+XTGoasjY5rw8+AatRIGCx7GS4wJRYjaHR0cDovL2xvZ28udmVy\naXNpZ24uY29tL3ZzbG9nby5naWYwHQYDVR0OBBYEFH/TZafC3ey78DAJ80M5+gKv\nMzEzMA0GCSqGSIb3DQEBBQUAA4IBAQCTJEowX2LP2BqYLz3q3JktvXf2pXkiOOzE\np6B4Eq1iDkVwZMXnl2YtmAl+X6/WzChl8gGqCBpH3vn5fJJaCGkgDdk+bW48DW7Y\n5gaRQBi5+MHt39tBquCWIMnNZBU4gcmU7qKEKQsTb47bDN0lAtukixlE0kF6BWlK\nWE9gyn6CagsCqiUXObXbf+eEZSqVir2G3l6BFoMtEMze/aiCKm0oHw0LxOXnGiYZ\n4fQRbxC1lfznQgUy286dUV4otp6F01vvpX1FQHKOtw5rDgb7MzVIcbidJ4vEZV8N\nhnacRHr2lVz2XTIIM6RUthg/aFzyQkqFOFSDX9HoLPKsEdao7WNq\n-----END CERTIFICATE-----\n"
/******************************************************************************************************/

static const char *TAG = "[TEST-MSG-FMT]";

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
  ESP_LOGI(TAG, "hw_init()");

  drv_nvs_init();
  drv_wifi_init();
  drv_mqtt_init();

  msg_raw_init();
  msg_fmt_init();
  master_fsm_init();

  msg_fmt_install(Validation, TEST_validation_success_cb);

  ESP_LOGI(TAG, "~hw_init()");
}

void sw_init(void) {
  ESP_LOGI(TAG, "sw_init()");
  params_init();
  ESP_LOGI(TAG, "~sw_init()");
}

void drv_wifi_online_cb(void) {
  ESP_LOGI(TAG, "[online!]");
}

void drv_wifi_offline_cb(void) {
  ESP_LOGI(TAG, "[offline!]");
}

void drv_mqtt_disconnected_cb(drv_mqtt_message_t *message) {
  hub_isOnline = FALSE;
  ESP_LOGI(TAG, "[disconnected:%p]", message);
}
void execution_perform_cb(void){
  ESP_LOGI(TAG,"[Execution]");
}
void validation_perform_cb(void){
  ESP_LOGI(TAG,"[Validation]");
}
void monitoring_perform_cb(void){
  ESP_LOGI(TAG,"[monitoring]");
}
void update_perform_cb(void){
  ESP_LOGI(TAG,"[update]");
}
void deployment_perform_cb(void){
  ESP_LOGI(TAG,"[deployment]");
}
void mqtt_test(void *params) {
  ESP_LOGI(TAG, " mqtt_test()");

  ESP_LOGI(TAG, ">> IOTURL_IDX");
  drv_nvs_set(IOTURL_IDX, TEST_IOTURL);

  ESP_LOGI(TAG, ">> HUBZID_IDX");
  drv_nvs_set(HUBZID_IDX, TEST_HUBID);

  ESP_LOGI(TAG, ">> AWSCRT_IDX");
  drv_nvs_set(AWSCRT_IDX,TEST_AWS_CRT);

  ESP_LOGI(TAG, ">> ECCCRT_IDX");
  drv_nvs_set(ECCCRT_IDX,TEST_ECC_CRT);

  ESP_LOGI(TAG, ">> ECCKEY_IDX");
  drv_nvs_set(ECCKEY_IDX,TEST_ECC_KEY);

  ESP_LOGI(TAG, ">> APPSWD_IDX");
  drv_nvs_set(APPSWD_IDX,TEST_PSWD);

  ESP_LOGI(TAG, ">> APSSID_IDX");
  drv_nvs_set(APSSID_IDX,TEST_SSID);

  params_set(STAGE_PARAM, TEST_STAGE);
  params_set(HUBID_PARAM, TEST_HUBID);

  vTaskDelay(5000 / portTICK_RATE_MS);

  check_activation_flags[internet_check_flag] = 1;
  run_activation_flags[mqtt_run_flag] = 1;
  check_activation_flags[cloud_check_flag] = 1;

  bool responded=FALSE;
  while(!responded){
    if(check_validation_pending()){
      vTaskDelay( 2000 / portTICK_RATE_MS);
      msg_raw_send(msg_raw_topic(Validation, TO_HUB), "{\r\n  \"hub\": \"hub-BN4MAPSDH939\",\r\n  \"action\":\"validate\"\r\n}");
      responded = TRUE;
    }
    vTaskDelay( 100 / portTICK_RATE_MS);
  }
  FOREVER {
    vTaskDelay(5000 / portTICK_RATE_MS);
  }

  ESP_LOGI(TAG, "~ mqtt_test()");
}

void fw_run(void) {
  ESP_LOGI(TAG, "fw_run()");

  xTaskCreatePinnedToCore(&drv_mqtt_task ,"drv_mqtt_task",1024*8 ,NULL ,5 ,NULL ,1);
  xTaskCreatePinnedToCore(&msg_fmt_o_task,"msg_fmt_o_task", 1024 * 3, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(&msg_fmt_i_task,"msg_fmt_i_task", 1024 * 3, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(&mqtt_test     ,"mqtt_test"    ,1024*8 ,NULL ,5 ,NULL ,1);
  xTaskCreatePinnedToCore(&connection_task, "connection_task", 1024 * 5, NULL, 5, NULL, 1); // Requires >= 1024*3
  xTaskCreatePinnedToCore(&validation_task, "validation_task", 1024 * 3, NULL, 5, NULL, 1);

  ESP_LOGI(TAG, "~fw_run()");
}

#endif
