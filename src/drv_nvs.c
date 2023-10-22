//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____   ____ _    __ ______ ____              _   __ _    __ _____         //
//    / __ \ / __ \ /  _/| |  / // ____// __ \            / | / /| |  / // ___/         //
//   / / / // /_/ / / /  | | / // __/  / /_/ /  ______   /  |/ / | | / / \__ \          //
//  / /_/ // _, _/_/ /   | |/ // /___ / _, _/  /_____/  / /|  /  | |/ / ___/ /          //
// /_____//_/ |_|/___/   |___//_____//_/ |_|           /_/ |_/   |___/ /____/           //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "drv_nvs.h"

#include "nvs_flash.h"
#include "params.h"
#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define NVS_STORE "lavigo-store"

/*** VARIABLES **************************************************************************/

static const char *TAG = "[DRV-NVS]";

/*** DECLARATIONS ***********************************************************************/


/*** DEFINITIONS ************************************************************************/

static void check_nvs(void){
  nvs_stats_t nvs_stats;
  nvs_get_stats(NULL,&nvs_stats);
  ESP_LOGI(TAG,"Count: UsedEntries = (%d), FreeEntries = (%d), AllEntries = (%d)\n",
       nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);
}

void drv_nvs_init(void) {
    /*DEBUG*/ESP_LOGI(TAG, "init()");
    sph_create(&flash_lock);
    esp_err_t err = nvs_flash_init();
    check_nvs();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG,"ESP ERROR FOUND? [%d] | [0x%X]",err,err);
        //ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_LOGW(TAG,"ESP ERROR FOUND? [%d] | [0x%X]",err,err);
    //ESP_ERROR_CHECK(err);

    ///*DEBUG*/ESP_LOGI(TAG, "~init()");
}

uint8_t drv_nvs_check(const char *key) {
    if (sph_step_retries(&flash_lock) == TRUE) {
        nvs_handle nvs_store = 0;

        uint8_t status = FAILURE;

        size_t length;

        esp_err_t err;

        err = nvs_open(NVS_STORE, NVS_READONLY, &nvs_store);
        err = nvs_get_str(nvs_store, key, NULL, &length);

        switch (err) {
            case ESP_OK:
                //ESP_LOGI(TAG, "(200) >< Success key:[%s]", key);
                status = SUCCESS;
                break;

            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGW(TAG, "(404) >< Not found key:[%s]", key);
                status = FAILURE;
                break;

            default:
                ESP_LOGE(TAG, "(500) >< Internal error:[0x%X] key:[%s]", err, key);
                status = FAILURE;
                break;
        }

        nvs_close(nvs_store);
        sph_give(&flash_lock);
        return status;
    } else {
        ESP_LOGE(TAG, "Semaphore untaken");
        return FAILURE;
    }
}

uint8_t drv_nvs_set(const char *key, char *value) {
    nvs_handle nvs_store = 0;

    uint8_t status = SUCCESS;

    if (sph_step_retries(&flash_lock) == TRUE) {
        esp_err_t err;

        err = nvs_open(NVS_STORE, NVS_READWRITE, &nvs_store);

        nvs_erase_key(nvs_store, key);
        nvs_commit(nvs_store);

        err = nvs_set_str(nvs_store, key, value);
        nvs_commit(nvs_store);

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "(500) >> Internal error:[0x%X] key:[%s]", err, key);
            status = FAILURE;
        }

        nvs_close(nvs_store);
        sph_give(&flash_lock);
        return status;
    } else {
        ESP_LOGE(TAG, "Semaphore untaken");
        return FAILURE;
    }
}

uint8_t drv_nvs_clear(const char *key) {
    nvs_handle nvs_store = 0;
    uint8_t status = SUCCESS;
    esp_err_t err;


    err = nvs_open(NVS_STORE, NVS_READWRITE, &nvs_store);
    if (drv_nvs_check(key) == FAILURE) {

        return FAILURE;
    } else {
        if (sph_step_retries(&flash_lock) == TRUE) {
            err = nvs_erase_key(nvs_store, key);
            nvs_commit(nvs_store);
            switch (err) {
                case ESP_OK:
                    ESP_LOGI(TAG, "(200) >< Success key:[%s]", key);
                    status = SUCCESS;
                    break;

                case ESP_ERR_NVS_NOT_FOUND:
                    ESP_LOGI(TAG, "(404) >< Not found key:[%s]", key);
                    break;

                default:
                    ESP_LOGE(TAG, "(500) >< Internal error:[0x%X] key:[%s]", err, key);
                    break;
            }

            sph_give(&flash_lock);
            return status;
        } else {
            ESP_LOGE(TAG, "Semaphore untaken");
            return FAILURE;
        }
    }

}

static uint8_t drv_nvs_str_length(const char *key, uint16_t * output_length) {
    nvs_handle nvs_store = 0;

    uint8_t status = SUCCESS;

    size_t length;

    esp_err_t err;

    if (drv_nvs_check(key)) {
        return ESP_FAIL;
    }
    if (sph_step_retries(&flash_lock) == TRUE) {
        err = nvs_open(NVS_STORE, NVS_READONLY, &nvs_store);
        err = nvs_get_str(nvs_store, key, NULL, &length);

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "(500) << Internal error:[0x%X] key:[%s]", err, key);
            status = FAILURE;
        }
        (*output_length) = (length);
        nvs_close(nvs_store);
        sph_give(&flash_lock);
        return status;
    } else {
        ESP_LOGE(TAG, "Semaphore untaken");
        return FAILURE;
    }
}

uint8_t drv_nvs_rename(const char * old_key, const char * new_key) {
    uint16_t length = 0;

    if (drv_nvs_str_length(old_key, &length) != SUCCESS) {
        return FAILURE;
    }

    char * temporal_buffer = (char*) calloc(1, length + 1);
    if (temporal_buffer == NULL) {
        return FAILURE;
    }

    if (drv_nvs_get(old_key, temporal_buffer) != SUCCESS) {
        return FAILURE;
    }

    if (drv_nvs_clear(old_key) != SUCCESS) {
        return FAILURE;
    }

    if (drv_nvs_set(new_key,temporal_buffer) != SUCCESS) {
        return FAILURE;
    }
    free(temporal_buffer);

    return SUCCESS;
}

uint8_t drv_nvs_get(const char *key, char *value) {

    nvs_handle nvs_store = 0;

    uint8_t status = SUCCESS;

    size_t length;

    esp_err_t err;

    if (drv_nvs_check(key)) {
        return ESP_FAIL;
    }
    if (sph_step_retries(&flash_lock) == TRUE) {
        err = nvs_open(NVS_STORE, NVS_READONLY, &nvs_store);
        /*! Added extra NVS_GET_STR to get correct LENGTH
         */
        err = nvs_get_str(nvs_store, key, NULL, &length);
        err = nvs_get_str(nvs_store, key, value, &length);

        //value[length + 1] = 0;

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "(500) << Internal error:[0x%X] key:[%s] getting %s", err, key, value);
            status = FAILURE;
        }

        nvs_close(nvs_store);
        sph_give(&flash_lock);
        return status;
    } else {
        ESP_LOGE(TAG, "(drv_nvs_get) Semaphore untaken");
        return FAILURE;
    }
}
