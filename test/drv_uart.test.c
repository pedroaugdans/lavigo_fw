//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____   ____ _    __ ______ ____              __  __ ___     ____  ______  //
//    / __ \ / __ \ /  _/| |  / // ____// __ \            / / / //   |   / __ \/_  __/  //
//   / / / // /_/ / / /  | | / // __/  / /_/ /  ______   / / / // /| |  / /_/ / / /     //
//  / /_/ // _, _/_/ /   | |/ // /___ / _, _/  /_____/  / /_/ // ___ | / _, _/ / /      //
// /_____//_/ |_|/___/   |___//_____//_/ |_|            \____//_/  |_|/_/ |_| /_/       //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "hubware.h"

#ifdef __TESTING_DRV_UART__

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "drv_uart.h"
#include "params.h"

static const char *TAG = "[TEST-DRV-UART]";

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

  drv_uart_init();

  ESP_LOGI(TAG, "~hw_init()");
}

void sw_init(void) {
  ESP_LOGI(TAG, "sw_init()");

  ESP_LOGI(TAG, "~sw_init()");
}

void uart_test(void *params) {
  ESP_LOGI(TAG, "uart_test()");

  vTaskDelay( 1000 / portTICK_RATE_MS);

  FOREVER {
    ESP_LOGI(TAG, "spin()");

    ESP_LOGI(TAG, "Waiting for a message terminated with \\n...");
    vTaskDelay( 2000 / portTICK_RATE_MS);

    char *message = NULL;

    uint16_t length = drv_uart_fetch("\n", &message);

    ESP_LOGI(TAG, "length:[%d]", length);

    if (length) {
      ESP_LOGI(TAG, "message:[%s]", message);
    }

    ESP_LOGI(TAG, "Waiting for a command on the list...");
    vTaskDelay( 2000 / portTICK_RATE_MS);

    uint8_t idx = drv_uart_next(registration_commands, 10);

    ESP_LOGI(TAG, "idx:[%d]", idx);

    if (idx != FAILURE) {
      ESP_LOGI(TAG, "command:[%s]", registration_commands[idx]);
    }

    vTaskDelay( 1000 / portTICK_RATE_MS);
  }

  ESP_LOGI(TAG, "~uart_test()");
}

void fw_run(void) {
  ESP_LOGI(TAG, "fw_run()");

  xTaskCreatePinnedToCore(&uart_test ,"uart_test" ,1024*4 ,NULL ,5 ,NULL ,1);

  ESP_LOGI(TAG, "~fw_run()");
}

#endif
