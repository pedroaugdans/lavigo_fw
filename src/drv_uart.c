//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____   ____ _    __ ______ ____              __  __ ___     ____  ______  //
//    / __ \ / __ \ /  _/| |  / // ____// __ \            / / / //   |   / __ \/_  __/  //
//   / / / // /_/ / / /  | | / // __/  / /_/ /  ______   / / / // /| |  / /_/ / / /     //
//  / /_/ // _, _/_/ /   | |/ // /___ / _, _/  /_____/  / /_/ // ___ | / _, _/ / /      //
// /_____//_/ |_|/___/   |___//_____//_/ |_|            \____//_/  |_|/_/ |_| /_/       //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "drv_uart.h"

#include "driver/uart.h"

#include "esp_vfs_dev.h"

#include "string.h"

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define UART_BUFFER_SIZE  2048
#define UART_COMMAND_SIZE  128
#define UART_MAX_WAIT      100

/*** VARIABLES **************************************************************************/

static const char *TAG = "[DRV-UART]";

char drv_uart_buffer[UART_BUFFER_SIZE] = { 0 };

/*** DECLARATIONS ***********************************************************************/

/*** DEFINITIONS ************************************************************************/

void drv_uart_init(void) {
  /*DEBUG*/ESP_LOGI(TAG, "init()");
  vTaskDelay(500/portTICK_RATE_MS); //Give some time to flush UART before configuring! should be changed by UART FLUSH function
  setvbuf(stdin, NULL, _IONBF, 0);

  uart_config_t uart_config = {
      .baud_rate = 115200,
      .data_bits = UART_DATA_8_BITS,
      .parity    = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .use_ref_tick = true
  };

  uart_param_config(UART_PORT, &uart_config);

  uart_driver_install(UART_PORT, UART_BUFFER_SIZE, UART_BUFFER_SIZE, 0, NULL, 0);

  esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
  esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
  esp_vfs_dev_uart_use_driver(UART_PORT);
  vTaskDelay(500/portTICK_RATE_MS); //Give some time for the UART to reconfigure before continue logging!
  ///*DEBUG*/ESP_LOGI(TAG, "~init()");
}

uint8_t drv_uart_next(const char **commands_p, uint8_t nof_commands) {
  char command[UART_BUFFER_SIZE] = { 0 };

  for (uint8_t j = 0, ticks = 0; command[0] != UART_STOP_COMMAND && ticks < UART_IDLE_TIMEOUT;) {
    uint8_t k = uart_read_bytes(UART_PORT, (uint8_t *)(command + j), 1, UART_MAX_WAIT / portTICK_RATE_MS);

    j += k;

    if (k == 0) {
      ticks++;
    }

    for (uint8_t i = 0; i < nof_commands; i++) {
      if (!strcmp(commands_p[i], command)) {
        return i;
      }
    }
  }

  return FAILURE;
}

uint16_t drv_uart_fetch(const char *terminator, char **message_p) {
  for(uint16_t i = 0; i < UART_BUFFER_SIZE ; i++) {
    drv_uart_buffer[i] = 0;
  }

  if (terminator != NULL) {
    uint8_t length = strlen(terminator);

    /*ESP_LOGE(TAG,"Now waiting until %s",terminator);*/

    for (uint16_t j = 0, ticks = 0; ticks < UART_IDLE_TIMEOUT;) {
      uint8_t k = uart_read_bytes(UART_PORT, (uint8_t *)(drv_uart_buffer + j), 1, UART_MAX_WAIT / portTICK_RATE_MS);

      j += k;
      if (k == 0) {
        ticks++;
      }

      if ((j > length) && (drv_uart_buffer[j-length] == '\r')) {
        drv_uart_buffer[j-length] = '\n';
      }

      if (j > length) {
        if (!strcmp((drv_uart_buffer+j-length), terminator)) {

          drv_uart_buffer[j-length] = 0;
          *message_p = drv_uart_buffer;

          return j - length;
        }
      }
    }
  }

  return 0;
}
