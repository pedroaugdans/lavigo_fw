/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "drv_locks.h"
#include "esp_log.h"
#define TRUE 1
#define FALSE 0

static const char * TAG = "[LOCK]";
bool sph_take_imm(SemaphoreHandle_t *semaphore_to_take) {
    return (xSemaphoreTake(*semaphore_to_take, (TickType_t) LOCK_IMMEDIATE_INTERVAL/portTICK_RATE_MS) == pdTRUE);
}

bool sph_step_retries(SemaphoreHandle_t *semaphore_to_take) {
    if (xSemaphoreTake(*semaphore_to_take, (TickType_t) LOCK_ZERO_INTERVAL/portTICK_RATE_MS) == pdTRUE) {
        //ESP_LOGI(TAG, "Lock taken successfuly");
        return pdTRUE;
    } else if (xSemaphoreTake(*semaphore_to_take, (TickType_t) LOCK_FIRST_INTERVAL/portTICK_RATE_MS) == pdTRUE) {
        ESP_LOGW(TAG, "failed zero order lock");
        return pdTRUE;
    } else if (xSemaphoreTake(*semaphore_to_take, (TickType_t) LOCK_SECOND_INTERVAL/portTICK_RATE_MS) == pdTRUE) {
        ESP_LOGW(TAG, "failed first order lock");
        return pdTRUE;
    } else {
        ESP_LOGW(TAG, "Failed lock timeout");
        return pdFALSE;
    }
}

bool sph_take_delayed(SemaphoreHandle_t *semaphore_to_take, uint16_t delay_ms){
    return xSemaphoreTake(*semaphore_to_take, (TickType_t) delay_ms/portTICK_RATE_MS);
}

bool sph_give(SemaphoreHandle_t *semaphore_to_take) {
    xSemaphoreGive( *semaphore_to_take );
    return TRUE;
}

bool sph_check(SemaphoreHandle_t *semaphore_to_check) {
    return uxSemaphoreGetCount( *semaphore_to_check );
}

bool sph_create(SemaphoreHandle_t * semaphore_to_take) {
    if ((*semaphore_to_take) != NULL) {
        ESP_LOGW(TAG,"Lock already initialized");
        return FALSE;
    }
    vSemaphoreCreateBinary(*semaphore_to_take);
    if((*semaphore_to_take) == NULL){
        ESP_LOGE(TAG,"Unable to create lock");
        return FALSE;
    }
    return TRUE;
}
