//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____   ____ _    __ ______ ____              ____ ___   ______            //
//    / __ \ / __ \ /  _/| |  / // ____// __ \            /  _/|__ \ / ____/            //
//   / / / // /_/ / / /  | | / // __/  / /_/ /  ______    / /  __/ // /                 //
//  / /_/ // _, _/_/ /   | |/ // /___ / _, _/  /_____/  _/ /  / __// /___               //
// /_____//_/ |_|/___/   |___//_____//_/ |_|           /___/ /____/\____/               //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __DRV_I2C_H__
#define __DRV_I2C_H__

#include "esp_system.h"
#include "esp_log.h"

#define I2C_FREQ_HZ  100000
#define I2C_SCL_IO       15
#define I2C_SDA_IO       33

/*FUNCTION****************************************/
/*drv_i2c_init Initializces I2C perispheric, including PIn initialization*/
/*Parameters**************************************/
void drv_i2c_init(void);

/*FUNCTION****************************************/
/*drv_i2c_write Writes REGISTR + DATA to a certain ADRESS
/ADRESS/1[readbyte] /REGISTER//DATA/ and waits for ACK*/
/*Parameters**************************************/
/*address: First seven bits of the message*/
/*registr: bits 8 to 16*/
/*data: bits 16 to 24*/
/*Returns**************************************/
/*HUB_OK a hubACK was received*/
esp_err_t drv_i2c_write(uint8_t address, uint8_t registr, uint8_t  data);

/*FUNCTION****************************************/
/*drv_i2c_read Sends 2 messages to the device: A first one
/ADRESS/1[readbyte] /REGISTER/ and waits for ACK
then a second one
/ADRESS/1[readbyte] and waits DATA*/
/*Parameters**************************************/
/*address: First seven bits of the message*/
/*registr: bits 8 to 16*/
/*data: Received data on second message*/
/*Returns**************************************/
/*HUB_OK a hubACK was received after reading*/
esp_err_t drv_i2c_read (uint8_t address, uint8_t registr, uint8_t *data);

#endif
