//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ______ ____   __    ____ __  __ __  ___ ______ _   __ ______              //
//    / __ \ / ____// __ \ / /   / __ \\ \/ //  |/  // ____// | / //_  __/              //
//   / / / // __/  / /_/ // /   / / / / \  // /|_/ // __/  /  |/ /  / /                 //
//  / /_/ // /___ / ____// /___/ /_/ /  / // /  / // /___ / /|  /  / /                  //
// /_____//_____//_/    /_____/\____/  /_//_/  /_//_____//_/ |_/  /_/                   //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "deployment.h"

#include "stdio.h"
#include "string.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "cJSON.h"

#include "params.h"
#include "messages.h"
#include "machines.h"
#include "pin_seq.h"
#include "msg_fmt.h"
#include "lavigoMasterFSM.h"
#include "registration.h"
#include "hub_error.h"

#define FOREVER while(1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define _ACTION_INIT  "init"
#define _ACTION_FIX   "fix"
#define _ACTION_CLEAR "clear"
/*** VARIABLES **************************************************************************/

static const char *TAG = "[DEPLOY]";

/*** DECLARATIONS ***********************************************************************/
static void _action_cb();
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

/*get_received_action dispatches the received message within
the deployment service. Receives the ACTION field string and Returns
the received action in terms of DEPLOYMENT ACTION TYPE enum*/
deployment_msg_actions get_received_action(char * received_action);

/*deployment_init_cb transforms cJSON * message into a resource saved in memory
and ready to be fixed*/
static hub_error_t deployment_init_cb(cJSON * message);

/*deployment_fix_cb sets all the resource flags into default value*/
static hub_error_t deployment_fix_cb(cJSON * message);

/*deployment_clear_cb Deletes the machine indicated in the message*/
static hub_error_t deployment_clear_cb(cJSON * message);

/*get_resource_tofix Looks up the resource to fix and returns its pointer
in the resource array*/
machines_machine_t* get_resource_tofix(cJSON *resource_p);

/*deployment_resource_fix transforms the msg to the machine NAME*/
uint8_t deployment_resource_fix(cJSON *resource_p);

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

 /*Looks for "resource" and "action field". Returns success if found*/
hub_error_t deployment_syntax_check(cJSON * object);

/*Looks for all necessary objects in resource template. Success if found*/
hub_error_t template_syntax_check(cJSON * object);

/*Looks for the resource name to be in range. SUCCESS if it is*/
hub_error_t deployment_content_check(cJSON * object);

/*queues a deployment message on MSG FORMAT*/
static void deployment_post_result(hub_error_t resultToPost, deployment_msg_actions action_to_post, uint8_t resource);

/***
 *         _            _                                  _      _       _ _
 *        | |          | |                                | |    (_)     (_) |
 *      __| | ___ _ __ | | ___  _   _ _ __ ___   ___ _ __ | |_    _ _ __  _| |_
 *     / _` |/ _ \ '_ \| |/ _ \| | | | '_ ` _ \ / _ \ '_ \| __|  | | '_ \| | __|
 *    | (_| |  __/ |_) | | (_) | |_| | | | | | |  __/ | | | |_   | | | | | | |_
 *     \__,_|\___| .__/|_|\___/ \__, |_| |_| |_|\___|_| |_|\__|  |_|_| |_|_|\__|
 *               | |             __/ |
 *               |_|            |___/
 */

/*Sets the new deplot=yed resource name - checks if its available*/
uint8_t deployment_resource(machines_machine_t *machine_p, cJSON *resource_p);

/*Sets the new deplot=yed resource signals and ports - checks if available*/
uint8_t deployment_signals(machines_machine_t *machine_p, cJSON *ports_p, cJSON *signals_p);

/*Sets the new deplot=yed resource Sequencies*/
uint8_t deployment_sequences(machines_machine_t *machine_p, cJSON *sequences_p, machines_channel_t channel);

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

