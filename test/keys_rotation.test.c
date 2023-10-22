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

#include "hubware.h"

#ifdef __TESTING_ROTATION__

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
#include "registration.h"
#include "drv_uart.h"

/*Rotation config*/
#define ROTATIONS_TO_ATTEMPT 1

static char *rotations_forward = "{\r\n  \"hub\": \"hub-BN4MAPSDH939\",\r\n  \"action\":\"forward\"\r\n}";

static char * rotations_totest[ROTATIONS_TO_ATTEMPT] = {
  "{\r\n  \"hub\": \"hub-BN4MAPSDH939\",\r\n  \"action\":\"rotate\",\r\n  \"info\":{\r\n      \"key\":\"ioturl\",\r\n      \"value\":\"yomama.com\"\r\n   }\r\n}",//ioturl
};
/*TEST DEFINITIONS*/
/*************************************VALIDATION CONFIG****************************************************/
#define TEST_STAGE "develop"
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

#define TEST_AWS_CRT "-----BEGIN CERTIFICATE-----\nMIIE0zCCA7ugAwIBAgIQGNrRniZ96LtKIVjNzGs7SjANBgkqhkiG9w0BAQUFADCB\nyjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\nExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJp\nU2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxW\nZXJpU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0\naG9yaXR5IC0gRzUwHhcNMDYxMTA4MDAwMDAwWhcNMzYwNzE2MjM1OTU5WjCByjEL\nMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZW\nZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJpU2ln\nbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxWZXJp\nU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9y\naXR5IC0gRzUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCvJAgIKXo1\nnmAMqudLO07cfLw8RRy7K+D+KQL5VwijZIUVJ/XxrcgxiV0i6CqqpkKzj/i5Vbex\nt0uz/o9+B1fs70PbZmIVYc9gDaTY3vjgw2IIPVQT60nKWVSFJuUrjxuf6/WhkcIz\nSdhDY2pSS9KP6HBRTdGJaXvHcPaz3BJ023tdS1bTlr8Vd6Gw9KIl8q8ckmcY5fQG\nBO+QueQA5N06tRn/Arr0PO7gi+s3i+z016zy9vA9r911kTMZHRxAy3QkGSGT2RT+\nrCpSx4/VBEnkjWNHiDxpg8v+R70rfk/Fla4OndTRQ8Bnc+MUCH7lP59zuDMKz10/\nNIeWiu5T6CUVAgMBAAGjgbIwga8wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8E\nBAMCAQYwbQYIKwYBBQUHAQwEYTBfoV2gWzBZMFcwVRYJaW1hZ2UvZ2lmMCEwHzAH\nBgUrDgMCGgQUj+XTGoasjY5rw8+AatRIGCx7GS4wJRYjaHR0cDovL2xvZ28udmVy\naXNpZ24uY29tL3ZzbG9nby5naWYwHQYDVR0OBBYEFH/TZafC3ey78DAJ80M5+gKv\nMzEzMA0GCSqGSIb3DQEBBQUAA4IBAQCTJEowX2LP2BqYLz3q3JktvXf2pXkiOOzE\np6B4Eq1iDkVwZMXnl2YtmAl+X6/WzChl8gGqCBpH3vn5fJJaCGkgDdk+bW48DW7Y\n5gaRQBi5+MHt39tBquCWIMnNZBU4gcmU7qKEKQsTb47bDN0lAtukixlE0kF6BWlK\nWE9gyn6CagsCqiUXObXbf+eEZSqVir2G3l6BFoMtEMze/aiCKm0oHw0LxOXnGiYZ\n4fQRbxC1lfznQgUy286dUV4otp6F01vvpX1FQHKOtw5rDgb7MzVIcbidJ4vEZV8N\nhnacRHr2lVz2XTIIM6RUthg/aFzyQkqFOFSDX9HoLPKsEdao7WNq\n-----END CERTIFICATE-----\n"

