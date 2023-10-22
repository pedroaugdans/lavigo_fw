/////////////////////////////////////////////////////////////////////////////////////
//      ____  ____  _____    ____________       _____ ____  _____________________//
//     / __ \/ __ \/  _/ |  / / ____/ __ \     / ___// __ \/  _/ ____/ ____/ ___///
//    / / / / /_/ // / | | / / __/ / /_/ /_____\__ \/ /_/ // // /_  / /_   \__ \ //
//   / /_/ / _, _// /  | |/ / /___/ _, _/_____/__/ / ____// // __/ / __/  ___/ / //
//  /_____/_/ |_/___/  |___/_____/_/ |_|     /____/_/   /___/_/   /_/    /____/  //
////////////////////////////////////////////////////////////////////////////////////  

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "string.h"

#include "base64_coding.h"
#include "drv_spiffs.h"
#include "drv_locks.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "errno.h"
#include "params.h"

#define DOESNOT_EXIST 0
#define EXISTS 1
#define SIZEOF_EL 1

#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00


/*** VARIABLES **************************************************************************/

static const char *TAG = "[DRV-SPIFFS]";
static const char *root = "/spiffs";
static const char *separator = "/";
SemaphoreHandle_t flash_lock = NULL;

/*** DECLARATIONS ***********************************************************************/

static void spiffs_debug_info(void);
static void spiffs_mount(void);
static uint8_t spiffs_file_save(const char * file_name, const char * file_content);

/*** DEFINITIONS ************************************************************************/

static uint8_t spiffs_file_save(const char * file_name, const char * file_content) {
    errno = 0;
    //size_t total = 0, used = 0;
    if (sph_step_retries(&flash_lock) == TRUE) {

        ESP_LOGI(TAG, "Opening file [%s]", file_name);
        FILE* f = NULL;
        do {
            f = fopen(file_name, "wb+");
            vTaskDelay(500 / portTICK_RATE_MS);
            ESP_LOGW(TAG, "Failing to open file [%s]", file_name);
            ESP_LOGE(TAG, "error is %d", errno);
        } while (f == NULL);
        if (f == NULL) {
            ESP_LOGE(TAG, "Failed to open file [%s] for writing", file_name);
            sph_give(&flash_lock);
            return FAILURE;
        }
        fprintf(f, file_content);
        fflush(f);
        fsync(fileno(f));
        fclose(f);
        ESP_LOGI(TAG, "File written [%s] with size [%d]", file_name, strlen(file_content));
        //esp_spiffs_info(NULL, &total, &used);
        sph_give(&flash_lock);
        return SUCCESS;
    } else {
        ESP_LOGE(TAG, "Semaphore untaken");
        return FAILURE;
    }
}

static uint8_t spiffs_file_check(const char * file_name) {
    struct stat st;
    if (sph_step_retries(&flash_lock) == TRUE) {
        if (stat(file_name, &st) == 0) {
            sph_give(&flash_lock);
            return EXISTS;
        }
        sph_give(&flash_lock);
        return DOESNOT_EXIST;
    } else {
        ESP_LOGE(TAG, "Semaphore untaken");
        return DOESNOT_EXIST;
    }
}

static size_t spiffs_file_load(const char * file_name, char * file_content) {
    FILE* f = NULL;
    size_t read_chars = 0, total_chars = 0;
    /*DEBUG*/ESP_LOGI(TAG, "Reading file [%s]", file_name);
    if (sph_step_retries(&flash_lock) == TRUE) {
        f = fopen(file_name, "r");
        if (f == NULL) {
            ESP_LOGE(TAG, "Failed to open [%s] for reading", file_name);
            sph_give(&flash_lock);
            return 0;
        }
        fseek(f, 0, SEEK_END);
        total_chars = ftell(f);
        rewind(f);
        read_chars = fread(file_content, SIZEOF_EL, total_chars, f);
        fflush(f);
        fclose(f);
        sph_give(&flash_lock);
        return read_chars;
    } else {
        ESP_LOGE(TAG, "Semaphore untaken");
        return 0;
    }
}

static uint8_t spiffs_file_erase(const char * file_name) {
    int res;
    if (sph_step_retries(&flash_lock) == TRUE) {
        res = unlink(file_name);
        if (res != 0) {
            ESP_LOGE(TAG, "file [%s] not found, error [%d] : [%s] ", file_name, res, strerror(res));
            sph_give(&flash_lock);
            return FAILURE;
        }
        sph_give(&flash_lock);
        return SUCCESS;
    } else {
        ESP_LOGE(TAG, "Semaphore untaken");
        return FAILURE;
    }
}

static uint8_t spiffs_file_rename(const char * filename_old, const char * filename_new) {
    if (sph_step_retries(&flash_lock) == TRUE) {
        if (rename(filename_old, filename_new) != 0) {
            ESP_LOGE(TAG, "could not rename [%s] to [%s]", filename_old, filename_new);
            sph_give(&flash_lock);
            return FAILURE;
        }
        sph_give(&flash_lock);
        return SUCCESS;
    } else {
        ESP_LOGE(TAG, "Semaphore untaken");
        return FAILURE;
    }
}

uint8_t drv_spiffs_clear(const char * key) {
    char file_destination[40] = {0};
    strcpy(file_destination, root);
    strcat(file_destination, separator);
    strcat(file_destination, key);

    return spiffs_file_erase((const char *) file_destination);
}

uint8_t drv_spiffs_rename(const char * old_key, const char * new_key) {
    char file_destination_old[40] = {0}, file_destination_new[40];
    strcpy(file_destination_old, root);
    strcat(file_destination_old, separator);
    strcat(file_destination_old, old_key);

    strcpy(file_destination_new, root);
    strcat(file_destination_new, separator);
    strcat(file_destination_new, new_key);


    return spiffs_file_rename(old_key, new_key);
}

void drv_spiffs_init(void) {
    sph_create(&flash_lock);
    spiffs_mount();
    /*DEBUG*/spiffs_debug_info();
}

uint8_t drv_spiffs_check(const char *key) {
    char file_destination[40] = {0};

    strcpy(file_destination, root);
    strcat(file_destination, separator);
    strcat(file_destination, key);
    if (spiffs_file_check(file_destination))
        return SUCCESS;
    else
        return FAILURE;
}

uint8_t drv_spiffs_set(const char *key, const char *value) {
    char file_destination[40] = {0};
    strcpy(file_destination, root);
    strcat(file_destination, separator);
    strcat(file_destination, key);
    if (spiffs_file_save(file_destination, value) == FAILURE) {
        return FAILURE;
    } else {
        return SUCCESS;
    }
}

uint8_t drv_spiffs_get(const char *key, char *value) {
    char file_destination[40] = {0};
    strcpy(file_destination, root);
    strcat(file_destination, separator);
    strcat(file_destination, key);
    if (!spiffs_file_load(file_destination, value)) {
        return FAILURE;
    } else {
        return SUCCESS;
    }
}

static void spiffs_debug_info(void) {
    esp_err_t ret;
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information [%s]", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}

static void spiffs_mount(void) {
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = root,
        .partition_label = NULL,
        .max_files = 10,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

}
static void spiffs_unmount(void) __attribute__((unused));


static void spiffs_unmount(void){
    esp_vfs_spiffs_unregister(NULL);
    ESP_LOGI(TAG, "SPIFFS unmounted");
}