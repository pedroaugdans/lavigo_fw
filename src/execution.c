/***
 *     /$$$$$$$$                                          /$$     /$$
 *    | $$_____/                                         | $$    |__/
 *    | $$      /$$   /$$  /$$$$$$   /$$$$$$$ /$$   /$$ /$$$$$$   /$$  /$$$$$$  /$$$$$$$
 *    | $$$$$  |  $$ /$$/ /$$__  $$ /$$_____/| $$  | $$|_  $$_/  | $$ /$$__  $$| $$__  $$
 *    | $$__/   \  $$$$/ | $$$$$$$$| $$      | $$  | $$  | $$    | $$| $$  \ $$| $$  \ $$
 *    | $$       >$$  $$ | $$_____/| $$      | $$  | $$  | $$ /$$| $$| $$  | $$| $$  | $$
 *    | $$$$$$$$/$$/\  $$|  $$$$$$$|  $$$$$$$|  $$$$$$/  |  $$$$/| $$|  $$$$$$/| $$  | $$
 *    |________/__/  \__/ \_______/ \_______/ \______/    \___/  |__/ \______/ |__/  |__/
 *
 *
 *
 */
#include "msg_fmt.h"
#include "messages.h"
#include "registration.h"
#include "lavigoMasterFSM.h"
#include "execution.h"
#include "params.h"
#include "pin_seq.h"
#include "pin_xio.h"
#include "pin_evt.h"
#include "clk_pit.h"
#include "string.h"
#include "drv_gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "pin_seq.h"
#include "machines.h"


#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00


#define UNPARSEDACTION "unparsed"

#define FOREVER while (1)

#define MAX_EXECUTION_MSGS  56
typedef struct {
    hub_error_t resultToPost; //Success if everything up to now is successful
    char actionPerformed[8];  //action name to post
    uint8_t resource;         //Resource responsible for action
    msgType type;             //Event, action, result
    actionStatus_t msg_status;//Success, pending
    bool sent;                //will turn TRUE when sent
} execution_message_info_t;

/*** VARIABLES **************************************************************************/
static const char *TAG = "[EXECUTE]";

SemaphoreHandle_t ex_post_sph = NULL;
SemaphoreHandle_t ex_loopback_sph = NULL;

/*Execution msg service *************/
/*execution_message_info_t Buffer that holds the messages information*/
execution_message_info_t execution_message_buffer[MAX_EXECUTION_MSGS];

/*execution_message_index Points to the latest message pushed in buffer space*/
static uint8_t execution_message_index = 0;
/*execution_message_sent Points to the latest message sent in buffer space*/
static uint8_t execution_message_sent = 0;
/*message_index_overflow shows the amount of time the buffer has overflown*/
static uint8_t message_index_overflow = 0;
/*** DECLARATIONS ***********************************************************************/
/***
 *         _ _                 _       _
 *        | (_)               | |     | |
 *      __| |_ ___ _ __   __ _| |_ ___| |__   ___ _ __
 *     / _` | / __| '_ \ / _` | __/ __| '_ \ / _ \ '__|
 *    | (_| | \__ \ |_) | (_| | || (__| | | |  __/ |
 *     \__,_|_|___/ .__/ \__,_|\__\___|_| |_|\___|_|
 *                | |
 *                |_|
 */
 /*<! dispatch_sequence_conditions CHECKS for the conditions of the detected finishing sequence in main loop*/
 static bool dispatch_sequence_conditions(machines_machine_t *machine_p, machines_sequence_t *sequence_p);
/*execution_perform_cb is the callback called by msg_fmt when an execution msg is received*/
static void execution_perform_cb(void);
/*<! dispatch_received_action_conditions CHECKS for the validity of the
 conditions of the received sequence. If everything is valid, here the sequence
 will be pushed to the machine*/
static hub_error_t dispatch_received_action_conditions(machines_machine_t *machine_p,
        machines_sequence_t *sequence_p);


 /***
  *                                            _
  *                                           | |
  *     _ __ ___  ___  __ _    ___ _   _ _ __ | |_ __ ___  __
  *    | '_ ` _ \/ __|/ _` |  / __| | | | '_ \| __/ _` \ \/ /
  *    | | | | | \__ \ (_| |  \__ \ |_| | | | | || (_| |>  <
  *    |_| |_| |_|___/\__, |  |___/\__, |_| |_|\__\__,_/_/\_\
  *                    __/ |        __/ |
  *                   |___/        |___/
  */
  /*execution_resource returns the resource found in the message*/
