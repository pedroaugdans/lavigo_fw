/***
 *     /$$      /$$  /$$$$$$  /$$   /$$ /$$$$$$ /$$$$$$$$ /$$$$$$  /$$$$$$$  /$$$$$$ /$$   /$$ /$$$$$$$$
 *    | $$$    /$$$ /$$__  $$| $$$ | $$|_  $$_/|__  $$__//$$__  $$| $$__  $$|_  $$_/| $$$ | $$|__  $$__/
 *    | $$$$  /$$$$| $$  \ $$| $$$$| $$  | $$     | $$  | $$  \ $$| $$  \ $$  | $$  | $$$$| $$   | $$
 *    | $$ $$/$$ $$| $$  | $$| $$ $$ $$  | $$     | $$  | $$  | $$| $$$$$$$/  | $$  | $$ $$ $$   | $$
 *    | $$  $$$| $$| $$  | $$| $$  $$$$  | $$     | $$  | $$  | $$| $$__  $$  | $$  | $$  $$$$   | $$
 *    | $$\  $ | $$| $$  | $$| $$\  $$$  | $$     | $$  | $$  | $$| $$  \ $$  | $$  | $$\  $$$   | $$
 *    | $$ \/  | $$|  $$$$$$/| $$ \  $$ /$$$$$$   | $$  |  $$$$$$/| $$  | $$ /$$$$$$| $$ \  $$   | $$
 *    |__/     |__/ \______/ |__/  \__/|______/   |__/   \______/ |__/  |__/|______/|__/  \__/   |__/
 *
 *
 *
 */

#include "monitoring.h"
#include "machines.h"
#include "msg_fmt.h"
#include "messages.h"
#include "registration.h"
#include "lavigoMasterFSM.h"
#include "launcher.h"
#include "params.h"
#include "msg_raw.h"
#include "cJSON.h"
#include "string.h"
#include "pin_seq.h"
#include "port_xio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define FOREVER  while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define TO_HUB   1
#define FROM_HUB 0
#define MONITOR_RUNNING  1
#define MONITOR_IDLE     0

#define MONITORING_PERIOD 1
/*** VARIABLES **************************************************************************/

static const char *TAG = "[MONITOR]";
static bool hub_reconnected = FALSE;
static bool hub_recovery = FALSE;
static uint8_t witness = 0;
bool hub_report = FALSE;
static uint32_t monitored_dram = 0, monitored_iram = 0;
/*This array contains whether or not to assembly available self-reports*/
bool data_to_report[TOTAL_REPORT_DATA] = {0};

SemaphoreHandle_t monitoring_msg_lock = NULL;
SemaphoreHandle_t monitoring_enquiry_lock = NULL;

/*** DECLARATIONS ***********************************************************************/

/*FUNCTION****************************************/
/*monitoring_setState Sset the running confirmation flag for this task. Intended for semaphoring*/
void monitoring_setState(bool state);

/*FUNCTION****************************************/
/*reset_verbosity Set all standard self-report messages items to assembly on TRUE*/
void reset_verbosity(void);

/*FUNCTION****************************************/
/*task_check_heap Fill input with hub RTOS information. Only memory*/
/*Parameters**************************************/
/*DRAM_out: Memory available to store 8-bit packed information, forced by malloc (so far)*/
/*IRAM_out: Memory available to stor 32-bit packed information*/
static void task_check_heap(uint32_t * DRAM_out, uint32_t * IRAM_out);

/*FUNCTION****************************************/
/*assemble_variables_msg Fill hub RTOS information report*/
/*Parameters**************************************/
/*DRAM_out: Memory available to store 8-bit packed information, forced by malloc (so far)*/
/*IRAM_out: Memory available to stor 32-bit packed information*/
static void assemble_variables_msg(uint32_t monitored_dram, uint32_t monitored_iram);

/*FUNCTION****************************************/
/*append_log Append a device log (dynamic instant characterization) to an array*/
/*Parameters**************************************/
/*log_toappend: Log information holder*/
static void append_log(hub_offline_log * log_toappend);

