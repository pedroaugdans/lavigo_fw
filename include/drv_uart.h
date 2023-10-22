//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____   ____ _    __ ______ ____              __  __ ___     ____  ______  //
//    / __ \ / __ \ /  _/| |  / // ____// __ \            / / / //   |   / __ \/_  __/  //
//   / / / // /_/ / / /  | | / // __/  / /_/ /  ______   / / / // /| |  / /_/ / / /     //
//  / /_/ // _, _/_/ /   | |/ // /___ / _, _/  /_____/  / /_/ // ___ | / _, _/ / /      //
// /_____//_/ |_|/___/   |___//_____//_/ |_|            \____//_/  |_|/_/ |_| /_/       //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __DRV_UART_H__
#define __DRV_UART_H__

#include "esp_system.h"
#include "esp_log.h"

#define UART_PORT          UART_NUM_0

#define UART_STOP_COMMAND   'q'
#define UART_COMMAND_LENGTH  6
#define UART_IDLE_TIMEOUT   50  // in hundreds of milliseconds

void drv_uart_init(void);

/*FUNCTION****************************************/
/*drv_uart_next Will look for a input sequence to match in the provided commands_p
list*/
/*Parameters**************************************/
/*commands_p: Array of admitable commands to be detected on the uart service*/
/*nof_commands; total amount of admitable commands in the commands-p array*/
/*Returns**************************************/
/*SUCCESS if a command was detected*/
uint8_t  drv_uart_next (const char **commands_p, uint8_t nof_commands);

/*FUNCTION****************************************/
/*drv_uart_fetch Will save every received character until the terminator is received*/
/*Parameters**************************************/
/*terminator: Character upon which the character sequence harvesting will finish*/
/*message_p; Memory holder in which the character will be saved*/
/*Returns**************************************/
/*SUCCESS if a sequence was saved*/
uint16_t drv_uart_fetch(const char  *terminator, char  **message_p);

#endif
