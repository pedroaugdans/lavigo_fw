/////////////////////////////////////////////////////////////////////////////////////
//      ____  ____  _____    ____________       _____ ____  _____________________//
//     / __ \/ __ \/  _/ |  / / ____/ __ \     / ___// __ \/  _/ ____/ ____/ ___///
//    / / / / /_/ // / | | / / __/ / /_/ /_____\__ \/ /_/ // // /_  / /_   \__ \ //
//   / /_/ / _, _// /  | |/ / /___/ _, _/_____/__/ / ____// // __/ / __/  ___/ / //
//  /_____/_/ |_/___/  |___/_____/_/ |_|     /____/_/   /___/_/   /_/    /____/  //
////////////////////////////////////////////////////////////////////////////////////  

#ifndef DRV_SPIFFS_H
#define DRV_SPIFFS_H


#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

void drv_spiffs_init(void);

uint8_t drv_spiffs_check(const char *key);
uint8_t drv_spiffs_set  (const char *key, const char *value);
uint8_t drv_spiffs_get  (const char *key, char *value);
uint8_t drv_spiffs_clear(const char * key);
uint8_t drv_spiffs_rename(const char * old_key,const char * new_key);

#endif /* DRV_SPIFFS_H */