/***
 *     _            _
 *    | |          | |
 *    | |_ __ _ ___| | __
 *    | __/ _` / __| |/ /
 *    | || (_| \__ \   <
 *     \__\__,_|___/_|\_\
 *
 *
 */
static void task_run(void);
static void task_idle(void);
static bool task_disengage(void);
static bool task_isDisengage(void);
static bool task_isEngage(void);
static bool task_isRun(void);
static bool task_engage(void);
static void task_init(void);
static void task_sleep(uint8_t ticks);
static void task_run_offline(void);
static bool task_isConnected(void);
static void task_engagement_done(void);
static void task_disengagement_done(void);
bool monitoring_isActive();
bool monitoring_isRunning();
void monitoring_sleep(uint8_t ticks);


/*** DEFINITIONS ************************************************************************/

void monitoring_task(void *pvParameters) {
    task_init();
    FOREVER{
        if (task_isEngage()) {
            bool check_running_flag = task_engage();
            if (check_running_flag) {
                task_engagement_done();
            }

        } else if (task_isRun()) {
            if (task_isConnected()) {
                task_run();
            } else {
                task_run_offline();
            }

        } else if (task_isDisengage()) {
            bool uncheck_running_flag = task_disengage();
            if (uncheck_running_flag) {
                task_disengagement_done();
            }

        } else {
            task_idle();
        }
        task_sleep(2);
      }
}

static void monitor_disconnection(void) {
    if (witness != task_isConnected()) {
        ESP_LOGW(TAG, "disconnection toggle");
        witness = task_isConnected();
        if (task_isConnected()) {
            hub_reconnected = TRUE;
        }
    }
}

static void task_engagement_done(void) {
    witness = task_isConnected();
    engage_activation_flags[monitor_engage_flag] = 0;
}

static void task_disengagement_done(void) {
    disengage_activation_flags[monitor_disengage_flag] = 0;
}

static bool task_isConnected(void) {
    return hub_isConnected;
}

static void task_run_offline(void) {
  task_check_heap(&monitored_dram, &monitored_iram);
  monitor_disconnection();
  monitoring_sleep(MONITORING_PERIOD);
}

static void dispatch_info_equiry(cJSON *monitor_info){
  bool is_resource = (!_check_jSONField(monitor_info,"resource")) ? true:false;
  bool is_sequence = (!_check_jSONField(monitor_info,"sequence")) ? true:false;
  bool is_xio = (!_check_jSONField(monitor_info,"xio")) ? true:false;
  ESP_LOGI(TAG,"Received resource? [%d] sequence? [%d]",is_resource,is_sequence);
  if(is_xio){
    uint8_t new_status= (uint8_t)_get_jSONField(monitor_info, "xio")->valuedouble;
    set_logging(new_status);
  }
  else if(is_resource && !is_sequence){
    uint8_t resource_name = (uint8_t)_get_jSONField(monitor_info, "resource")->valuedouble;
    if(machine_check_existance(resource_name) != FAILURE){
      ESP_LOGI(TAG,"Sending resource info on next report");
      machine_need_report_set(resource_name);
    } else {
      ESP_LOGE(TAG,"No such machine to [report]");
    }
  } else if(is_resource && is_sequence){
    uint8_t resource_name = (uint8_t)_get_jSONField(monitor_info, "resource")->valuedouble;
    if(machine_check_existance(resource_name) != FAILURE){
      ESP_LOGI(TAG,"Sending sequence info on next report");
      pin_seq_report_machine_seq_set(resource_name);
    } else {
      ESP_LOGE(TAG,"No such sequence to [report]");
    }
  }

}

static void dispatch_report_enquiry(cJSON *monitor_info) {
  if(monitor_info != NULL){
    dispatch_info_equiry(monitor_info);
  } else {
    ESP_LOGI(TAG,"No details asked");
    reset_verbosity();
  }
  set_hub_report();
    if (sph_take_delayed(&monitoring_msg_lock, LOCK_MSG_INTERVAL) == TRUE) {
        ESP_LOGI(TAG,"Report enquired from handler");
        sph_give(&monitoring_msg_lock);
    }
}