uint8_t TEST_deployment_init_cb(void) {
    cJSON *root = cJSON_Parse(msg_fmt_message_static(Deployment));
    uint8_t test_result = deployment_init_cb(root);
    stash_jSON_msg(root);
    return test_result;
}

uint8_t TEST_deployment_fix_cb(void) {
    cJSON *root = cJSON_Parse(msg_fmt_message_static(Deployment));
    uint8_t test_result = deployment_fix_cb(root);
    stash_jSON_msg(root);
    return test_result;
}

uint8_t TEST_deployment_clear_cb(void) {
    cJSON *root = cJSON_Parse(msg_fmt_message_static(Deployment));
    uint8_t test_result = deployment_clear_cb(root);
    stash_jSON_msg(root);
    return test_result;
}

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
 /*Main task administration*/
void deployment_task(void *pvParameters) {
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
        task_sleep(1);
      }
}

static void task_engagement_done(void) {
    set_ram_usage(uxTaskGetStackHighWaterMark(NULL), deployment_task_idx);
    engage_activation_flags[deploy_engage_flag] = 0;
}

static void task_disengagement_done(void) {
    disengage_activation_flags[deploy_disengage_flag] = 0;
}

static bool task_isConnected(void) {
    return hub_isConnected;
}

static void task_run_offline(void) {
    //ESP_LOGI(TAG, "Running offline()");
}

/*Set the MQTT callback to execute on received message*/
static void task_init(void) {
    msg_fmt_install(Deployment, _action_cb);
}

static bool task_engage(void) {
/*Check if hardware version was logged before starting*/
    if (!is_hardware_version_selected) {
        ESP_LOGE(TAG, "Hardware version undefined!");

    } else {
/*Load ALL resources installed in flash to operate.
NOTE that execution will be waiting till this is over to
install the event sequencies on the queues*/
        machines_load();
    }
    set_ram_usage(uxTaskGetStackHighWaterMark(NULL), deployment_task_idx);
    run_confirmation_flag[deployment_current] = 1;
    return TRUE;
}

static void task_run(void) {
    set_ram_usage(uxTaskGetStackHighWaterMark(NULL), deployment_task_idx);
}

static bool task_disengage(void) {
    run_confirmation_flag[deployment_current] = 0;
    return TRUE;
}

static bool task_isRun(void) {
    return run_activation_flags[deploy_run_flag];
}

static bool task_isEngage(void) {
    return engage_activation_flags[deploy_engage_flag];
}

static bool task_isDisengage(void) {
    return disengage_activation_flags[deploy_disengage_flag];
}

static void task_idle(void) {
    ESP_LOGI(TAG, "idle()");
    task_sleep(5);
}

static void task_sleep(uint8_t ticks) {
    vTaskDelay(suspendedThreadDelay[deployment_flag] * ticks / portTICK_RATE_MS);
}

void deployment_machine_4_test(void) {
    ESP_LOGE(TAG,"Deprecated");
}

static void deployment_post_result(hub_error_t deployment_action_result, deployment_msg_actions action_to_post, uint8_t resource) {
    deploymentResult resultToPost = failDeploy;
    msg_fmt_load(Deployment);
    msg_fmt_edit_str(Deployment, (char *) msg_fields[actionField], (char *) deploymentActions[action_to_post]);
    msg_fmt_edit_str(Deployment, (char *) msg_fields[hubIdField], (char *) params_get(HUBID_PARAM));
    msg_fmt_edit_str(Deployment, (char *) msg_fields[typeField], (char *) message_type[type_result]);
    msg_fmt_edit_int(Deployment, (char *) msg_fields[resourceField], resource);

    switch (deployment_action_result) {
        case HUB_OK:
            resultToPost = successDeploy;
            break;
        default:
            resultToPost = failDeploy;
            msg_fmt_append_error(Deployment, deployment_action_result);
            break;
    }
    msg_fmt_edit_str(Deployment, (char *) msg_fields[statusField], (char *) deployment_status[resultToPost]);
    msg_fmt_send(Deployment);
}

