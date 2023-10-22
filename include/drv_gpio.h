/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   drv_gpio.h
 * Author: independent contractor
 *
 * Created on December 26, 2019, 10:51 AM
 */

#ifndef DRV_GPIO_H
#define DRV_GPIO_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

typedef enum {
    pattern_fixed=0,
    pattern_blink,
    pattern_glitch,
    TOTAL_GLPIO_PATTERNS
} gpio_pattern_t;

typedef enum {
    GPIO_0,
    GPIO_AP_MOD,
    GPIO_ACCEL,
    GPIO_LED_0,
    GPIO_LED_1,
    TOTAL_HUB_GPIOS
} hub_gpio_t;

typedef void (*gpio_pisr_cb)(void);

typedef struct {
    gpio_config_t low_level_config;
    uint8_t io_pin_num;
    uint8_t initial_state;
    gpio_pisr_cb gpio_pisr[TOTAL_GLPIO_PATTERNS];
    uint8_t current_state;
} hub_gpio_config_t;

#define GPIO_GPIO_0 0
#define GPIO_GPIO_AP 32
#define GPIO_GPIO_ACC 23
#define GPIO_GPIO_LED_0 21
#define GPIO_GPIO_LED_1 19

#define AMOUNT_OF_LEDS 2

#define GPIO_OUTPUT_PIN_SEL(X) (1ULL<<(X))

void drv_gpio_init(void);
void drv_gpio_high(hub_gpio_t gpio_tohigh);
void drv_gpio_low(hub_gpio_t gpio_tohigh);
bool drv_gpio_read(hub_gpio_t gpio_toread);
void drv_gpio_install_cb(hub_gpio_t hub_gpio, gpio_pisr_cb gpio_pisr,gpio_pattern_t pattern_topisr);
void drv_gpio_set(hub_gpio_t gpio_toset,bool value);
void gpio_task(void *params);


#endif /* DRV_GPIO_H */

