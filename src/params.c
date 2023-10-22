
#include "params.h"

#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "drv_locks.h"

/*** DECLARATIONS ***********************************************************************/
#define _MAX_PARAM_SIZE 64
#define FALSE 0
#define TRUE 1

/*** VARIABLES **************************************************************************/
static const char *TAG = "[PARAMS]";
static char _values[NOF_PARAMS][_MAX_PARAM_SIZE] = {0};
char firmware_version[10] = {0};

SemaphoreHandle_t ram_param_loc = NULL;
SemaphoreHandle_t timestamp_loc = NULL;

uint32_t hub_timestamp = 0;


bool need_monitor_dump = FALSE;
bool hub_isOnline = FALSE;    /*MQTT connection*/
bool hub_isConnected = FALSE; /*LAVIGO connection*/
bool hub_no_internet = FALSE;
hub_mode_t fallback_or_running = 1;


static char *_keys[] = {
    "stage",
    "hubId",
    "mapidx"
};

uint16_t service_error[totalThreads] = {
  /* UART_flag=0,         */0,
  /* WIFI_flag,           */0,
  /* MQTT_flag,           */0,
  /* update_flag,         */0,
  /* registration_flag=0, */0,
  /* connection_flag=0,   */0,
  /* validation_flag,     */0,
  /* monitor_flag,        */0,
  /* deployment_flag,     */0,
  /* execution_flag,      */0,
  /* launcher_flag,       */0,
  /* console_flag         */0,
  /* fallback_flag        */0,
  /* totalThreads         */
};

bool threadFlags[totalThreads] = {
    /* UART_flag=0,         */1,
    /* WIFI_flag,           */1,
    /* MQTT_flag,           */1,
    /* update_flag,         */0,
    /* registration_flag=0, */0,
    /* connection_flag=0,   */0,
    /* validation_flag,     */0,
    /* monitor_flag,        */0,
    /* deployment_flag,     */0,
    /* execution_flag,      */0,
    /* launcher_flag,       */0,
    /* console_flag         */0,
    /* fallback_flag         */0,
    /* totalThreads         */
};

bool check_activation_flags[total_check_activation_flags] = {
    /* registation_check_flag = 0,  */0,
    /* internet_check_flag,         */0,
    /* cloud_check_flag,            */0,
    /* update_check_flag,           */0,

    /* total_check_activation_flags */
};

bool run_activation_flags[total_run_activation_flag] = {
    /*pin_xio_run_flag = 0,*/0,
    /*pin_evt_run_flag,    */0,
    /*pin_seq_run_flag,    */0,
    /*clk_pit_run_flag,    */0,
    /*internet_run_flag,   */0,
    /*mqtt_run_flag,       */0,
    /*launcher_run_flag,   */0,
    /*console_run_flag,    */0,
    /*monitor_run_flag,    */0,
    /*deploy_run_flag,     */0,
    /*execute_run_flag,    */0,
    /*update_run_flag,     */0,
    /*AP_run_flag,         */0,
    /*fallback_run_flag    */0,
};

bool engage_activation_flags[total_engage_activation_flags] = {
    /*monitor_engage_flag = 0,     */0,
    /*deploy_engage_flag,          */0,
    /*execute_engage_flag,         */0,
    /*console_engage_flag          */0,
    /*AP_engage_flag               */0,
    /*fallback_engage_flag         */0,
    /*total_engage_activation_flags*/
};

bool disengage_activation_flags[total_disengage_activation_flags] = {
    /*monitor_disengage_flag = 0,     */0,
    /*deploy_disengage_flag,          */0,
    /*execute_disengage_flag,         */0,
    /*console_disengage_flag          */0,
    /*AP_disengage_flag               */0,
    /*fallback_disengage_flag         */0,
    /*total_disengage_activation_flags*/
};

unsigned int suspendedThreadDelay[totalThreads] = {
    /* UART_flag=0,         */1000,
    /* WIFI_flag,           */1000,
    /* MQTT_flag,           */1000,
    /* update_flag,         */1000,
    /* registration_flag=0, */200,
    /* connection_flag=0,   */100,
    /* validation_flag,     */1000,
    /* monitor_flag,        */1000,
    /* deployment_flag,     */1000,
    /* execution_flag,      */1000,
    /* launcher_flag,       */1000,
    /* console_flag         */1000,
    /* AP_run_flag          */1000,
    /* fallback_run_flag    */10,
    /* totalFlagscc         */
};