deployment_msg_actions get_received_action(char * received_action) {
    ESP_LOGI(TAG, "Received action [%s]", received_action);

    deployment_msg_actions received_type = deployment_msg_init;

    if (!strcmp(received_action,
            deploymentActions[deployment_msg_init])) {
        received_type = deployment_msg_init;
    } else if (!strcmp(received_action,
            deploymentActions[deployment_msg_fix])) {
        received_type = deployment_msg_fix;
    } else if (!strcmp(received_action,
            deploymentActions[deployment_msg_clear])) {
        received_type = deployment_msg_clear;
    }
    ESP_LOGI(TAG, "Received action [%d]", received_type);
    return received_type;
}


static void _action_cb(void) {
    hub_error_t deployment_action_result = HUB_ERROR_DEPL_GEN;
    deployment_msg_actions received_action = deployment_msg_init;
    uint8_t resource_id = 0xFF;
    cJSON *root = cJSON_Parse(msg_fmt_message_static(Deployment));

    if (deployment_syntax_check(root) != HUB_OK) {
        deployment_post_result(HUB_ERROR_DEPL_GEN, deployment_msg_clear, resource_id);
        stash_jSON_msg(root);
        return;
    }

    if (deployment_content_check(root) != HUB_OK) {
        deployment_post_result(HUB_ERROR_DEPL_GEN, deployment_msg_clear, resource_id);
        stash_jSON_msg(root);
        return;
    }

    received_action = get_received_action(_get_jSONField(root,
            msg_fields[actionField])->valuestring
            );
    resource_id = _get_jSONField(root,
            msg_fields[resourceField])->valuedouble;

    switch (received_action) {
        case deployment_msg_init:
            deployment_action_result = deployment_init_cb(root);
            break;
        case deployment_msg_fix:
            deployment_action_result = deployment_fix_cb(root);
            break;
        case deployment_msg_clear:
            deployment_action_result = deployment_clear_cb(root);
            break;
        default:
            deployment_post_result(deployment_action_result, received_action, resource_id);
    }
    deployment_post_result(deployment_action_result, received_action, resource_id);
    stash_jSON_msg(root);
}



static hub_error_t deployment_init_cb(cJSON * message) {
    if (!is_hardware_version_selected) {
        ESP_LOGE(TAG, "undefined hardware version, cant deploy!");
        return HUB_ERROR_DEPL_UNDEFINED_VERSION;
    }

    if (_check_jSONField(message, "resource")) {
        return HUB_ERROR_DEPL_GEN;
    }
    if (_check_jSONField(message, "template")) {
        return HUB_ERROR_DEPL_GEN;
    }

    cJSON *template = cJSON_GetObjectItem(message, "template");

    if (_check_jSONField(template, "ports")) {

        return HUB_ERROR_DEPL_GEN;
    }
    if (_check_jSONField(template, "signals")) {

        return HUB_ERROR_DEPL_GEN;
    }
    if (_check_jSONField(template, "actions")) {

        return HUB_ERROR_DEPL_GEN;
    }
    if (_check_jSONField(template, "events")) {

        return HUB_ERROR_DEPL_GEN;
    }

    machines_machine_t *machine_p;

    machines_make(&machine_p);

    if (machine_p==NULL) {
        return HUB_ERROR_DEPL_GEN;
    }


    if (deployment_resource(machine_p, _get_jSONField(message, "resource")) == FAILURE) {
        machines_free(&machine_p);
        return HUB_ERROR_DEPL_GEN;
    };

    if (deployment_signals(machine_p, _get_jSONField(template, "ports"), _get_jSONField(template, "signals"))
            == FAILURE) {
        machines_free(&machine_p);
        return HUB_ERROR_DEPL_GEN;
    };

    if (deployment_sequences(machine_p, _get_jSONField(template, "actions"), Action)
            == FAILURE) {
        machines_free(&machine_p);
        return HUB_ERROR_DEPL_GEN;
    };

    if (deployment_sequences(machine_p, _get_jSONField(template, "events"), Event)
            == FAILURE) {
        machines_free(&machine_p);
        return HUB_ERROR_DEPL_GEN;
    };
    machines_hab_t set_disabled_state = machine_disabled;
    machine_flags_set(machine_p, ENABLED_FLAG, &set_disabled_state);
    machines_idle_t set_idle_state = is_idle;
    machine_flags_set(machine_p, IDLE_FLAG, &set_idle_state);
    machines_control_t set_no_control = control_none;
    machine_flags_set(machine_p, CONTROL_FLAG, &set_no_control);
    machines_new_sequence_t set_same_sequence = same_sequence;
    machine_flags_set(machine_p, SAME_ACTION_FLAG, &set_same_sequence);
    machine_flags_clear_action(machine_p);
    machine_update(machine_p);

    ESP_LOGI(TAG, "<SAVING MACHINE [%d] @ [%d]>", machine_p->deployment_info.resource,get_hub_timestamp());
    if (machines_save(machine_p) != SUCCESS) {
        machines_free(&machine_p);
        return HUB_ERROR_DEPL_GEN;
    }
    if (run_activation_flags[execute_run_flag]) {
        if (is_hardware_version_selected) {
            machine_single_prepare(machine_p);
        } else {
            ESP_LOGW(TAG, "Hardware version unselected");
        }
    }
    ESP_LOGI(TAG, "</SAVING MACHINE [%d] @ [%d]>", machine_p->deployment_info.resource,get_hub_timestamp());
    return HUB_OK;
}