void reset_verbosity(void) {
    //TODO: switch between verbosity levels
    ESP_LOGI(TAG,"Reseting verbocity on monitoring fields");
    data_to_report[resources_usage_data] = TRUE;
    data_to_report[firmware_data] = TRUE;
    data_to_report[running_mode_data] = TRUE;
    data_to_report[timestamp_data] = TRUE;
}

static void assemble_error_object(void)__attribute__((unused));
static void assemble_error_object(void){
    msg_fmt_start_object_item(Monitoring);
    for(uint8_t k = 0; k < totalThreads; k++){
      if(service_error[k]!=0){
        ESP_LOGE(TAG,"logging error %d on %s",service_error[k],monitoring_error_keys_fields[k]);
      }
    }
    msg_format_add_object(Monitoring, ERROR_FIELD);
}

void set_hub_report(void){
  hub_report = true;
}

bool get_hub_report(void){
  bool someone_needs_report = any_machine_need_report_get();
  return someone_needs_report||hub_report;
}


static void clean_joker_buffer(char * buffer,uint8_t size){
  for(uint8_t k = 0;k<size;k++){
    buffer[k] = 0;
  }
}

static void assemble_sequence_info_msg(void){
  pin_seq_sequence_t sequence_to_report;
  uint8_t seq_idx = 0;
  char joker_buffer[25] = {0};
  do {
    seq_idx = pin_seq_report_get_next(&sequence_to_report);
    if(seq_idx == FAILURE){
      ESP_LOGW(TAG,"No more sequencies to print");
      break;
    }
    msg_fmt_start_object_item(Monitoring);
    msg_fmt_add_int_toarray(Monitoring, "machine", sequence_to_report.machine_p->deployment_info.resource);
    msg_fmt_add_str_toarray(Monitoring, "name", sequence_to_report.sequence_p->name);
    msg_fmt_add_str_toarray(Monitoring, "status", sequence_to_report.sequence_p->status);
    msg_fmt_add_str_toarray(Monitoring, "pattern", sequence_to_report.sequence_p->pattern);
    msg_fmt_add_int_toarray(Monitoring, "target", sequence_to_report.sequence_p->target);
    msg_fmt_add_int_toarray(Monitoring, "idx", sequence_to_report.index);
    clean_joker_buffer(joker_buffer,25);
    snprintf(joker_buffer, 7,"seq%03d", seq_idx);
    msg_format_add_object(Monitoring, joker_buffer);
  } while(seq_idx != FAILURE);
}

