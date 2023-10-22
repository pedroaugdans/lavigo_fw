//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____   ____ _    __ ______ ____              _   __ _    __ _____         //
//    / __ \ / __ \ /  _/| |  / // ____// __ \            / | / /| |  / // ___/         //
//   / / / // /_/ / / /  | | / // __/  / /_/ /  ______   /  |/ / | | / / \__ \          //
//  / /_/ // _, _/_/ /   | |/ // /___ / _, _/  /_____/  / /|  /  | |/ / ___/ /          //
// /_____//_/ |_|/___/   |___//_____//_/ |_|           /_/ |_/   |___/ /____/           //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __DRV_NVS_H__
#define __DRV_NVS_H__

#include "esp_system.h"
#include "esp_log.h"

void drv_nvs_init(void);

/*FUNCTION****************************************/
/*drv_nvs_clear Attempts a subscription a the specified topic. Upon message
reception, the registered single message callback will be called.*/
/*Parameters**************************************/
/*topic: topic in which the messages will be fetched*/
/*Returns**************************************/
/*SUCCESS if subscription request was successful. The subscription fulfillment
will be done in the next YIELD cycle*/
uint8_t drv_nvs_clear(const char *key);

/*FUNCTION****************************************/
/*drv_nvs_check check if the provided entry exists*/
/*Parameters**************************************/
/*key: Key of entry*/
/*Returns**************************************/
/*SUCCESS if operation succeeeded*/
uint8_t drv_nvs_check(const char *key);

/*FUNCTION****************************************/
/*drv_nvs_set Creates an entry to store the provided value content*/
/*Parameters**************************************/
/*key: Name of the memory entry*/
/*value: string to save*/
/*Returns**************************************/
/*SUCCESS if operation succeeeded*/
uint8_t drv_nvs_set  (const char *key, char *value);

/*FUNCTION****************************************/
/*drv_nvs_get Loads the content of the provided key and stores it into the
provided space in value. The caller of this function must be sure to provide
enough space*/
/*Parameters**************************************/
/*key: Name of the memory entry*/
/*value: provided space to store the content*/
/*Returns**************************************/
/*SUCCESS if operation succeeeded*/
uint8_t drv_nvs_get  (const char *key, char *value);

/*FUNCTION****************************************/
/*drv_nvs_rename re-creates an entry with a different name, erasing
the previous one*/
/*Parameters**************************************/
/*old_key: Name of the existing entry*/
/*new_key: Name of the next entry to point to the same segment*/
/*Returns**************************************/
/*SUCCESS if operation succeeeded*/
uint8_t drv_nvs_rename(const char * old_key, const char * new_key);

#endif