bool run_confirmation_flag[total_runningFlags] = {0};

uint32_t ram_usage_task[MAX_TASKS] = {[0 ... MAX_TASKS - 1] = 0xFFFF};

void set_ram_usage(uint32_t usage, threadtype_t task_usage) {
    if (sph_step_retries(&ram_param_loc)) {
        if (task_usage > MAX_TASKS) {
            ESP_LOGE(TAG, "Bad task");
            return;
        }
        if ((ram_usage_task[task_usage] > usage) && (usage)) {
            ram_usage_task[task_usage] = usage;
        }
        sph_give(&ram_param_loc);
    } else {
        ESP_LOGE(TAG, "[RAM-USAGE] lock untaken");
    }
}

void print_usages(void) {
    if (sph_step_retries(&ram_param_loc)) {
        for (threadtype_t k = 0; k < MAX_TASKS; k++) {
            ESP_LOGE(TAG, "Task usage at [%d] : [%d]", k, ram_usage_task[k]);
        }
        ESP_LOGI(TAG,"xPortGetFreeHeapSize :%d\n", xPortGetFreeHeapSize() );
        sph_give(&ram_param_loc);

    } else {
        ESP_LOGE(TAG, "[RAM-USAGE] lock untaken");
    }
}

/*** DEFINITIONS ************************************************************************/

bool is_hardware_version_selected = FALSE;
layout_version_t current_layout_version = DEFAULT_HARDWARE_VERSION;

void dispatch_hardware_version(char * hw_ver) {
    ESP_LOGI(TAG,"dispatch_hardware_version()");
    layout_version_t new_layout_version;
    if (!strcmp(hw_ver, VERSION_0_5_0)) {
        new_layout_version = version_5_0;
    } else if (!strcmp(hw_ver, VERSION_0_5_1)) {
        new_layout_version = version_5_1;
    } else {
        ESP_LOGE(TAG, "Hardware version unloaded");
        return;
    }
    set_layout_version(new_layout_version);
    ESP_LOGI(TAG,"hw_ver: [%s]",hw_ver);
}

void set_layout_version(layout_version_t new_version){
    current_layout_version = new_version;
    is_hardware_version_selected = TRUE;
}

void params_init(void) {
    sph_create(&timestamp_loc);
    sph_create(&ram_param_loc);
}

char *params_get(param_t key) {
    return _values[key];
}

void params_set(param_t key, char *value) {
    ESP_LOGI(TAG, "Setting %s = %s", _keys[key], value);
    strcpy(_values[key], value);
}

bool get_hub_connection_status() {
    return hub_isConnected;
}

bool get_hub_Online_status() {
    return hub_isOnline;
}

esp_timer_handle_t hub_timer_handler;

void hub_timestamp_update(void* arg) {
    //ESP_LOGI(TAG,"LOL [%d]",hub_timestamp);
    esp_timer_start_once(hub_timer_handler, TIMESTAMP_INTERLEAVE_US);

    if (sph_take_imm(&timestamp_loc) == true) {
        hub_timestamp++;
        sph_give(&timestamp_loc);
    } else {
        ESP_LOGE(TAG, "[TIMESTAMP] Lock untaken");
    }
}

uint32_t get_hub_timestamp(void) {
    uint32_t current_timestamp = 0;
    if (sph_take_imm(&timestamp_loc) == true) {
        current_timestamp = hub_timestamp;
        sph_give(&timestamp_loc);
    } else {
        ESP_LOGE(TAG, "[return TIMESTAMP] Lock untaken");
    }
    return current_timestamp;
}

void assemble_key_name(char * name_to_return,const char * key,drv_keys_buffer_t parameter_buffer){
  /*Backwards compatibility*/
  if(parameter_buffer == parameters_0){
    snprintf(name_to_return, strlen(key) + 1 + 3, "%s", key);
    ESP_LOGI(TAG,"plain key name: %s",name_to_return);
    return;
  }
  snprintf(name_to_return, strlen(key) + 1 + 3, "%s-%02d", key, parameter_buffer);
  ESP_LOGI(TAG,"key name: %s",name_to_return);
  return;
}