static void assemble_machine_info_msg(void){
  machines_machine_t *machine_p = machine_report();
  if(machine_p == NULL){
    ESP_LOGE(TAG,"Cannot report NULL machine");
    return;
  } else {
    char joker_buffer[25] = {0};
    hub_offline_log flags_log;
    msg_fmt_start_object_item(Monitoring);
    /*signal*/msg_fmt_add_int_toarray(Monitoring, "mach start", machine_p->deployment_info.signals[0][0].port);
    /*signal*/msg_fmt_add_int_toarray(Monitoring, "mach ack", machine_p->deployment_info.signals[0][1].port);
    /*signal*/msg_fmt_add_int_toarray(Monitoring, "ret start", machine_p->deployment_info.signals[1][0].port);
    /*signal*/msg_fmt_add_int_toarray(Monitoring, "ret ack", machine_p->deployment_info.signals[1][1].port);
    /*Mach Actions*/for(uint8_t k = 0; k < machine_p->deployment_info.nof_sequences[0][0]; k++){
    /*Mach Actions*/      clean_joker_buffer(joker_buffer,25);
    /*Mach Actions*/      snprintf(joker_buffer, 19,"mach-act[%s]",machine_p->deployment_info.sequences[0][0][k].name);
    /*Mach Actions*/      msg_fmt_add_str_toarray(Monitoring, joker_buffer, machine_p->deployment_info.sequences[0][0][k].pattern);
    /*Mach Actions*/}
    /*Mach Events*/for(uint8_t k = 0; k < machine_p->deployment_info.nof_sequences[0][1]; k++){
    /*Mach Events*/      clean_joker_buffer(joker_buffer,25);
    /*Mach Events*/      snprintf(joker_buffer, 19,"mach-evt[%s]",machine_p->deployment_info.sequences[0][1][k].name);
    /*Mach Events*/      msg_fmt_add_str_toarray(Monitoring, joker_buffer, machine_p->deployment_info.sequences[0][1][k].pattern);
    /*Mach Events*/}
    /*ret Actions*/for(uint8_t k = 0; k < machine_p->deployment_info.nof_sequences[1][0]; k++){
    /*ret Actions*/      clean_joker_buffer(joker_buffer,25);
    /*ret Actions*/      snprintf(joker_buffer, 19,"ret-act[%s]",machine_p->deployment_info.sequences[1][0][k].name);
    /*ret Actions*/      msg_fmt_add_str_toarray(Monitoring, joker_buffer, machine_p->deployment_info.sequences[1][0][k].pattern);
    /*ret Actions*/}
    /*ret Events*/for(uint8_t k = 0; k < machine_p->deployment_info.nof_sequences[1][1]; k++){
    /*ret Events*/      clean_joker_buffer(joker_buffer,25);
    /*ret Events*/      snprintf(joker_buffer, 19,"ret-evt[%s]",machine_p->deployment_info.sequences[1][1][k].name);
    /*ret Events*/      msg_fmt_add_str_toarray(Monitoring, joker_buffer, machine_p->deployment_info.sequences[1][1][k].pattern);
    /*ret Events*/}
    /*flags*/
    assemble_log(machine_p,&flags_log);
    append_log(&flags_log);
  int32_t buffer_name = machine_p->deployment_info.resource;
  if(buffer_name>128 || buffer_name<1){
    buffer_name = 0;
  }
  clean_joker_buffer(joker_buffer,25);
  snprintf(joker_buffer, 7,"res%03d", buffer_name);
  msg_format_add_object(Monitoring, joker_buffer);
  }
}

static void assemble_report_msg(void) {
    msg_fmt_edit_str(Monitoring, (char *) msg_fields[actionField], monitoring_fields[monitoring_type_report]);
    msg_fmt_start_object_item(Monitoring);
    ESP_LOGI(TAG,"Assembling report msg");
    bool append_info = FALSE;
    if (data_to_report[resources_usage_data]) {
        assemble_variables_msg(monitored_dram, monitored_iram);
        data_to_report[resources_usage_data] = 0;
        append_info = TRUE;
    }
    if (data_to_report[running_mode_data]) {
        msg_fmt_add_str_toarray(Monitoring, (char *) monitoring_config_fields[config_mode_field],
                hub_mode_msg[fallback_or_running]);
        data_to_report[running_mode_data] = 0;
        append_info = TRUE;
    }
    if (data_to_report[firmware_data]) {
        msg_fmt_add_str_toarray(Monitoring, (char *) monitoring_config_fields[config_fw_fields],
                firmware_version);
        data_to_report[firmware_data] = 0;
        append_info = TRUE;
    }
    if(data_to_report[timestamp_data]){
      msg_fmt_add_int_toarray(Monitoring, (char *) monitoring_config_fields[config_ts_fields],
              get_hub_timestamp());
      data_to_report[timestamp_data] = 0;
      append_info = TRUE;
    }
    if(append_info){
      msg_format_add_object(Monitoring, INFO);
    }
    if(any_machine_need_report_get()){
      assemble_machine_info_msg();
    }
    if(any_sequence_need_report_get() == SUCCESS){
      assemble_sequence_info_msg();
    }
    hub_report = FALSE;
}