static uint8_t execution_resource(cJSON *object);
  /*execution_post_result pushes a message to the msg_fmt buffer. returns success is message was successfully pushed*/
static uint8_t execution_post_result(hub_error_t resultToPost, char *actionPerformed, uint8_t resource, msgType type,actionStatus_t action_status);
/*execution_syntax_check Check if the minimum required fields are present in the message*/
hub_error_t execution_syntax_check(cJSON * object);
/*execution_q_message Pushes a message to the local queue msg*/
static void execution_q_message(hub_error_t resultToPost, char *actionPerformed, uint8_t resource, msgType type, actionStatus_t action_status);
/*checkIfValid Looks for a specific key in an object*/
static bool checkIfValid(const cJSON *jsonObject, char *itemToFind);

  /***
   *     _____                    _   _
   *    |  ___|                  | | (_)
   *    | |____  _____  ___ _   _| |_ _  ___  _ __
   *    |  __\ \/ / _ \/ __| | | | __| |/ _ \| '_ \
   *    | |___>  <  __/ (__| |_| | |_| | (_) | | | |
   *    \____/_/\_\___|\___|\__,_|\__|_|\___/|_| |_|
   *
   *
   */

/*execution_perform Will dispatch all the information of the execution command, until
execution happens on the dispatch_received_action_conditions*/
static hub_error_t execution_perform(cJSON *root, uint16_t resource);
/*Main task of the execution, harvest sequencies from the sequence buffer*/
static void execution_update(void);
/*COnfussion callback. I really dont understand why this funcion exists*/
static void execution_action(cJSON *object);


/***
 *     _                             _                _
 *    | |                           | |              | |
 *    | |     ___   ___  _ __ ______| |__   __ _  ___| | __
 *    | |    / _ \ / _ \| '_ \______| '_ \ / _` |/ __| |/ /
 *    | |___| (_) | (_) | |_) |     | |_) | (_| | (__|   <
 *    \_____/\___/ \___/| .__/      |_.__/ \__,_|\___|_|\_\
 *                      | |
 *                      |_|
 */

/*This function dispatches the finished action. For now, it just calls
the function that handles the communication layer of this service*/
static void execution_loopback(pin_seq_sequence_t event);

/*This function will work with the internal machine FSM to handle
standard cycles without the cloud infrastracture*/
static bool execution_loopback_offline(pin_seq_sequence_t event);
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
static void task_run_offline(void);
static bool task_isConnected(void);
static void task_engagement_done(void);
static void task_disengagement_done(void);
static void task_sleep(uint8_t ticks);

/*** DEFINITIONS ************************************************************************/

void execution_task(void *pvParameters) {
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
        task_sleep(1);}
}

static void task_engagement_done(void) {
    fallback_or_running = hub_running_mode;
    engage_activation_flags[execute_engage_flag] = 0;
}

static void task_disengagement_done(void) {
    disengage_activation_flags[execute_disengage_flag] = 0;
}

static bool task_isConnected(void) {
    return hub_isConnected;
    //return FALSE;
}

static void task_run_offline(void) {
    execution_update();
    //ESP_LOGI(TAG, "Running ofline()");
}

static bool wait_for_deployment(void) {
    if (!is_hardware_version_selected) {
        ESP_LOGW(TAG, "Not loading because hardware version not selected");
        return FALSE;
    }
    for (uint8_t k = 0; (k < 60) && !machines_loaded_get(); k++) {
        ESP_LOGI(TAG, "Machines still unloaded");
        task_sleep(1);
    }
    if (machines_loaded_get()) {
        machines_prepare();
        ESP_LOGI(TAG, "Machines prepared!");
        return TRUE;
    } else {
        ESP_LOGE(TAG, "Machines unprepared!");
        return FALSE;
    }
}

static bool task_engage(void) {
    wait_for_deployment();
    run_confirmation_flag[execution_current] = 1;
    return true;
}

static void task_run(void) {
    execution_update();
}

static bool task_disengage(void) {
    queues_flush();
    run_confirmation_flag[execution_current] = 0;
    return true;
}

