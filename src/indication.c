/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "indication.h"
#include "drv_gpio.h"
#include "esp_log.h"

#define FOREVER while(1)

static const char * TAG = "[INDICATION]";

static bool led_indication_sequencies[MAX_LED_INDICATIONS][AMOUNT_OF_LEDS][LED_INDICATION_SPACES] = {
    INDICATOR_STARTING1,
    INDICATOR_STARTING2,
    INDICATOR_STARTING3,
    INDICATOR_STARTING4,
    INDICATOR_STARTING5,
    INDICATOR_STARTING6,
    INDICATOR_STARTING7,
    INDICATOR_STARTING8,
    INDICATOR_STARTING9,
    INDICATOR_STARTING10
};
static led_indication_t current_sequence = 0;

void set_current_led_indication_sequence(led_indication_t next_sequence) {
    if (next_sequence > MAX_LED_INDICATIONS) {
        ESP_LOGE(TAG, "Unexpected led sequence");
        return;
    }
    current_sequence = next_sequence;
}

led_indication_t get_current_led_indication_sequence(void) {
    return current_sequence;
}

static void task_sleep(uint8_t ticks) {
    vTaskDelay(100 * ticks / portTICK_RATE_MS);
}

void update_leds(uint8_t next_step) {
    drv_gpio_set(GPIO_LED_0, led_indication_sequencies[current_sequence][0][next_step]);
    drv_gpio_set(GPIO_LED_1, led_indication_sequencies[current_sequence][1][next_step]);

}

void indication_task(void *params) {
    uint8_t sequence_step = 0;
    FOREVER{
        update_leds(sequence_step);
        sequence_step++;
        sequence_step %= LED_INDICATION_SPACES;
        task_sleep(2);}
}