static void append_xio(port_status_t status_to_send){
  msg_fmt_add_int_toarray(Monitoring, (char *) monitoring_fallback_msg[fallback_port_field], status_to_send.port);
  msg_fmt_add_int_toarray(Monitoring, (char *) monitoring_fallback_msg[fallback_level_field], status_to_send.status);
  msg_fmt_add_int_toarray(Monitoring, (char *) monitoring_fallback_msg[fallback_tstamp_field], status_to_send.timestamp);
}

static void assemble_xio_msg(void){
    int8_t remaining_msg = 5;
    port_status_t status_handler;
    msg_fmt_edit_str(Monitoring, (char *) msg_fields[actionField], monitoring_fields[monitoring_type_xio]);
    do {
      if(get_next_xio_msg(&status_handler) == FAILURE){
        ESP_LOGI(TAG,"No more XIO msgs to log!");
        break;
      }
      msg_fmt_start_object_item(Monitoring);
      append_xio(status_handler);
      msg_fmt_append_array(Monitoring, INFO);
      remaining_msg--;
    } while(remaining_msg > 0);
}

static uint8_t monitor_check_input_msg(cJSON *root){
  if (!root) {
      ESP_LOGE(TAG, "Input msg json invalid");
      return FAILURE;
  }
  if (_check_jSONField(root,
          msg_fields[actionField])) {
      ESP_LOGE(TAG, "No config field");
      stash_jSON_msg(root);
      return FAILURE;
  }
  return SUCCESS;
}

static void _monitor_callback(void) {
    cJSON *root = cJSON_Parse(msg_fmt_message_static(Monitoring));

    if(monitor_check_input_msg(root) == FAILURE){
      stash_jSON_msg(root);
      return;
    }

    char * received_order = cJSON_GetObjectItem(root,
            msg_fields[actionField])->valuestring;

    /************** Dispatch configuration*/
    if (!strcmp(FALLBACK_MODE, received_order)) {
        if (fallback_or_running == hub_running_mode) {
            fsm_q_evt(trgfb_Mevent);
        } else {
            ESP_LOGW(TAG, "HUB already en FALLBACK mode");
            set_hub_report();
        }
    } else if (!strcmp(SILENCE_MODE, received_order)) {

    } else if (!strcmp(BOOTUP_MODE, received_order)) {
        if (fallback_or_running == hub_fallback_mode) {
            fsm_q_evt(trgfb_Mevent);
        } else {
            ESP_LOGW(TAG, "HUB already en BOOTUP mode");
            set_hub_report();
        }
    } else if (!strcmp(REPORT_MODE, received_order)) {
        cJSON *monitor_info = cJSON_GetObjectItem(root,INFO);
        dispatch_report_enquiry(monitor_info);
    } else if(!strcmp(SILENCE_MODE, received_order)){

    } else {
        ESP_LOGE(TAG, "Unknown monitoring action");
    }
    stash_jSON_msg(root);
}

static void task_init(void) {
    sph_create(&monitoring_msg_lock);
    msg_fmt_install(Monitoring, _monitor_callback);
    esp_timer_create_args_t oneshot_timer_args = {
        .callback = &hub_timestamp_update,
        /* argument specified here will be passed to timer callback function */
        .arg = (void*) NULL,
        .name = "one-shot"
    };
    esp_timer_create(&oneshot_timer_args, &hub_timer_handler);
    esp_timer_start_once(hub_timer_handler, TIMESTAMP_INTERLEAVE_US);
    reset_verbosity();
}

static bool task_engage(void) {
    monitoring_setState(MONITOR_RUNNING);
    return TRUE;
}

void dump_recovery_test(void) {
    machines_machine_t *machine_p;
    machines_machine(&machine_p, 4);
    hub_reconnected = TRUE;
    hub_recovery = TRUE;
    if (machine_p == NULL) {
        return;
    }
    log_transaction_step(machine_p, 1);
    log_transaction_step(machine_p, 1);
    log_transaction_step(machine_p, 1);
}