#define UPDATE_CRT "-----BEGIN CERTIFICATE-----\nMIIEYzCCA0ugAwIBAgIQAYL4CY6i5ia5GjsnhB+5rzANBgkqhkiG9w0BAQsFADBa\nMQswCQYDVQQGEwJJRTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJl\nclRydXN0MSIwIAYDVQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTE1\nMTIwODEyMDUwN1oXDTI1MDUxMDEyMDAwMFowZDELMAkGA1UEBhMCVVMxFTATBgNV\nBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3LmRpZ2ljZXJ0LmNvbTEjMCEG\nA1UEAxMaRGlnaUNlcnQgQmFsdGltb3JlIENBLTIgRzIwggEiMA0GCSqGSIb3DQEB\nAQUAA4IBDwAwggEKAoIBAQC75wD+AAFz75uI8FwIdfBccHMf/7V6H40II/3HwRM/\nsSEGvU3M2y24hxkx3tprDcFd0lHVsF5y1PBm1ITykRhBtQkmsgOWBGmVU/oHTz6+\nhjpDK7JZtavRuvRZQHJaZ7bN5lX8CSukmLK/zKkf1L+Hj4Il/UWAqeydjPl0kM8c\n+GVQr834RavIL42ONh3e6onNslLZ5QnNNnEr2sbQm8b2pFtbObYfAB8ZpPvTvgzm\n+4/dDoDmpOdaxMAvcu6R84Nnyc3KzkqwIIH95HKvCRjnT0LsTSdCTQeg3dUNdfc2\nYMwmVJihiDfwg/etKVkgz7sl4dWe5vOuwQHrtQaJ4gqPAgMBAAGjggEZMIIBFTAd\nBgNVHQ4EFgQUwBKyKHRoRmfpcCV0GgBFWwZ9XEQwHwYDVR0jBBgwFoAU5Z1ZMIJH\nWMys+ghUNoZ7OrUETfAwEgYDVR0TAQH/BAgwBgEB/wIBADAOBgNVHQ8BAf8EBAMC\nAYYwNAYIKwYBBQUHAQEEKDAmMCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdp\nY2VydC5jb20wOgYDVR0fBDMwMTAvoC2gK4YpaHR0cDovL2NybDMuZGlnaWNlcnQu\nY29tL09tbmlyb290MjAyNS5jcmwwPQYDVR0gBDYwNDAyBgRVHSAAMCowKAYIKwYB\nBQUHAgEWHGh0dHBzOi8vd3d3LmRpZ2ljZXJ0LmNvbS9DUFMwDQYJKoZIhvcNAQEL\nBQADggEBAC/iN2bDGs+RVe4pFPpQEL6ZjeIo8XQWB2k7RDA99blJ9Wg2/rcwjang\nB0lCY0ZStWnGm0nyGg9Xxva3vqt1jQ2iqzPkYoVDVKtjlAyjU6DqHeSmpqyVDmV4\n7DOMvpQ+2HCr6sfheM4zlbv7LFjgikCmbUHY2Nmz+S8CxRtwa+I6hXsdGLDRS5rB\nbxcQKegOw+FUllSlkZUIII1pLJ4vP1C0LuVXH6+kc9KhJLsNkP5FEx2noSnYZgvD\n0WyzT7QrhExHkOyL4kGJE7YHRndC/bseF/r/JUuOUFfrjsxOFT+xJd1BDKCcYm1v\nupcHi9nzBhDFKdT3uhaQqNBU4UtJx5g=\n-----END CERTIFICATE-----\n"

#define MAP_IDX "undefined"

#define STAGE_IDX "develop"
/******************************************************************************************************/

static const char *TAG = "[KEY-ROTATION-TEST]";

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
  drv_uart_init();
  drv_nvs_init();
  drv_wifi_init();
  drv_mqtt_init();

  ESP_LOGI(TAG, "~hw_init()");
}

void sw_init(void) {
  ESP_LOGI(TAG, "sw_init()");
  params_init();

  msg_raw_init();
  msg_fmt_init();
  master_fsm_init();
  ESP_LOGI(TAG, "~sw_init()");
}

