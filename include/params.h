#ifndef __PARAMS_H__
#define __PARAMS_H__

#include <stdbool.h>
#include <stdint.h>
#include "drv_locks.h"
#include "esp_timer.h"
#include "hubware.h"
#define MASTER_FSM_CHANGED

#define DEVZID_SECRET_LENGTH    6
#define IOTURL_SECRET_LENGTH   50
#define SYSSTG_SECRET_LENGTH   10
#define HUBZID_SECRET_LENGTH   40
#define ECCCRT_SECRET_LENGTH 1100
#define ECCKEY_SECRET_LENGTH  400
#define AWSCRT_SECRET_LENGTH 1800
#define UPDKEY_SECRET_LENGTH 1800
#define APSSID_CONFIG_MIN_LENGTH 4
#define APPSWD_CONFIG_MIN_LENGTH 4
#define MAPIDX_MIN_LEN 4
#define MONITOR_VARIABLES 1

#define HUB_DEFAULT_STARTUP_MODE    HUB_NORMAL_MODE_KEY
#define HUB_LAST_MODE_KEY   "hubmod"
#define HUB_NORMAL_MODE_KEY   "runmod"
#define HUB_FALLBACK_MODE_KEY   "fbkmod"

#define VERSION_0_5_0 "version_0_5_0"
#define VERSION_0_5_1 "version_0_5_1"

typedef enum {
    version_5_0,
    version_5_1,
    TOTAL_LAYOUT_VERSIONS
} layout_version_t;

typedef enum {
  parameters_0,
  parameters_1,
  unselected_parameter,
  TOTAL_PARAMETERS
} drv_keys_buffer_t;

typedef struct {
  drv_keys_buffer_t key_flash_idx;
  uint8_t key_version;
} drv_key_storage;

#define KEYS_CURRENT_BUFFER "keybuf"
#define KEYS_LAST_BUFFER "keylast"
#define KEYS_UNCOFIRMED_ROL "rolstat"
#define MAX_ROTATIONS_ATTEMPTS 2

extern drv_key_storage current_used_keys[];
extern drv_key_storage last_used_keys[];
extern bool rol_status[];
void assemble_key_name(char * name_to_return,const char * key,drv_keys_buffer_t parameter_buffer);

extern char firmware_version[];
#define DEFAULT_HARDWARE_VERSION version_5_0
extern bool is_hardware_version_selected;
extern layout_version_t current_layout_version;

/***
 *       ___                              _
 *      / __|___ _ __  _ __  __ _ _ _  __| |___
 *     | (__/ _ \ '  \| '  \/ _` | ' \/ _` (_-<
 *      \___\___/_|_|_|_|_|_\__,_|_||_\__,_/__/
 *
 */
#define DEVZID_IDX "devzid"
#define IOTURL_IDX "ioturl"
#define SYSSTG_IDX "sysstg"
#define HUBZID_IDX "hubzid"
#define ECCKEY_IDX "ecckey"
#define ECCCRT_IDX "ecccrt"
#define AWSCRT_IDX "awscrt"
#define APSSID_IDX "apssid"
#define APPSWD_IDX "appswd"
#define MAPIDX_IDX "mapidx"
#define UPDKEY_IDX "updkey"

typedef enum {
    DEVZID_CMD = 0,
    IOTURL_CMD,
    SYSSTG_CMD,
    HUBZID_CMD,
    ECCKEY_CMD,
    ECCCRT_CMD,
    AWSCRT_CMD,
    APSSID_CMD,
    APPSWD_CMD,
    UPDKEY_CMD,
    MAPIDX_CMD,
    HELPME_CMD,
    NOF_REG_CMD
} registration_command_t;

extern const char *registration_commands[];

#define REG_TERMINATOR "\n\n"
#define REG_ACK "huback\n"

/***
 *      ___ ___ __  __    __ _
 *     | __/ __|  \/  |  / _| |__ _ __ _ ___
 *     | _|\__ \ |\/| | |  _| / _` / _` (_-<
 *     |_| |___/_|  |_| |_| |_\__,_\__, /__/
 *                                 |___/
 */
typedef enum {
    /*0*/drv_mqtt_task_idx,
    /*1*/msg_fmt_i_task_idx,
    /*2*/launcher_task_idx,
    /*3*/registration_task_idx,
    /*4*/connection_task_idx,
    /*5*/validation_task_idx,
    /*6*/update_task_idx,
    /*7*/monitoring_task_idx,
    /*8*/deployment_task_idx,
    /*9*/execution_task_idx,
    /*10*/clk_pit_task_idx,
    /*11*/pin_seq_task_idx,
    /*12*/pin_gpio_task_idx,
    /*13*/pin_evt_task_idx,
    /*14*/console_task_idx,
    /*15*/FSM_task,
    /*16*/machines_update_task,
    /*17*/fallback_running_task,
    /*18*/fallback_msg_task,
    /*19*/ap_mode_task,
    /*20*/execution_msg_task,
    /*21*/msg_fmt_o_task_idx,
    /*  */MAX_TASKS
} threadtype_t;

typedef enum{
  hub_fallback_mode = 0,
  hub_running_mode = 1,
  hub_idle_mode = 2,
} hub_mode_t;