static void assemble_reconnection_msg(void) {
    ESP_LOGI(TAG, "Assembling reconnect msg");
    msg_fmt_edit_str(Monitoring, (char *) msg_fields[actionField], monitoring_fields[monitoring_type_reconnected]);
    hub_reconnected = FALSE;
}

static void append_log(hub_offline_log * log_toappend) {
    msg_fmt_add_int_toarray(Monitoring,
       monitoring_recovery_fields[idle_recovery_field], log_toappend->is_idle);
    msg_fmt_add_int_toarray(Monitoring,
       monitoring_recovery_fields[enable_recovery_field], log_toappend->is_allowed);
    msg_fmt_add_int_toarray(Monitoring,
       monitoring_recovery_fields[control_recovery_field], log_toappend->transaction_control);
    msg_fmt_add_int_toarray(Monitoring,
       monitoring_recovery_fields[resource_recovery_field], log_toappend->resource);
    msg_fmt_add_str_toarray(Monitoring,
       monitoring_recovery_fields[action_recovery_field], log_toappend->name);
    msg_fmt_add_int_toarray(Monitoring,
       monitoring_recovery_fields[new_sequence_recovery_field], log_toappend->sequence_changed);
         //monitoring_recovery_fields[new_sequence_recovery_field], log_toappend->sequence_changed);
    //complete...
}

static void assemble_recovery_msg(void) {
    //uint8_t keep_going = TRUE, remaining_msg = 5;
    uint8_t remaining_msg = 5;
    hub_offline_log log_toappend;
    msg_fmt_edit_str(Monitoring, (char *) msg_fields[actionField], monitoring_fields[monitoring_type_recovery]);
    ESP_LOGI(TAG, "Assembling recover msg");
    do {
        //keep_going = unlog_machine(&log_toappend);
        if(!unlog_machine(&log_toappend)){
          data_to_report[recovery_data] = false;
          break;
        }
        msg_fmt_start_object_item(Monitoring);
        append_log(&log_toappend);
        msg_fmt_append_array(Monitoring, INFO);
        remaining_msg--;
    } while (remaining_msg);
    ESP_LOGI(TAG, "sending %s", msg_raw_message(Monitoring, FROM_HUB));
    ESP_LOGI(TAG, "Sending recovery msg");
}

static uint8_t assemble_heartbeat_msg(void) {
    if(msg_fmt_load(Monitoring) == FAILURE){
      ESP_LOGW(TAG,"Sending another message on topic");
      return FAILURE;
    }
    msg_fmt_edit_str(Monitoring, (char *) msg_fields[actionField], monitoring_fields[monitoring_type_heartbeat]);
    msg_fmt_edit_str(Monitoring, (char *) msg_fields[hubIdField], (char *) params_get(HUBID_PARAM));
    return SUCCESS;
}

static void assemble_variables_msg(uint32_t monitored_dram, uint32_t monitored_iram) {
    msg_fmt_add_int_toarray(Monitoring, (char *) monitoringVariables[DRAM_monitoringField],
            monitored_dram);
    msg_fmt_add_int_toarray(Monitoring, (char *) monitoringVariables[IRAM_monitoringField],
            monitored_iram);
}

static void assemble_dump_msg(void) {
    hub_offline_log log_toappend;
    uint8_t max_logs_per_msg = 5;

    msg_fmt_edit_str(Monitoring, (char *) msg_fields[actionField], monitoring_fields[monitoring_type_dump]);
    ESP_LOGI(TAG, "Assembling DUMP msg");

    do{
      msg_fmt_start_object_item(Monitoring);
      unlog_transaction_step(&log_toappend);
      append_log(&log_toappend);
      msg_fmt_append_array(Monitoring, INFO);
      max_logs_per_msg--;
      if(!get_remaining_offline_logs()){
        need_monitor_dump = FALSE;
        ESP_LOGI(TAG,"No more logs to log");
      }
    } while ((max_logs_per_msg));
}