bool preparations_finished = FALSE;
uint8_t total_cycles = 0;

void rotation_test(void *params) {
  ESP_LOGI(TAG, " rotation_test()");

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

  ESP_LOGI(TAG, ">> UPDATE_IDX");
  drv_nvs_set(UPDKEY_IDX,UPDATE_CRT);

  ESP_LOGI(TAG, ">> MAP_IDX");
  drv_nvs_set(MAPIDX_IDX,MAP_IDX);

  ESP_LOGI(TAG, ">> STAGE_IDX");
  drv_nvs_set(SYSSTG_IDX,STAGE_IDX);

  check_activation_flags[registation_check_flag] = 1;
  vTaskDelay( 10000 / portTICK_RATE_MS);
  check_activation_flags[internet_check_flag] = 1;
  run_activation_flags[mqtt_run_flag] = 1;
  check_activation_flags[cloud_check_flag] = 1;
  check_activation_flags[registation_check_flag] = 0;


  FOREVER {
    ESP_LOGI(TAG,"Starting rotations...");
    for(uint8_t k = 0 ; k < ROTATIONS_TO_ATTEMPT ; k++){
      while(!preparations_finished){
        if(total_cycles>0){
          ESP_LOGI(TAG,"Waiting attempt...");
        }else {
          ESP_LOGI(TAG,"Waiting preparations...");
        }
        vTaskDelay( 1000 / portTICK_RATE_MS);
      }
      vTaskDelay( 5000 / portTICK_RATE_MS);
      ESP_LOGI(TAG,"Preparations are finished");
      vTaskDelay( 5000 / portTICK_RATE_MS);

      msg_fmt_test_cb(Validation,rotations_totest[k]);
      TEST_execute_regisration_callback();
      vTaskDelay( 5000 / portTICK_RATE_MS);
      msg_fmt_test_cb(Validation,rotations_forward);
      TEST_execute_regisration_callback();
      vTaskDelay( 5000 / portTICK_RATE_MS);
      TEST_reset_iassignment();
      preparations_finished = FALSE;
    }
  }
  ESP_LOGI(TAG, "~ rotation_test()");
}

void i_answer_validation_messages(void *params) {
  FOREVER{
    bool responded=FALSE;
    while(!responded){
      if(check_validation_pending()){
        vTaskDelay( 2000 / portTICK_RATE_MS);
        msg_raw_send(msg_raw_topic(Validation, TO_HUB), "{\r\n  \"hub\": \"hub-BN4MAPSDH939\",\r\n  \"action\":\"validate\"\r\n}");
        ESP_LOGI(TAG,"I answered a validation message");
        responded = TRUE;
        preparations_finished=TRUE;
        vTaskDelay( 5000 / portTICK_RATE_MS);
        total_cycles++;
      }
      vTaskDelay( 100 / portTICK_RATE_MS);
    }
  }
}

void fw_run(void) {
  ESP_LOGI(TAG, "fw_run()");

  xTaskCreatePinnedToCore(&drv_mqtt_task ,"drv_mqtt_task",1024*8 ,NULL ,5 ,NULL ,1);
  xTaskCreatePinnedToCore(&msg_fmt_o_task,"msg_fmt_o_task", 1024 * 3, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(&msg_fmt_i_task,"msg_fmt_i_task", 1024 * 3, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(&rotation_test,"rotation_test"    ,1024*8 ,NULL ,5 ,NULL ,1);
  xTaskCreatePinnedToCore(&registration_task, "registration_task", 512 * 7, NULL, 5, NULL, 1); // Requires >= 1024*3
  xTaskCreatePinnedToCore(&connection_task, "connection_task", 1024 * 5, NULL, 5, NULL, 1); // Requires >= 1024*3
  xTaskCreatePinnedToCore(&validation_task, "validation_task", 1024 * 3, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(&i_answer_validation_messages, "validation_task", 1024 * 3, NULL, 5, NULL, 1);

  ESP_LOGI(TAG, "~fw_run()");
}

#endif