static bool task_isRun(void) {
    if (run_activation_flags[execute_run_flag]) {
        if (is_hardware_version_selected) {
            return TRUE;
        } else {
            ESP_LOGE(TAG, "Hardware version not selected");
            task_sleep(5);
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

static void task_init(void) {
    ESP_LOGI(TAG, "execution_init()");
    sph_create(&ex_post_sph);
    sph_create(&ex_loopback_sph);

    if (ex_post_sph == NULL) {
        ESP_LOGE(TAG, "Could not create Semaphore");
    } else {
        ESP_LOGI(TAG, "SUccessfuly created semaphore");
    }

    msg_fmt_install(Execution, execution_perform_cb);


    ESP_LOGI(TAG, "~execution_init()");
}

static bool task_isEngage(void) {
    return engage_activation_flags[execute_engage_flag];
}

static bool task_isDisengage(void) {
    return disengage_activation_flags[execute_disengage_flag];
}

static void task_idle(void) {
    ESP_LOGI(TAG, "idle()");
    task_sleep(5);
}

static void task_sleep(uint8_t ticks) {
    vTaskDelay(suspendedThreadDelay[execution_flag] * ticks / portTICK_RATE_MS);
}

static void execution_update(void) {
    pin_seq_sequence_t event;
    while (pin_seq_next(&event) == SUCCESS) {
        set_ram_usage(uxTaskGetStackHighWaterMark(NULL), execution_task_idx);
        ESP_LOGI(TAG, "Detected NEW sequence finished [%s]", event.sequence_p->name);
        if (event.sequence_p->channel == Event) {
            machine_single_event_prepare(event.machine_p, event.sequence_p);
        }/*<! Re-push events so that they are detected again on the future
        */
        if (dispatch_sequence_conditions(event.machine_p, event.sequence_p) == IGNORE) {
            ESP_LOGW(TAG, "[%s] ignored on update for conditions", event.sequence_p->name);
            continue;
        }

        ESP_LOGI(TAG, "Successfuly executed [%s] on [%d]", event.sequence_p->name,
                event.machine_p->deployment_info.resource);
        if (task_isConnected()) {
            execution_loopback(event);
        } else {
            if(execution_loopback_offline(event)){
              data_to_report[recovery_data] = true;
              ESP_LOGI(TAG,"Recovery Activated");
              machine_need_recover_set(event.machine_p);
            }
        }

    }
#ifdef EXECUTION_FAILURE_EXISTS
    if (0) { /*When execution failure is implemented*/
        ESP_LOGE(TAG, "FAILURE Executing [%s] on [%d]", event.sequence_p->name, event.machine_p->resource);
        execution_post_result(failedExecution,
                event.sequence_p->name,
                event.machine_p->resource,
                (msgType) event.sequence_p->channel);
        machines_machine_disable(event.machine_p);
    }
#endif
}

static void execution_perform_cb(void) {
    cJSON *message = cJSON_Parse(msg_fmt_message_static(Execution));
    uint8_t resource = FAILURE;
    hub_error_t msg_parse_result = execution_syntax_check(message);

    if (msg_parse_result != HUB_OK) {
        /*TRASH*/
        execution_q_message(msg_parse_result, "cycle", 0, 0,0);
        stash_jSON_msg(message);
        return;
    }
    if (!_check_jSONField(message, "action")) {
        execution_action(message);
    }
    if (!_check_jSONField(message, "resource")) {
        resource = execution_resource(message);
    }
    if(!task_isRun()){
      ESP_LOGE(TAG,"Task not running");
      execution_q_message(msg_parse_result, "cycle", 0, 0,0);
      stash_jSON_msg(message);
      return;
    }
    //ESP_LOGE(TAG,"Perform callback loaded @[%d]",get_hub_timestamp());
    msg_parse_result = execution_perform(message, resource);
    if (msg_parse_result != HUB_OK) {
        execution_q_message(msg_parse_result, "cycle", 0, 0,0);
    }
    stash_jSON_msg(message);
    //ESP_LOGE(TAG,"Perform finished @[%d]",get_hub_timestamp());
    ESP_LOGI(TAG, "~Execution()");
}

static hub_error_t execution_perform(cJSON *root, uint16_t resource) {
    const cJSON *actionHolder = NULL;
    char *ActionName = NULL;
    msgResults resultToPost = 0;
    uint8_t target, sequence = 0;
    machines_machine_t *machine_p;
    machines_sequence_t *sequence_p;

    if (!root) {
        ESP_LOGE(TAG, "No valid action");
        return HUB_ERROR_EXEC_GEN;
    }
    actionHolder = cJSON_GetObjectItemCaseSensitive(root, "action");

    if (!resultToPost && !checkIfValid(actionHolder, "action")) {
        ESP_LOGE(TAG, "no action in json");
        ActionName = UNPARSEDACTION;
        return HUB_ERROR_EXEC_GEN;
    } else {
        ActionName = actionHolder->valuestring;
        ESP_LOGI(TAG, "Looking for %s in machine %d", ActionName, resource);
    }
    if (!resultToPost && (machines_machine(&machine_p, resource) == FAILURE)) {
        ESP_LOGE(TAG, "received action for unexistant machine\r\n");
        return HUB_ERROR_EXEC_GEN;
    }
/*******************************************************************************************************/
/*NOTE: This is a MISTAKE. This piece of code is ONLY WORKING because inside the MACHINES TARGET function
we check RESOURCE actions first and RETROFIT actions after.*/
    target = machines_target(machine_p, ActionName);
/*******************************************************************************************************/
    sequence = machines_sequence(machine_p, target, Action, ActionName);

    if (!resultToPost && sequence == FAILURE) {
        ESP_LOGE(TAG, "No action %d in machine %d", sequence, resource);
        return HUB_ERROR_EXEC_GEN;
    }

    sequence_p = &(machine_p->deployment_info.sequences[target][Action][sequence]);
    return dispatch_received_action_conditions(machine_p, sequence_p);
}

static bool dispatch_sequence_conditions(machines_machine_t *machine_p, machines_sequence_t *sequence_p) {
    if (machine_flags_dispatcher(machine_p,
            sequence_p) == IGNORE) {
        ESP_LOGW(TAG,"Sequence [%s] ignored",sequence_p->name);
        return IGNORE;
    }
    ESP_LOGW(TAG,"Sequence [%s] acceppted",sequence_p->name);
    machine_control_task_dispatcher(machine_p, sequence_p, !task_isConnected());
    return DONT_IGNORE;
}

static hub_error_t dispatch_received_action_conditions(machines_machine_t *machine_p,
        machines_sequence_t *sequence_p) {
    if (machine_flags_dispatcher(machine_p, sequence_p) == IGNORE) {
        ESP_LOGW(TAG,"Action not allowed");
        return HUB_ERROR_EXEC_GEN;
    }
    if (update_machine_flags(machine_p, sequence_p, !task_isConnected()) != SUCCESS) {
        ESP_LOGE(TAG, "COULD NOT update status\r\n");
        return HUB_ERROR_EXEC_GEN;
    }
    pin_seq_push(machine_p, sequence_p);
    ESP_LOGI(TAG, "QUEUEING ACTION %s FOR MACHINE %d ()", sequence_p->name,
            machine_p->deployment_info.resource);
    return HUB_OK;
}

static actionStatus_t dispatch_msg_status(char * _status){
    actionStatus_t result = 0;
    if(!strcmp(action_status[runningStatus],_status)){
        return runningStatus;
    } else if(!strcmp(action_status[successStatus],_status)){
        return successStatus;
    } else if(!strcmp(action_status[triggerStatus],_status)){
        return triggerStatus;
    }
    return result;
}

static void execution_q_message(hub_error_t resultToPost, char *actionPerformed, uint8_t resource, msgType type, actionStatus_t action_status) {
    if (sph_step_retries(&ex_post_sph) == pdTRUE) {
        char * action_p = execution_message_buffer[execution_message_index].actionPerformed;
        strcpy(action_p, actionPerformed);

        execution_message_buffer[execution_message_index].resource
                = resource;
        execution_message_buffer[execution_message_index].resultToPost =
                resultToPost;
        execution_message_buffer[execution_message_index].sent = FALSE;
        execution_message_buffer[execution_message_index].type = (msgType) type;
        execution_message_buffer[execution_message_index].msg_status = action_status;
        ESP_LOGI(TAG, "[Queing msg], Idx [%d]",execution_message_index);

        if (execution_message_index >= (MAX_EXECUTION_MSGS-1)) {
            execution_message_index = 0;
            message_index_overflow++;
            ESP_LOGW(TAG,"Overflowing Execution Queue [%d]",message_index_overflow);
        }
        execution_message_index++;
        sph_give(&ex_post_sph);
        ESP_LOGI(TAG, "[Queing msg]");
    } else {
        ESP_LOGE(TAG, "[Q MSG] Lock untaken");
    }
}

static void execution_deq_message() {
    if (sph_step_retries(&ex_post_sph) == pdTRUE) {
        if ((execution_message_sent < execution_message_index) ||
                (message_index_overflow > 0)) {
            set_ram_usage(uxTaskGetStackHighWaterMark(NULL), execution_msg_task);
            if(execution_post_result(
                    execution_message_buffer[execution_message_sent].resultToPost,
                    execution_message_buffer[execution_message_sent].actionPerformed,
                    execution_message_buffer[execution_message_sent].resource,
                    execution_message_buffer[execution_message_sent].type,
                    execution_message_buffer[execution_message_sent].msg_status
                  ) != SUCCESS){
                    ESP_LOGW(TAG,"Retrying later");
                    sph_give(&ex_post_sph);
                    return;
                  }
            execution_message_buffer[execution_message_sent].sent = TRUE;
            ESP_LOGI(TAG, "[DeQueing msg], Idx [%d] / [%d]",execution_message_sent,execution_message_index);
            if (execution_message_sent >= (MAX_EXECUTION_MSGS-1)) {
                execution_message_sent = 0;
                message_index_overflow--;
                ESP_LOGW(TAG,"DEOverflowing Execution Queue [%d]",message_index_overflow);
            }
            execution_message_sent++;
            ESP_LOGI(TAG, "[Dequeing message]");
        }
        sph_give(&ex_post_sph);
    } else {
        ESP_LOGE(TAG, "[DEQ MSG] Lock untaken");
    }
}

void execution_msg_q(void *pvParameters) {
    FOREVER{
      if(task_isRun()){
        execution_deq_message();
        vTaskDelay(100 / portTICK_RATE_MS);
      } else {
        vTaskDelay(1000 / portTICK_RATE_MS);
      }
    }
}

static uint8_t execution_post_result(hub_error_t resultToPost, char *actionPerformed, uint8_t resource, msgType type,actionStatus_t action_satus) {
    set_ram_usage(uxTaskGetStackHighWaterMark(NULL), execution_task_idx);
    ESP_LOGI(TAG, "Sending [%s] for [%d], status [%d]", actionPerformed,resource, resultToPost);
    if(msg_fmt_load(Execution) != SUCCESS){
      ESP_LOGI(TAG,"Topic still sending message...");
      return FAILURE;
    }
    msg_fmt_edit_str(Execution, (char *) msg_fields[typeField], (char *) message_type[type]);
    msg_fmt_edit_str(Execution, (char *) msg_fields[actionField], (char *) actionPerformed);
    msg_fmt_edit_str(Execution, (char *) msg_fields[hubIdField], (char *) params_get(HUBID_PARAM));
    switch (resultToPost) {
        case HUB_OK:
            msg_fmt_edit_int(Execution, (char *) msg_fields[resourceField], resource);
            msg_fmt_edit_str(Execution, (char *) msg_fields[statusField], (char *) action_status[action_satus]);
            break;
        default:
            msg_fmt_edit_int(Execution, (char *) msg_fields[resourceField], -1);
            msg_fmt_edit_str(Execution, (char *) msg_fields[statusField], (char *) action_status[failedExecution]);
            msg_fmt_append_error(Execution, resultToPost);
            break;
    }
    msg_fmt_send(Execution);
    return SUCCESS;
}

static void execution_loopback(pin_seq_sequence_t event) {
    actionStatus_t status = dispatch_msg_status(event.sequence_p->status);

    //stop sending STARTUP messages

    execution_q_message(HUB_OK, event.sequence_p->name, event.machine_p->deployment_info.resource,
            (msgType) event.sequence_p->channel, status);
}

void trigger_test(void) {
    machines_machine_t *target_machine;
    machines_machine(&target_machine, 4);
    if (target_machine == NULL) {
        ESP_LOGE(TAG, "Incorrect test setup");
        return;
    }
    msg_fmt_loopback_execution_prepare(target_machine->deployment_info.resource, "allow");
    msg_fmt_loopback_execution();
    task_sleep(1);
    msg_fmt_loopback_execution_prepare(target_machine->deployment_info.resource, "testr");
    msg_fmt_loopback_execution();
}

static void offline_loopback_loopback(machines_machine_t *machine_p) {
    if (sph_step_retries(&ex_loopback_sph) == pdTRUE) {
        char * action_name = NULL;
        machine_flags_get(machine_p,CURRENT_ACTION_FLAG,&action_name);
        msg_fmt_loopback_execution_prepare(machine_p->deployment_info.resource,action_name);
        msg_fmt_loopback_execution();
        sph_give(&ex_loopback_sph);
    } else {
        ESP_LOGE(TAG, "[loopback-loopback] lock untaken");
    }
}

static void offline_loopback_reallow(machines_machine_t *machine_p) {
    if (sph_step_retries(&ex_loopback_sph) == pdTRUE) {
        msg_fmt_loopback_execution_prepare(machine_p->deployment_info.resource, MACHINE_ENABLE_ACTION);
        msg_fmt_loopback_execution();
        sph_give(&ex_loopback_sph);
    } else {
        ESP_LOGE(TAG, "[loopback-loopback] lock untaken");
    }
}

static void offline_loopback_deny(machines_machine_t *machine_p) {
    if (sph_step_retries(&ex_loopback_sph) == pdTRUE) {
        msg_fmt_loopback_execution_prepare(machine_p->deployment_info.resource, MACHINE_DISABLE_ACTION);
        msg_fmt_loopback_execution();
        sph_give(&ex_loopback_sph);
    } else {
        ESP_LOGE(TAG, "[loopback-loopback] lock untaken");
    }
}

static bool offline_loopback_dispatch_action(machines_machine_t *machine_p) {
    char * machine_action;
    machine_flags_get(machine_p, CURRENT_ACTION_FLAG, &machine_action);
    ESP_LOGI(TAG, "received [DENY]. continuing");
    if (!strcmp(machine_action, DEFAULT_MACHINE_ACTION)) {
        ESP_LOGW(TAG, "No action for next step. Aborting");
        return false;
    } else {
        ESP_LOGI(TAG, "Performing [%s]", machine_action);
        offline_loopback_loopback(machine_p);
        return true;
    }
}

static bool execution_loopback_offline(pin_seq_sequence_t event) {
  char * machine_action;
  machine_flags_get(event.machine_p, CURRENT_ACTION_FLAG, &machine_action);
  if (event.sequence_p->channel == Action) {
    if (!strcmp(event.sequence_p->name, MACHINE_DISABLE_ACTION)) {
      ESP_LOGI(TAG, "[LOOP-BACK] now performing [%s] after DENY", machine_action);
      return offline_loopback_dispatch_action(event.machine_p); /*If Deny, push action*/
    }
  } else {
    if (!strcmp(event.sequence_p->status, execution_status[triggerStatus])) { /*If trigger, push DENY*/
      ESP_LOGI(TAG, "[LOOP-BACK] pushing DENY after [%s]", event.sequence_p->name);
      offline_loopback_deny(event.machine_p);
      return true;
    }
    else if (!strcmp(event.sequence_p->name, machine_action)) { /*NOT checking if status SUCCESS, if Event with action==machine action, reallow*/
      offline_loopback_reallow(event.machine_p);
      ESP_LOGI(TAG, "[LOOP-BACK] reallowing after [%s]", event.sequence_p->name);
      return true;
    }
  }
  return false;
}



static bool checkIfValid(const cJSON *jsonObject, char *itemToFind) {
    if (jsonObject == NULL) {
#ifdef __TESTING_MSG_DEPLOY__
        ESP_LOGE(TAG, "No device %s, aborting", itemToFind);
#endif
        return 0;
    } else {
        return 1;
    }
}

uint8_t _test_performAction(cJSON *root, uint16_t resource) {
    return execution_perform(root, resource);
}

void TEST_execution_cb(void) {
    execution_perform_cb();
}

static void execution_action(cJSON *object) {
    char * name = NULL;
    name = _get_jSONField(object, "action")->valuestring;
    /*DEBUG*/ESP_LOGI(TAG, "received action [%s]", name);
}

static uint8_t execution_resource(cJSON *object) {
    uint8_t resource = 0;
    resource = _get_jSONField(object, "resource")->valueint;
    /*DEBUG*/ESP_LOGI(TAG, "received from resource [%d]", resource);
    return resource;
}


hub_error_t execution_syntax_check(cJSON * object) {
    if (_check_jSONField(object,
            msg_fields[resourceField]) != HUB_OK) {
        return HUB_ERROR_EXEC_UNEXISTANT_RESORCE;
    }

    if (_check_jSONField(object,
            msg_fields[actionField]) != HUB_OK) {
        return HUB_ERROR_EXEC_UNEXISTANT_ACTION;
    }

    if (check_ifString(object,
            msg_fields[actionField]) != HUB_OK) {
        return HUB_ERROR_EXEC_UNEXISTANT_ACTION;
    }

    if (check_ifString(object,
            msg_fields[resourceField]) == HUB_OK) {
        return HUB_ERROR_COMM_MSG_NOJSON;
    }

    ESP_LOGI(TAG, "Execution Syntax check OK");
    return HUB_OK;
}
