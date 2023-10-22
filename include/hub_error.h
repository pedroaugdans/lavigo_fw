/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   hub_error.h
 * Author: independent contractor
 *
 * Created on November 13, 2019, 4:25 AM
 */

#ifndef HUB_ERROR_H
#define HUB_ERROR_H
#include "esp_err.h"
#include "params.h"

typedef uint16_t hub_error_t;   




#define HUB_OK                              0x0
#define HUB_ERROR_COMM_GEN                  0x9000
#define HUB_ERROR_COMM_CONN_GEN             0x9100
#define HUB_ERROR_COMM_MQTT_GEN             0x9200
#define HUB_ERROR_COMM_MSG_GEN              0x9300
#define HUB_ERROR_COMM_MSG_NOJSON           0x9301
#define HUB_ERROR_COMM_MSG_NOFIELD          0x9304
#define HUB_ERROR_COMM_MSG_NORESOURCE       0x9302
#define HUB_ERROR_COMM_MSG_NOACTION         0x9303

// EXECUTION
#define HUB_ERROR_EXEC_GEN                  0xA000
#define HUB_ERROR_EXEC_UNEXISTANT_RESORCE   0xA001
#define HUB_ERROR_EXEC_UNEXISTANT_ACTION    0xA002
#define HUB_ERROR_EXEC_LOWLEVEL_GEN         0xA100
#define HUB_ERROR_EXEC_LOOPBACK_GEN         0xA200
#define HUB_ERROR_EXEC_FORBIDDEN_ACTION     0xA201
#define HUB_ERROR_EXEC_FORBIDDEN_TRIGGER    0xA202
#define HUB_ERROR_EXEC_UNSENT_MSG           0xA203
// DEPLOYMENT
#define HUB_ERROR_DEPL_GEN                  0xB000
#define HUB_ERROR_DEPL_RESOURCE_EXISTS      0xB001
#define HUB_ERROR_DEPL_PORT_BUSY            0xB002
#define HUB_ERROR_DEPL_MISSING_PORT         0xB003
#define HUB_ERROR_DEPL_MISSING_SIGNAL       0xB004
#define HUB_ERROR_DEPL_MISSING_ACTIONS      0xB005
#define HUB_ERROR_DEPL_MISSING_EVENT        0xB006
#define HUB_ERROR_DEPL_UNEXISTANT_PORT      0xB007
#define HUB_ERROR_DEPL_UNDEFINED_VERSION    0xB008
#define HUB_ERROR_DEPL_ED                   0xB100

//monitoring
#define HUB_ERROR_MONITOR_GEN               0xC000
#define HUB_ERROR_MONITOR_MSG_GEN           0xC100
#define HUB_ERROR_CONSOLE_GEN               0xC200
//config
#define HUB_ERROR_CONFIG_GEN                0xD000
#define HUB_ERROR_CONFIG_FSM_GEN            0xD100
#define HUB_ERROR_MACHINE_GEN               0xD200
#define HUB_ERROR_MACHINE_ENGINE_GEN        0xD300
#define HUB_ERROR_MACHINE_FLOW_GEN          0xD400
#define HUB_ERROR_DRV_GEN                   0x8000
// HAL
#define HUB_ERROR_                          0xF000
#define HUB_ERROR_DRV_MEM_GEN               0xF100
#define HUB_ERROR_UPDATES_GEN               0xF000
#define HUB_ERROR_GENERIC                   0xFFFF

#endif