static uint32_t publishing_divider = 0;

bool get_xio_logging(void){
  return is_any_msg();
}

static void task_run(void) {
    task_check_heap(&monitored_dram, &monitored_iram);
    monitor_disconnection();
    if (sph_take_delayed(&monitoring_msg_lock, LOCK_MSG_INTERVAL) == TRUE) {
        if(assemble_heartbeat_msg() == FAILURE){
          ESP_LOGW(TAG,"Sending message on same topic, retrying later");
          sph_give(&monitoring_msg_lock);
          return;
        }
        if (hub_reconnected) {
            logs_flush_ram();
            assemble_reconnection_msg();
            msg_fmt_send(Monitoring);
        } else if (data_to_report[recovery_data]) {
            assemble_recovery_msg();
            msg_fmt_send(Monitoring); /*<! IMMIDIATE send msg when RECOVERY*/
            ESP_LOGI(TAG,"Hub reconvery message sent");
        } else if (need_monitor_dump) {
            assemble_dump_msg();
            msg_fmt_send(Monitoring); /*<! IMMIDIATE send msg when MONITOR*/
        } else if (get_hub_report()) {
            assemble_report_msg();
            msg_fmt_send(Monitoring);
          } else if (get_xio_logging()) {
            assemble_xio_msg();
            msg_fmt_send(Monitoring);
        } else if (!(publishing_divider % HEARTBEAT_MONITOR)) {
            msg_fmt_send(Monitoring);
            ESP_LOGI(TAG, "Sending monitoring message...");
        }
        sph_give(&monitoring_msg_lock);
    } else {
        ESP_LOGE(TAG, "Monitoring msg semaphore untaken");
    }

    if(!(publishing_divider % REPORT_MONITOR)){
      reset_verbosity();
      set_hub_report();
    }
    monitoring_sleep(MONITORING_PERIOD);
    publishing_divider++;
}

static bool task_disengage(void) {
    monitoring_setState(MONITOR_IDLE);
    return TRUE;
}

static bool task_isRun(void) {
    return run_activation_flags[monitor_run_flag];
}

static bool task_isEngage(void) {
    return engage_activation_flags[monitor_engage_flag];
}

static bool task_isDisengage(void) {
    return disengage_activation_flags[monitor_disengage_flag];
}

static void task_idle(void) {
    //ESP_LOGI(TAG, "idle()");
    task_sleep(2);
}

bool monitoring_isActive() {
    return threadFlags[monitor_flag];
}

bool monitoring_isRunning() {
    return run_confirmation_flag[monitor_current];
}

void monitoring_setState(bool state) {
    run_confirmation_flag[monitor_current] = state;
}

static void task_sleep(uint8_t ticks) {
    //ESP_LOGI(TAG, "task_sleep()");
    monitoring_sleep(ticks);
}

void monitoring_sleep(uint8_t ticks) {
    //ESP_LOGI(TAG, "monitoring_sleep()");
    set_ram_usage(uxTaskGetStackHighWaterMark(NULL), monitoring_task_idx);
    vTaskDelay(suspendedThreadDelay[monitor_flag] * ticks / portTICK_PERIOD_MS);
    //ESP_LOGI(TAG, "~monitoring_sleep()");
}

static void task_check_heap(uint32_t * DRAM_out, uint32_t * IRAM_out) {
    (*DRAM_out) = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    (* IRAM_out) = heap_caps_get_free_size(MALLOC_CAP_32BIT);
    ESP_LOGI(TAG,"xPortGetFreeHeapSize :%d\n", xPortGetFreeHeapSize() );
    ESP_LOGI(TAG, "/***********/MONITOR STATUS/***********/ \nAvailable DRAM [%d], iRAM [%d] @ [%d] \nmode: [%s], layout [%s]",
            (*DRAM_out), (* IRAM_out), get_hub_timestamp()
            ,hub_mode_msg[fallback_or_running]
            , current_layout_version ? "VERSION_0_5_1" : "VERSION_0_5_0");
}