uint8_t deployment_resource(machines_machine_t *machine_p, cJSON *resource_p) {
    machines_machine_t *_machine_p;
    if (machines_machine(&_machine_p, resource_p->valuedouble) == SUCCESS) {
        ESP_LOGE(TAG, "[init] (400) Resource name already exists!");
        return FAILURE;
    }

    machine_p->deployment_info.resource = resource_p->valuedouble;

    return SUCCESS;
}

uint8_t deployment_signals(machines_machine_t *machine_p, cJSON *ports_p, cJSON *signals_p) {
    char *letters[MACHINES_MAX_PORTS]; // Row
    char *colors [MACHINES_MAX_PORTS]; // Column

    uint8_t port_idx = 0;
    cJSON *port_item = ports_p->child;

    while (port_item != NULL) { //TODO: assert nof ports
        if (_check_jSONField(port_item, "letter")) {
            ESP_LOGE(TAG, "[init] (400) Letter indicator is missing!");
            return FAILURE;
        }

        if (check_ifString(port_item, "letter") != HUB_OK) {
            return FAILURE;
        }

        if (_check_jSONField(port_item, "color")) {
            ESP_LOGE(TAG, "[init] (400) Column indicator is missing!");
            return FAILURE;
        }

        if (check_ifString(port_item, "color") != HUB_OK) {
            return FAILURE;
        }

        letters[port_idx] = _get_jSONField(port_item, "letter")->valuestring; //TODO: assert letter

        if (check_field_content(letters[port_idx],
                (char **) machines_rows, COLUMN_MAX) != HUB_OK) {
            return FAILURE;
        }

        colors[port_idx] = _get_jSONField(port_item, "color")->valuestring; //TODO: assert color

        if (check_field_content(colors[port_idx],
                (char **) machines_columns, COLUMN_MAX) != HUB_OK) {
            return FAILURE;
        }

        port_idx++;
        port_item = port_item->next;
    }

    uint8_t amount_of_signals = cJSON_GetArraySize(signals_p);
    if (amount_of_signals > 2) {
        ESP_LOGE(TAG, "(400) Init cannot handle [%d] signals", amount_of_signals);
        return FAILURE;
    }

    char name;
    uint8_t port = 0, dir = 1;

    uint8_t signal_idx = 0;
    cJSON *signal_item = signals_p->child;

    while (signal_item != NULL) { //TODO: assert nof signals
        if (_check_jSONField(signal_item, "name")) {
            ESP_LOGE(TAG, "[init] (400) Signal name is missing!");
            return FAILURE;
        }
        if (check_ifString(signal_item, "name") != HUB_OK) {
            ESP_LOGE(TAG, "[init] (400) Signal name should be string!");
            return FAILURE;
        }
        if (_check_jSONField(signal_item, "port")) {
            ESP_LOGE(TAG, "[init] (400) Signal port is missing!");
            return FAILURE;
        }
        if (check_ifString(signal_item, "port") == HUB_OK) {
            ESP_LOGE(TAG, "[init] (400) PORT cannot be STRING");
            return FAILURE;
        }
        if (_check_jSONField(signal_item, "direction")) {
            ESP_LOGE(TAG, "[init] (400) Signal direction is missing!");
            return FAILURE;
        }
        if (check_ifString(signal_item, "direction") == HUB_OK) {
            ESP_LOGE(TAG, "[init] (400) Direction cannot be STRING");
            return FAILURE;
        }

        name = _get_jSONField(signal_item, "name")->valuestring[0]; //TODO: assert name
        port = _get_jSONField(signal_item, "port")->valuedouble; //TODO: assert port
        dir = _get_jSONField(signal_item, "direction")->valuedouble;

        if (dir > idir) {
            ESP_LOGE(TAG, "[init] (400) Direction %d", dir);
            return FAILURE;
        }

        machine_p->deployment_info.signals[Resource][signal_idx].name = name;
        machine_p->deployment_info.signals[Resource][signal_idx].port = machines_layout(letters[port], colors[port], dir, Resource);

        if (machines_check_port_free(machine_p->deployment_info.signals[Resource][signal_idx].port)
                == SUCCESS) {
            ESP_LOGE(TAG, "Port [%d] occupied", machine_p->deployment_info.signals[Resource][signal_idx].port);
            return FAILURE;
        }

        machine_p->deployment_info.nof_signals[Resource]++;
        machine_p->deployment_info.signals[Retrofit][signal_idx].name = name;
        machine_p->deployment_info.signals[Retrofit][signal_idx].port =
                machines_layout(letters[port], colors[port], 1 - dir, Retrofit);

        if (machines_check_port_free(machine_p->deployment_info.signals[Retrofit][signal_idx].port)
                == SUCCESS) {
            ESP_LOGE(TAG, "Port [%d] occupied", machine_p->deployment_info.signals[Retrofit][signal_idx].port);
            return FAILURE;
        }

        machine_p->deployment_info.nof_signals[Retrofit]++;
        signal_idx++;
        signal_item = signal_item->next;
    }

    return SUCCESS;
}