extern hub_mode_t fallback_or_running;

void print_usages(void);
void set_ram_usage(uint32_t usage, threadtype_t task_usage);
extern uint32_t ram_usage_task[MAX_TASKS];
void params_init(void);

typedef enum {
    uart_flag = 0,
    wifi_flag,
    mqtt_flag,
    update_flag,
    registration_flag,
    connection_flag,
    validation_flag,
    monitor_flag,
    deployment_flag,
    execution_flag,
    launcher_flag,
    console_flag,
    AP_flag,
    fallback_flag,
    totalThreads
} threadActivationFlag;

extern uint16_t service_error[totalThreads];

typedef enum {
    registation_check_flag = 0,
    internet_check_flag,
    cloud_check_flag,
    update_check_flag,
    total_check_activation_flags
} check_activation_flag_t;

typedef enum {
    pin_xio_run_flag = 0,
    pin_evt_run_flag,
    pin_seq_run_flag,
    clk_pit_run_flag,
    internet_run_flag,
    mqtt_run_flag,
    launcher_run_flag,
    console_run_flag,
    monitor_run_flag,
    deploy_run_flag,
    execute_run_flag,
    update_run_flag,
    AP_run_flag,
    fallback_run_flag,
    total_run_activation_flag
} run_activation_flag_t;

typedef enum {
    monitor_engage_flag = 0,
    deploy_engage_flag,
    execute_engage_flag,
    console_engage_flag,
    AP_engage_flag,
    fallback_engage_flag,
    total_engage_activation_flags
} engage_activation_flag_t;

typedef enum {
    monitor_disengage_flag = 0,
    deploy_disengage_flag,
    execute_disengage_flag,
    console_disengage_flag,
    AP_disengage_flag,
    fallback_disengage_flag,
    total_disengage_activation_flags
} disengage_deactivation_flag_t;

typedef enum {
    monitor_current = 0,
    deployment_current,
    execution_current,
    console_current,
    update_current,
    AP_current,
    fallback_current,
    total_runningFlags
} runningActivationFlags;

extern bool check_activation_flags[];
extern bool engage_activation_flags[];
extern bool disengage_activation_flags[];
extern bool run_activation_flags[total_run_activation_flag];
extern bool run_confirmation_flag[];
extern bool threadFlags[];

void dispatch_hardware_version(char * hw_ver);
void set_layout_version(layout_version_t new_version);
extern unsigned int suspendedThreadDelay[];

/***
 *       ___ ___  _  _ _  _
 *      / __/ _ \| \| | \| |  _ __  __ _ _ _ __ _ _ __  ___
 *     | (_| (_) | .` | .` | | '_ \/ _` | '_/ _` | '  \(_-<
 *      \___\___/|_|\_|_|\_| | .__/\__,_|_| \__,_|_|_|_/__/
 *                           |_|
 */

#define CONNECTION_PING_TIMEOUT 10000
#define MAX_CONNECTION_RETRIES 10

typedef enum {
    STAGE_PARAM = 0,
    HUBID_PARAM,
    HARDV_PARAM,
    NOF_PARAMS,
} param_t;
bool get_hub_connection_status(void);
bool get_hub_Online_status(void);

extern bool hub_isOnline;
extern bool hub_isConnected;
extern bool hub_no_internet;

char *params_get(param_t key);
void params_set(param_t key, char *value);

extern bool hub_isOnline;
extern char config[];
extern char *config_p;
extern char ssid[];
extern char pswd[];

void connection_printConfig(char *buffer);
void connection_clearKeys();
/***
 *      _  _ _   _ ___   _   _    _
 *     | || | | | | _ ) | |_(_)__| |__
 *     | __ | |_| | _ \ |  _| / _| / /
 *     |_||_|\___/|___/  \__|_\__|_\_\
 *
 */

#ifdef  TESTING_TIMING
#define TIMESTAMP_INTERLEAVE_US    10000
#else
#define TIMESTAMP_INTERLEAVE_US    100000
#endif
void hub_timestamp_update(void* arg);
extern uint32_t hub_timestamp;
extern esp_timer_handle_t hub_timer_handler;
uint32_t get_hub_timestamp(void);

/***
 *      _  _ _   _ ___   _
 *     | || | | | | _ ) | |___  __ _ ___
 *     | __ | |_| | _ \ | / _ \/ _` (_-<
 *     |_||_|\___/|___/ |_\___/\__, /__/
 *                             |___/
 */

#define LOGS_METADATA_NVS_KEY   "LOGMTDT"
#define LOGS_RAM_METADATA       "RAMLOGS"
#define LOG_ROOT_NAME   "log-"
#define RAM_LOG_ROOT    "raml"
extern bool need_monitor_dump;
extern uint8_t offline_log_page;
extern uint8_t ram_offline_logs;
extern SemaphoreHandle_t flash_lock;

typedef enum {
    resources_usage_data = 0,
    firmware_data,
    running_mode_data,
    recovery_data,
    machine_data,
    timestamp_data,
    TOTAL_REPORT_DATA
} report_data_t;

bool get_hub_report(void);
void set_hub_report(void);
extern bool data_to_report[];


#define SERVER_STORAGE  "WBPG"

#endif
