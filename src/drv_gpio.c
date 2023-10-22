/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "params.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"


#include "drv_gpio.h"
#define PULL_DOWN_DIS   0
#define PULL_UP_DIS   0
#define FOREVER while(1)
#define INIT_CALLBACKS_ASDEFAULT    {default_gpio_pisr,default_gpio_pisr,default_gpio_pisr}


static const char * TAG = "[GPIO-DRV]";

/*<!Variables*/

void default_gpio_pisr(void) {
    ESP_LOGI(TAG, "Calling default pisr");
}


static hub_gpio_config_t hub_gpio_config[] = {
    /*   low_level_config,                                                                          io_pin_num, initial_state,  gpio_pisr,  current_state;*/
    /*<!GPIO_GPIO_0, */
    {
        {1ULL << (GPIO_GPIO_0), GPIO_MODE_INPUT, PULL_UP_DIS, PULL_DOWN_DIS, GPIO_PIN_INTR_DISABLE}, GPIO_GPIO_0, 0,INIT_CALLBACKS_ASDEFAULT, 0
    },
    /*<!GPIO_AP_MOD, */
    {
        {1ULL << (GPIO_GPIO_AP), GPIO_MODE_INPUT, PULL_UP_DIS, PULL_DOWN_DIS, GPIO_PIN_INTR_DISABLE}, GPIO_GPIO_AP, 0,INIT_CALLBACKS_ASDEFAULT, 0
    },
    /*<!GPIO_ACCEL,  */
    {
        {1ULL << (GPIO_GPIO_ACC), GPIO_MODE_INPUT, PULL_UP_DIS, PULL_DOWN_DIS, GPIO_PIN_INTR_DISABLE}, GPIO_GPIO_ACC, 0,INIT_CALLBACKS_ASDEFAULT, 0
    },
    /*<!GPIO_LED_0,  */
    {
        {1ULL << (GPIO_GPIO_LED_0), GPIO_MODE_OUTPUT, PULL_UP_DIS, PULL_DOWN_DIS, GPIO_PIN_INTR_DISABLE}, GPIO_GPIO_LED_0, 0,INIT_CALLBACKS_ASDEFAULT, 0
    },
    /*<!GPIO_LED_1   */
    {
        {1ULL << (GPIO_GPIO_LED_1), GPIO_MODE_OUTPUT, PULL_UP_DIS, PULL_DOWN_DIS, GPIO_PIN_INTR_DISABLE}, GPIO_GPIO_LED_1, 0,INIT_CALLBACKS_ASDEFAULT, 0
    }
};

/*<!Definitions*/

void drv_gpio_install_cb(hub_gpio_t hub_gpio, gpio_pisr_cb gpio_pisr, gpio_pattern_t pattern_topisr) {
    hub_gpio_config[hub_gpio].gpio_pisr[pattern_topisr] = gpio_pisr;
}

static void drv_gpio_configure_pins(void) {
    for (hub_gpio_t k = 0; k < TOTAL_HUB_GPIOS; k++) {
        gpio_config(&(hub_gpio_config[k].low_level_config));
    }

}

void drv_gpio_high(hub_gpio_t gpio_tohigh) {
    if (gpio_tohigh > TOTAL_HUB_GPIOS) {
        ESP_LOGE(TAG, "No such GPIO !");
        return;
    }
    if (hub_gpio_config[gpio_tohigh].low_level_config.mode != GPIO_MODE_OUTPUT) {
        ESP_LOGW(TAG, "putting INPUT on HIGH? weird");
        return;
    }
    gpio_set_level(hub_gpio_config[gpio_tohigh].io_pin_num, 1);

}

void drv_gpio_low(hub_gpio_t gpio_tolow) {
    if (gpio_tolow > TOTAL_HUB_GPIOS) {
        ESP_LOGE(TAG, "No such GPIO !");
        return;
    }
    if (hub_gpio_config[gpio_tolow].low_level_config.mode != GPIO_MODE_OUTPUT) {
        ESP_LOGW(TAG, "putting INPUT on LOW? weird");
        return;
    }
    gpio_set_level(hub_gpio_config[gpio_tolow].io_pin_num, 0);

}

void drv_gpio_set(hub_gpio_t gpio_toset, bool value) {
    if (gpio_toset > TOTAL_HUB_GPIOS) {
        ESP_LOGE(TAG, "No such GPIO !");
        return;
    }
    if (hub_gpio_config[gpio_toset].low_level_config.mode != GPIO_MODE_OUTPUT) {
        ESP_LOGW(TAG, "putting INPUT on LOW? weird");
        return;
    }
    hub_gpio_config[gpio_toset].current_state = value;
}