uint8_t deployment_sequences(machines_machine_t *machine_p, cJSON *sequences_p, machines_channel_t channel) {
    char *name, *pattern, *status;
    uint8_t target, enabled_cond, idle_cond;

    uint8_t targ0_idx = 0, targ1_idx = 0;
    cJSON *sequence_item = sequences_p->child;

    while (sequence_item != NULL) { //TODO: assert nof sequences
        if (_check_jSONField(sequence_item, "name")) {
            ESP_LOGE(TAG, "[init] (400) Sequence name is missing!");
            return FAILURE;
        }
        if (check_ifString(sequence_item, "name") != HUB_OK) {
            return FAILURE;
        }
        if (_check_jSONField(sequence_item, "target")) {
            ESP_LOGE(TAG, "[init] (400) Sequence target is missing!");
            return FAILURE;
        }
        if (check_ifString(sequence_item, "target") == HUB_OK) {
            return FAILURE;
        }
        if (_check_jSONField(sequence_item, "pattern")) {
            ESP_LOGE(TAG, "[init] (400) Sequence pattern is missing!");
            return FAILURE;
        }
        if (check_ifString(sequence_item, "pattern") != HUB_OK) {
            return FAILURE;
        }

        name = _get_jSONField(sequence_item, "name")->valuestring; //TODO: assert target
        target = (uint8_t) _get_jSONField(sequence_item, "target")->valuedouble; //TODO: assert target
        pattern = _get_jSONField(sequence_item, "pattern")->valuestring; //TODO: assert target
        status = _get_jSONField(sequence_item, "status")->valuestring; //TODO: assert target
        enabled_cond = enabling_condition_dont_care;
        idle_cond = idle_condition_dont_care;

        if (_check_jSONField(sequence_item, "idle")) {
            ESP_LOGW(TAG, "[init] Machine sequence activation enabled condition missing");
        } else {
            enabled_cond = (uint8_t) _get_jSONField(sequence_item, "idle")->valuedouble;
            if (enabled_cond > total_valid_enabled) {

                return FAILURE;
            }
        }
        machine_p->deployment_info.sequences[target][channel][target ? targ1_idx : targ0_idx].enabling_condition = enabled_cond;

        if (_check_jSONField(sequence_item, "enable")) {
            ESP_LOGW(TAG, "[init] Machine sequence activation idle condition missing");
        } else {
            idle_cond = (uint8_t) _get_jSONField(sequence_item, "enable")->valuedouble;
            if (idle_cond > total_valid_enabled) {
                return FAILURE;
            }
        }
        machine_p->deployment_info.sequences[target][channel][target ? targ1_idx : targ0_idx].idle_condition = idle_cond;

        ESP_LOGI(TAG, "[init] got sequence [%s] for target [%d] AND CHANNEL [%d]", pattern, target, channel);
        if (target > Retrofit) {
            ESP_LOGE(TAG, "[init] Invalid target [%d]", target);
            return FAILURE;
        }

        if (machines_sequence(machine_p, target, channel, name) != FAILURE) {
            ESP_LOGE(TAG, "[init] (400) Sequence pattern is missing!");
            return FAILURE;
        }

        if (check_field_content(status, action_status, MAX_ACTION_STATUS) != HUB_OK) {
            return FAILURE;
        }

        strlcpy(machine_p->deployment_info.sequences[target][channel][target ? targ1_idx : targ0_idx].name,
                name, strlen(name) + 1);
        strlcpy(machine_p->deployment_info.sequences[target][channel][target ? targ1_idx : targ0_idx].pattern,
                pattern, strlen(pattern) + 1);
        strlcpy(machine_p->deployment_info.sequences[target][channel][target ? targ1_idx : targ0_idx].status,
                status, strlen(status) + 1);

        machine_p->deployment_info.sequences[target][channel][target ? targ1_idx : targ0_idx].channel = channel;

        machine_p->deployment_info.sequences[target][channel][target ? targ1_idx : targ0_idx].target = target;

        machine_p->deployment_info.nof_sequences[target][channel]++;

        ESP_LOGI(TAG, "Added [%s] seq [%s] to target [%d] on channel [%d]",
                machine_p->deployment_info.sequences[target][channel][target ? targ1_idx : targ0_idx].name,
                machine_p->deployment_info.sequences[target][channel][target ? targ1_idx : targ0_idx].pattern,
                machine_p->deployment_info.sequences[target][channel][target ? targ1_idx : targ0_idx].target,
                machine_p->deployment_info.sequences[target][channel][target ? targ1_idx : targ0_idx].channel);

        target ? targ1_idx++ : targ0_idx++;
        sequence_item = sequence_item->next;
    }

    ESP_LOGI(TAG, "Received [%d] actions for RETROFIT, [%d] for RESOURCE",
            machine_p->deployment_info.nof_sequences[Retrofit][Action],
            machine_p->deployment_info.nof_sequences[Resource][Action]);

    return SUCCESS;
}

