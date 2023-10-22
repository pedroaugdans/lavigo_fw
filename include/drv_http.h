/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   drv_http.h
 * Author: independent contractor
 *
 * Created on January 27, 2020, 12:09 PM
 */

#ifndef DRV_HTTP_H
#define DRV_HTTP_H
#include "stdint.h"

void drv_init_http(void);

#define MAX_URIS 5
#define MAX_KEYS 10

typedef void (*key_callback)(char * );

uint8_t drv_http_register_key(char * key,key_callback key_cb);

typedef struct {
    char * uri_method;
    char * uri_content;
} uri_type_t;
void drv_http_install_uri(char * method, char * content);

#endif /* DRV_HTTP_H */

