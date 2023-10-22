/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   machines_mapping.h
 * Author: independent contractor
 *
 * Created on February 11, 2020, 3:47 PM
 */

#ifndef MACHINES_MAPPING_H
#define MACHINES_MAPPING_H

#include "machines.h"

extern const uint8_t machines_adapters_mapping[TOTAL_LAYOUT_VERSIONS][ROW_MAX][COLUMN_MAX];
extern const uint8_t machines_adapters_delta[TOTAL_LAYOUT_VERSIONS][ROW_MAX][COLUMN_MAX];
extern const uint8_t machines_machines_mapping[TOTAL_LAYOUT_VERSIONS][ROW_MAX][COLUMN_MAX];
extern const uint8_t machines_machines_delta[TOTAL_LAYOUT_VERSIONS][ROW_MAX][COLUMN_MAX];
//extern const uint8_t machines_retrofits_mapping[TOTAL_HARDWARE_VERSIONS][ROW_MAX][COLUMN_MAX];
//extern const uint8_t machines_retrofits_delta[TOTAL_HARDWARE_VERSIONS][ROW_MAX][COLUMN_MAX];

#endif /* MACHINES_MAPPING_H */