static hub_error_t deployment_fix_cb(cJSON * message) {
     if (_check_jSONField(message, "resource")) {
         ESP_LOGE(TAG,"[Fix] No resource field");
        return HUB_ERROR_DEPL_GEN;
    }
    if (deployment_resource_fix(_get_jSONField(message, "resource")) != SUCCESS) {
        return HUB_ERROR_DEPL_GEN;
    }
    machines_machine_t * machine_to_fix = get_resource_tofix(_get_jSONField(message, "resource"));
    machines_clear_all_status(machine_to_fix);
    return HUB_OK;
}

uint8_t deployment_resource_fix(cJSON *resource_p) {
    machines_machine_t *_machine_p;
    uint8_t _resource = resource_p->valuedouble;
    ESP_LOGI(TAG,"Checking resource [%d] existance",_resource);
    if (machines_machine(&_machine_p, resource_p->valuedouble) == FAILURE) {
        ESP_LOGE(TAG, "[FIX] (400) Resource name unexistant!");
        return FAILURE;
    }
    return SUCCESS;
}

machines_machine_t* get_resource_tofix(cJSON *resource_p){
    machines_machine_t *_machine_p;

    if (machines_machine(&_machine_p, resource_p->valuedouble) == FAILURE) {
        ESP_LOGE(TAG, "[FIX] (400) Resource name unexistant!");

        return NULL;
    }
    return _machine_p;
}