bool drv_gpio_read(hub_gpio_t gpio_toread) {
    if (gpio_toread > TOTAL_HUB_GPIOS) {
        ESP_LOGE(TAG, "No such GPIO !");
        return 0;
    }
    return gpio_get_level(hub_gpio_config[gpio_toread].io_pin_num);
}

static void task_sleep(uint8_t ticks) {
    set_ram_usage(uxTaskGetStackHighWaterMark(NULL), pin_gpio_task_idx);
    vTaskDelay(100 * ticks / portTICK_RATE_MS);
}


#define PATTERN_MOVEMENTS 30
static uint8_t pattern_generator[TOTAL_HUB_GPIOS] = {0};
void drv_gpio_dispatcher(hub_gpio_t pushed_gpio, bool gpio_state);

static void drv_gpio_reject(void) __attribute__((unused));

static void drv_gpio_reject(void){

}

void gpio_task(void *params) {
    FOREVER{
        for (hub_gpio_t k = 0; k < TOTAL_HUB_GPIOS; k++) {
            if (hub_gpio_config[k].low_level_config.mode != GPIO_MODE_INPUT) {
                gpio_set_level(hub_gpio_config[k].io_pin_num, hub_gpio_config[k].current_state);
                continue;
            }
            bool read_gpio = drv_gpio_read(k);
            if ((read_gpio != hub_gpio_config[k].current_state) ||
                    (pattern_generator[k] != 0)) {
                if (pattern_generator[k] == 0) {
                    pattern_generator[k] = PATTERN_MOVEMENTS;
                } else {
                    pattern_generator[k]--;
                }
                hub_gpio_config[k].current_state = read_gpio;
                if (hub_gpio_config[k].gpio_pisr != NULL) {
                    ESP_LOGI(TAG, "Toggled [%d], value [%d]", k, read_gpio);
                    drv_gpio_dispatcher(k, read_gpio);

                }
            }
        }
        task_sleep(2);}
}

void gpio_set_rest_points(void) {
    for (uint8_t k = 0; k < TOTAL_HUB_GPIOS; k++) {
        if(hub_gpio_config[k].low_level_config.mode == GPIO_MODE_INPUT){
            hub_gpio_config[k].current_state = drv_gpio_read(k);
        }
    }
}

void drv_gpio_init(void) {
    drv_gpio_configure_pins();
    gpio_set_rest_points();
}

gpio_pattern_t gpio_find_best_pattern(hub_gpio_t pushed_gpio);
//static bool predefined_patterns[TOTAL_PREDEFINED_PATTERNS][PATTERN_MOVEMENTS];
static bool harvested_events[TOTAL_HUB_GPIOS][PATTERN_MOVEMENTS] = {0};

void drv_gpio_dispatcher(hub_gpio_t pushed_gpio, bool gpio_state) {
    ESP_LOGI(TAG,"Putting [%d] in [%d], in space [%d] ",gpio_state,pushed_gpio,
            PATTERN_MOVEMENTS - pattern_generator[pushed_gpio]);

    harvested_events[pushed_gpio][PATTERN_MOVEMENTS - pattern_generator[pushed_gpio]] = gpio_state;
    if (pattern_generator[pushed_gpio] == 0) {
        gpio_pattern_t detected_pattern = gpio_find_best_pattern(pushed_gpio);
        hub_gpio_config[pushed_gpio].gpio_pisr[detected_pattern]();
    }
}

gpio_pattern_t gpio_find_best_pattern(hub_gpio_t pushed_gpio) {
    bool prev_space = harvested_events[pushed_gpio][0];
    uint8_t total_toggles = 0;
    ESP_LOGI(TAG,"COunted [%d] toggles",total_toggles);
    for (uint8_t k = 1; k < PATTERN_MOVEMENTS; k++) {
        if (prev_space != harvested_events[pushed_gpio][k]) {
            total_toggles++;
        }
        prev_space = harvested_events[pushed_gpio][k];
    }
    if (total_toggles < 3) {
        ESP_LOGI(TAG,"Fixed pattern [%d] blinks",total_toggles);
        return pattern_fixed;
    } if (total_toggles >= 3) {
        ESP_LOGI(TAG,"Blinking pattern [%d] blinks",total_toggles);
        return pattern_blink;

    } else {
        ESP_LOGE(TAG,"ERROR PATTERN");
        return pattern_glitch;
    }
}