void deployment_erase_last(void) {
    machines_clear_next();
}

static hub_error_t deployment_clear_cb(cJSON * message) {
    if (_check_jSONField(message, "resource")) {
        ESP_LOGE(TAG,"[Clear] No resource field");
        return HUB_ERROR_DEPL_GEN;
    }
    machines_machine_t *_machine_p;
    if (machines_machine(&_machine_p, _get_jSONField(message, "resource")->valuedouble) == FAILURE) {
        ESP_LOGE(TAG, "[CLR] (400) No resource [%d]!", (uint8_t) message->valuedouble);
        return HUB_ERROR_DEPL_GEN;
    }
    machines_free(&_machine_p);
    return HUB_OK;
}

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
hub_error_t deployment_syntax_check(cJSON * object) {
    if (_check_jSONField(object,
            msg_fields[resourceField]) != HUB_OK) {
        return HUB_ERROR_DEPL_GEN;
    }

    if (_check_jSONField(object,
            msg_fields[actionField]) != HUB_OK) {
        return HUB_ERROR_DEPL_MISSING_ACTIONS;
    }

    if (check_ifString(object,
            msg_fields[actionField]) != HUB_OK) {
        return HUB_ERROR_DEPL_MISSING_ACTIONS;
    }

    char * action_field = _get_jSONField(object,
            msg_fields[actionField])->valuestring;

    if (check_field_content(action_field, deploymentActions, TOTAL_DEPLOYMENT_ACTIONS) != HUB_OK) {
        return HUB_ERROR_DEPL_MISSING_ACTIONS;
    }
    ESP_LOGI(TAG, "Deployment Syntax check OK");
    return HUB_OK;
}

hub_error_t deployment_content_check(cJSON * object) {
    int16_t resource_field = _get_jSONField(object,
            msg_fields[resourceField])->valuedouble;
    if (resource_field < 0 || resource_field > 255) {
        ESP_LOGE(TAG, "invalid resurce [%d]", resource_field);
        return HUB_ERROR_DEPL_GEN;
    }

    ESP_LOGI(TAG, "Deployment CONTENT check OK");
    return HUB_OK;
}

hub_error_t init_syntax_check(cJSON * object) {
    if (_check_jSONField(object,
            msg_fields[templateField]) != HUB_OK) {
        return HUB_ERROR_DEPL_GEN;
    }

    cJSON * template_parsed = _get_jSONField(object,
            msg_fields[templateField]);

    if (template_syntax_check(template_parsed)
            != HUB_OK) {
        return HUB_ERROR_DEPL_GEN;
    }
    ESP_LOGI(TAG, "Template Syntax OK ");
    return HUB_OK;
}

hub_error_t template_syntax_check(cJSON * object) {
    for (uint8_t k = 0; k < TEMPLATE_TOTAL_FIELDS; k++) {
        if (_check_jSONField(object,
                template_fields[k]) != HUB_OK) {
            return HUB_ERROR_DEPL_GEN;
        }
    }

    /*CHECKING SIGNALS****/
    return HUB_OK;
}

uint8_t sequence_syntax_check(char * sequence) {
    return SUCCESS;
}
