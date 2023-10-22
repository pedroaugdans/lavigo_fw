/***
 *     /$$$$$$$                      /$$             /$$                          /$$     /$$
 *    | $$__  $$                    |__/            | $$                         | $$    |__/
 *    | $$  \ $$  /$$$$$$   /$$$$$$  /$$  /$$$$$$$ /$$$$$$    /$$$$$$  /$$$$$$  /$$$$$$   /$$  /$$$$$$  /$$$$$$$
 *    | $$$$$$$/ /$$__  $$ /$$__  $$| $$ /$$_____/|_  $$_/   /$$__  $$|____  $$|_  $$_/  | $$ /$$__  $$| $$__  $$
 *    | $$__  $$| $$$$$$$$| $$  \ $$| $$|  $$$$$$   | $$    | $$  \__/ /$$$$$$$  | $$    | $$| $$  \ $$| $$  \ $$
 *    | $$  \ $$| $$_____/| $$  | $$| $$ \____  $$  | $$ /$$| $$      /$$__  $$  | $$ /$$| $$| $$  | $$| $$  | $$
 *    | $$  | $$|  $$$$$$$|  $$$$$$$| $$ /$$$$$$$/  |  $$$$/| $$     |  $$$$$$$  |  $$$$/| $$|  $$$$$$/| $$  | $$
 *    |__/  |__/ \_______/ \____  $$|__/|_______/    \___/  |__/      \_______/   \___/  |__/ \______/ |__/  |__/
 *                         /$$  \ $$
 *                        |  $$$$$$/
 *                         \______/
 */

#include "registration.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "cJSON.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "drv_nvs.h"
#include "drv_uart.h"
#include "launcher.h"
#include "params.h"
#include "hub_error.h"
#include "cJSON.h"
#include "msg_fmt.h"
#include "base64_coding.h"
#include "drv_mqtt.h"
#include "drv_wifi.h"
#include "connection.h"

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define SECRET_LENGTH AWSCRT_SECRET_LENGTH + 16
#define MAC_BUFFER_LENGTH DEVZID_SECRET_LENGTH

#define SECRET_PREFIX "secret"

/*** VARIABLES **************************************************************************/

static const char *TAG = "[RGSTR]";

/*These commands serve as a string to:
*Be a key to locate the values of the key in flash
*serve as commands to download rotation keys from cloud platform*/
const char *registration_commands[] = {
    DEVZID_IDX,//OK
    IOTURL_IDX,//OK
    SYSSTG_IDX,//
    HUBZID_IDX,
    ECCKEY_IDX,
    ECCCRT_IDX,
    AWSCRT_IDX,
    APSSID_IDX,
    APPSWD_IDX,
    UPDKEY_IDX,
    MAPIDX_IDX,
    "helpme"
};

/*This array contains the maximun length of each key*/
uint16_t registration_commandLengths[] = {
    DEVZID_SECRET_LENGTH,
    IOTURL_SECRET_LENGTH,
    SYSSTG_SECRET_LENGTH,
    HUBZID_SECRET_LENGTH,
    ECCKEY_SECRET_LENGTH,
    ECCCRT_SECRET_LENGTH,
    AWSCRT_SECRET_LENGTH,
    APSSID_CONFIG_MIN_LENGTH,
    MAPIDX_MIN_LEN,
    APPSWD_CONFIG_MIN_LENGTH,
    UPDKEY_SECRET_LENGTH,
    0
};

bool registration_checkpoints[NOF_REG_CMD] = {0};
static bool isStageFInished = FALSE;
msg_validation_callback_t validation_callback = NULL;
static uint8_t rotation_attempts = 0;

drv_key_storage current_used_keys[HELPME_CMD];
drv_key_storage last_used_keys[HELPME_CMD];
bool rol_status[HELPME_CMD];
/*** DECLARATIONS ***********************************************************************/

/***
 *     _                                     __ _
 *    | |                                   / _(_)
 *    | | _____ _   _ ___    ___ ___  _ __ | |_ _  __ _
 *    | |/ / _ \ | | / __|  / __/ _ \| '_ \|  _| |/ _` |
 *    |   <  __/ |_| \__ \ | (_| (_) | | | | | | | (_| |
 *    |_|\_\___|\__, |___/  \___\___/|_| |_|_| |_|\__, |
 *               __/ |                             __/ |
 *              |___/                             |___/
 */
void registration_dispatchCommand(registration_command_t next_cmd);
/*<read and save a whole key through UART, of at least secretMinLen characters
and stop reading at the appearance of terminator*/
char *registration_getSecret(uint32_t secretMinLen, char *terminator);
/*<print the requested key in console*/
void registration_printSecret(char *secret_p);
/*<get device number, which is the MAC adress in ESP32*/
void registration_getMAC(uint8_t *mac_buffer_p);
/*<print THE DEVICE MAC ADRESS*/
void registration_printMAC(uint8_t *mac_buffer_p);
/*<send HUBACK message through UART for command acknowledging in current scripts*/
void registration_printACK();

/*<Load main flash information for hub from FLASH to RAM*/
void registration_storeParams();
/*<Save the indicated key (secret_p) in secretId, to flash*/
void registration_logSecret(const char *secretId, char *secret_p);
/*<print connection key in console*/
void connection_logConfig(const char *configId);
/*<get the connection key from UART*/
char * connection_getConfig(uint32_t configMinLen, char *terminator);

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

static void task_idle(void);
static bool task_isCheck(void);
static bool task_check(void);
static void check_publish(void);
static void check_sleep(uint8_t ticks)__attribute__((unused));
static void task_init(void);
/*<default sleep task routine*/
void registration_sleep(uint8_t ticks);
/*<Checks for activation flag, set by the master fsm initial flashOK effect*/
bool registration_isActive();
/*<Check if any key is remaining to be downloaded*/
uint8_t registration_isIncomplete();
/*<check for every key and alternative space (for key roatition feature)*/
uint8_t registration_Newversion_isIncomplete(void);
/*<Read next command for key downloading, from UART*/
uint8_t registration_handleNextCommand();
/*<dispatch the received command to its correspondent key parsing*/
/*<Load main flash information for hub from FLASH to RAM*/
void registration_publishEvent();

/***
 *               _        _   _
 *              | |      | | (_)
 *     _ __ ___ | |_ __ _| |_ _  ___  _ __
 *    | '__/ _ \| __/ _` | __| |/ _ \| '_ \
 *    | | | (_) | || (_| | |_| | (_) | | | |
 *    |_|  \___/ \__\__,_|\__|_|\___/|_| |_|
 *
 *
 */
/*<callback from registration for msg_fmt to be called upon receiving a command from
the cloud platform*/
static void registration_callback(void);
/*<Unimplemented Intended to inform about errors in rotation*/
static void registration_post_result(void)__attribute__((unused));
/*<execue rotation, will trigger a test that uses newly added keys*/
static void _action_forward_cb(cJSON * info)__attribute__((unused));
/*<this function will be called upon unsuccessful role forward, and will
roleback newly installed keys*/
static void rolback_key(registration_command_t key);
/*<Dispatch received command from cloud platform, when rotating*/
static registration_command_t dispatch_command(char * receivde_command);
/*empty(assign 0) buffer buffer_to_clean up to length index*/
static void clean_name_buffer(char * buffer_to_clean, uint8_t length);

/*** DEFINITIONS ************************************************************************/

void registration_set_validation_callback(msg_validation_callback_t new_validation_callbacl){
  if(new_validation_callbacl == NULL){
    ESP_LOGE(TAG,"Registering NULL validation callback");
    return;
  }
  validation_callback = new_validation_callbacl;
}

static registration_msg_actions dispatch_registration_message(char * action){
  registration_msg_actions action_to_return = registration_msg_validate;
  if(!strcmp(action,registration_actions[registration_msg_validate])){
    action_to_return = registration_msg_validate;
  } else if (!strcmp(action,registration_actions[registration_msg_rotate])){
    action_to_return = registration_msg_rotate;
  } else if (!strcmp(action,registration_actions[registration_msg_roleback])){
    action_to_return = registration_msg_roleback;
  } else if (!strcmp(action,registration_actions[registration_msg_roleforward])){
    action_to_return = registration_msg_roleforward;
  } else {
    action_to_return = MAX_REGISTRATION_MSGS;
  }
  return action_to_return;
}

void _action_forward_cb(cJSON * info){
  rotation_attempts = 0;
  connection_reset();
  force_disconnection();
//trigger registration
}

static void role_forward_failed(drv_mqtt_message_t *msg){
  bool am_i_trying_to_rotate = false;
  for(uint8_t k = 0;k<HELPME_CMD;k++){
    if(rol_status[k]){
      am_i_trying_to_rotate = true;
    }
  }
  if(!am_i_trying_to_rotate){
    ESP_LOGW(TAG,"Not trying any rotations.");
    return;
  }
  if(rotation_attempts<MAX_ROTATIONS_ATTEMPTS){
    ESP_LOGW(TAG,"Still [%d] attempts pending",MAX_ROTATIONS_ATTEMPTS - rotation_attempts);
    rotation_attempts++;
    return;
  }

  ESP_LOGW(TAG,"Rolling back");
  for(uint8_t k = 0;k<HELPME_CMD;k++){
    if(rol_status[k]){
      am_i_trying_to_rotate = true;
      rolback_key(k);
    }
  }
  _action_forward_cb(NULL);
}

static void save_current_keys_metadata(void){
  char * current_keys_encoded_info = (char*) calloc(2 * sizeof (drv_key_storage), HELPME_CMD);
  if (drv_nvs_check(KEYS_CURRENT_BUFFER) == SUCCESS) {
      ESP_LOGI(TAG, "Saving current keys metadata");
  }

  b64_encode((const unsigned char *) current_used_keys, current_keys_encoded_info, sizeof (drv_key_storage)*HELPME_CMD);
  drv_nvs_set(KEYS_CURRENT_BUFFER, current_keys_encoded_info);
  free(current_keys_encoded_info);
  return;
}

static void save_old_keys_metadata(void){
  char * old_keys_encoded_info = (char*) calloc(2 * sizeof (drv_key_storage), HELPME_CMD);
  if (drv_nvs_check(KEYS_LAST_BUFFER) == SUCCESS) {
      ESP_LOGI(TAG, "Saving last keys metadata");
      ///drv_nvs_clear(machine_file_name);
  }

  b64_encode((const unsigned char *) last_used_keys, old_keys_encoded_info, sizeof (drv_key_storage)*HELPME_CMD);
  drv_nvs_set(KEYS_LAST_BUFFER, old_keys_encoded_info);
  free(old_keys_encoded_info);

  return;
}

static void save_rol_status_metadata(void){

}

static void rolback_key(registration_command_t key){
  ESP_LOGW(TAG,"Rolling [%d] back",key);
  char flash_pointer_name[12] = {0};
  char old_url[50] = {0};
  drv_key_storage swap_helper;
  swap_helper.key_flash_idx = last_used_keys[key].key_flash_idx;
  swap_helper.key_version = last_used_keys[key].key_version;

  last_used_keys[key].key_flash_idx = current_used_keys[key].key_flash_idx;
  last_used_keys[key].key_version = current_used_keys[key].key_version;

  current_used_keys[key].key_flash_idx = swap_helper.key_flash_idx;
  current_used_keys[key].key_version = swap_helper.key_version;

  assemble_key_name(flash_pointer_name,registration_commands[key],last_used_keys[key].key_flash_idx);
  drv_nvs_get(flash_pointer_name,old_url);
  ESP_LOGE(TAG,"Element in last buffer [%s]: [%s]",flash_pointer_name,old_url);
  clean_name_buffer(flash_pointer_name,12);
  clean_name_buffer(old_url,50);


  assemble_key_name(flash_pointer_name,registration_commands[key],current_used_keys[key].key_flash_idx);
  drv_nvs_get(flash_pointer_name,old_url);
  ESP_LOGE(TAG,"Element in new buffer [%s]: [%s]",flash_pointer_name,old_url);

  save_current_keys_metadata();
  save_old_keys_metadata();
  save_rol_status_metadata();

  return;
}

static void rotate_key(registration_command_t key, char * value){
  char flash_pointer_name[12] = {0};
  char old_url[50] = {0};

  uint8_t new_version_number =
    last_used_keys[key].key_version > current_used_keys[key].key_version?
    last_used_keys[key].key_version : current_used_keys[key].key_version;
  new_version_number++;

  ESP_LOGI(TAG,"New key [%d] version number: [%d]",key,new_version_number);

  last_used_keys[key].key_flash_idx = current_used_keys[key].key_flash_idx;
  last_used_keys[key].key_version = current_used_keys[key].key_version;

  assemble_key_name(flash_pointer_name,registration_commands[key],last_used_keys[key].key_flash_idx);
  drv_nvs_get(flash_pointer_name,old_url);
  ESP_LOGE(TAG,"Element in last buffer [%s]: [%s]",flash_pointer_name,old_url);

  current_used_keys[key].key_version = new_version_number;
  drv_keys_buffer_t new_pointer =
    current_used_keys[key].key_flash_idx == parameters_0?parameters_1:parameters_0;

  ESP_LOGI(TAG,"moving from index [%d] to index [%d]",
    current_used_keys[key].key_flash_idx,new_pointer);

  current_used_keys[key].key_flash_idx = new_pointer;
  clean_name_buffer(flash_pointer_name,12);
  clean_name_buffer(old_url,50);
  assemble_key_name(flash_pointer_name,registration_commands[key],current_used_keys[key].key_flash_idx);
  ESP_LOGI(TAG,"Saving key value [%s] in [%s]",value,flash_pointer_name);
  drv_nvs_set(flash_pointer_name, value);

  save_current_keys_metadata();
  save_old_keys_metadata();
  save_rol_status_metadata();

  rol_status[key] = 1;
  return;
}

static void dispatch_received_key(cJSON * info){
  if(info == NULL){
    ESP_LOGE(TAG,"No useful INFO field received");
    return;
  }
  if (check_ifString(info, "key") != HUB_OK) {
    ESP_LOGE(TAG,"No key in info");
      return;
  } else {
    ESP_LOGI(TAG,"Saving new key [%s]",_get_jSONField(info, "key")->valuestring);
  }
  if (check_ifString(info, "value") != HUB_OK) {
    ESP_LOGE(TAG,"No value in info");
      return;
  } else {
    ESP_LOGI(TAG,"Got value [%s]",_get_jSONField(info, "value")->valuestring);
  }
  char * key = _get_jSONField(info, "key")->valuestring;

  registration_command_t received_key = dispatch_command(key);

  if(received_key<NOF_REG_CMD){
    ESP_LOGI(TAG,"Rotating!");
  } else {
    ESP_LOGE(TAG,"Unrecognized received key");
    return;
  }

  rotate_key(received_key,_get_jSONField(info, "value")->valuestring);
}

static hub_error_t registration_check_syntax(cJSON * root){
  if (_check_jSONField(root,
          msg_fields[actionField]) != HUB_OK) {
      return HUB_ERROR_DEPL_MISSING_ACTIONS;
  }
  return HUB_OK;
}

uint8_t registration_load_old_buffer(void){
  char * old_buffer_encoded_info = (char*) calloc(2 * sizeof (drv_key_storage), HELPME_CMD);

  if(drv_nvs_get(KEYS_LAST_BUFFER, old_buffer_encoded_info) != HUB_OK){
    ESP_LOGW(TAG,"No old keys found");
    return FAILURE;
  }
  b64_decode((const char *) old_buffer_encoded_info, (unsigned char *) last_used_keys);
  free(old_buffer_encoded_info);
  return SUCCESS;
}

uint8_t registration_load_current_buffer(void){
  char * current_buffer_encoded_info = (char*) calloc(2 * sizeof (drv_key_storage), HELPME_CMD);

  if(drv_nvs_get(KEYS_CURRENT_BUFFER, current_buffer_encoded_info) != HUB_OK){
    ESP_LOGW(TAG,"No current keys found");
    return FAILURE;
  }
  b64_decode((const char *) current_buffer_encoded_info, (unsigned char *) current_used_keys);
  free(current_buffer_encoded_info);
  return SUCCESS;
}

void registration_load_rol_status(void){
  //unimplemented
}

void initialize_keys_buffers(void){
  registration_load_rol_status();
  if(registration_load_current_buffer() != SUCCESS){
    for(uint8_t k = 0 ; k < HELPME_CMD; k++){
      current_used_keys[k].key_flash_idx = parameters_0;
      current_used_keys[k].key_version = (uint8_t)0;
    }
    save_current_keys_metadata();
    ESP_LOGW(TAG,"No current buffer found");
  } else {
    ESP_LOGI(TAG,"Found Current keys");
    for(uint8_t k = 0 ; k < HELPME_CMD; k++){
      ESP_LOGI(TAG,"[%s] idx[%d] ver[%d]",registration_commands[k],
      current_used_keys[k].key_flash_idx,current_used_keys[k].key_version);
    }
  }
  if(registration_load_old_buffer() != SUCCESS){
    ESP_LOGW(TAG,"No old buffer found");
  }
  for(uint8_t k = 0 ; k < HELPME_CMD; k++){
    rol_status[k] = 0;
  }
}

void TEST_execute_regisration_callback(void){
  registration_callback();
}

static void registration_callback(void){
  cJSON *root = cJSON_Parse(msg_fmt_message_static(Validation));
  ESP_LOGI(TAG,"%s, pointer: [%p]",msg_fmt_message_static(Validation),root);
  if (registration_check_syntax(root) != HUB_OK) {
    ESP_LOGE(TAG,"No action in message");
      //registration_post_result(HUB_ERROR_DEPL_GEN, deployment_msg_clear, resource_id);
      return;
  }
  char * received_action_str = _get_jSONField(root,
            msg_fields[actionField])->valuestring;
  registration_msg_actions received_action = dispatch_registration_message(received_action_str);
  switch (received_action){
    case registration_msg_validate:
      ESP_LOGI(TAG,"Received [VALIDATE]");
      if(validation_callback != NULL){
          validation_callback();
      }
    break;
    case registration_msg_rotate:
      ESP_LOGI(TAG,"Received [ROTATE]");
      dispatch_received_key(_get_jSONField(root, "info"));
    break;
    case registration_msg_roleback:
      ESP_LOGI(TAG,"Received [ROLEBACK]");
    break;
    case registration_msg_roleforward:
      ESP_LOGI(TAG,"Received [ROLEFORWARD]");
      _action_forward_cb(_get_jSONField(root, "info"));
    break;
    default:
      ESP_LOGE(TAG,"Unknown action");
  }
  stash_jSON_msg(root);
}

void delete_all_keys(void) {
    ESP_LOGW(TAG, "Deleting all keys");
    drv_nvs_clear(registration_commands[1]);
    drv_nvs_clear(registration_commands[2]);
    drv_nvs_clear(registration_commands[3]);
    drv_nvs_clear(registration_commands[4]);
}

void registration_task(void *params) {
    task_init();
    FOREVER{
        if (task_isCheck()) {
            bool publish_event = task_check();
            if (publish_event) {
                check_publish();
            }
        } else {
            task_idle();
        }
        registration_sleep(1);}
}

static bool task_isCheck(void) {
    /*DEBUG*///ESP_LOGI(TAG, "isActive()");
    return check_activation_flags[registation_check_flag];
}

static void task_init(void){
  msg_fmt_install(Validation, registration_callback);
  drv_mqtt_install(role_forward_failed,ConnFailed);
  initialize_keys_buffers();
}

static bool task_check(void) {
    bool publish_event = (!registration_Newversion_isIncomplete());
    bool commandToHandle = registration_handleNextCommand();
    return publish_event&&commandToHandle;
}

static void task_idle(void) {
    //ESP_LOGI(TAG,"idle()");
    registration_sleep(1);
}

static void clean_name_buffer(char * buffer_to_clean, uint8_t length){
  for(uint8_t k = 0; k<length;k++){
    buffer_to_clean[k]=0;
  }
}

uint8_t registration_Newversion_isIncomplete(void) {
    /*DEBUG*/ESP_LOGI(TAG, "isIncomplete()");

    uint8_t pending = 0;
    char current_key_name[12] = {0};

    for (int i = IOTURL_CMD; i <= MAPIDX_CMD; i++) {
      clean_name_buffer(current_key_name,12);
      assemble_key_name(current_key_name,registration_commands[i],
        current_used_keys[i].key_flash_idx);
        if (drv_nvs_check(current_key_name) != ESP_OK) {
            pending += 1;
        }
    }

    if (0 < pending && pending < (MAPIDX_CMD - IOTURL_CMD)) {
        ESP_LOGE(TAG, "Uneven Initialization! Warning! Possible error with ROM!\n");
    }
    return pending != 0;
}

static registration_command_t dispatch_command(char * receivde_command){
  for (uint8_t i = 0; i < NOF_REG_CMD; i++) {
    if (!strcmp(registration_commands[i], receivde_command)) {
      return (registration_command_t)i;
    }
  }
  return NOF_REG_CMD;
}

uint8_t registration_handleNextCommand() {
    /*DEBUG*/ESP_LOGI(TAG, "handleNextCommand()");

    bool err;
    registration_command_t next_cmd;
    next_cmd = (registration_command_t) drv_uart_next(registration_commands, NOF_REG_CMD);

    if (next_cmd < NOF_REG_CMD) {
        registration_dispatchCommand(next_cmd);

        err = 0;
    } else {
        err = 1;
    }

    /*DEBUG*/ESP_LOGI(TAG, "~handleNextCommand()");

    return err;
}

void registration_dispatchCommand(registration_command_t next_cmd) {
    set_ram_usage(uxTaskGetStackHighWaterMark(NULL), registration_task_idx);
    /*DEBUG*/ESP_LOGI(TAG, "dispatchCommand()");
    char flash_pointer_name[12] = {0};
    uint8_t mac_buffer[DEVZID_SECRET_LENGTH + 6] = {0};
    char *secret_p;

    switch (next_cmd) {
        case DEVZID_CMD:
            registration_getMAC(mac_buffer);
            registration_printMAC(mac_buffer);
            break;

        case IOTURL_CMD:
        case SYSSTG_CMD:
        case HUBZID_CMD:
        case ECCKEY_CMD:
        case ECCCRT_CMD:
        case AWSCRT_CMD:
        case UPDKEY_CMD:
        case MAPIDX_CMD:
            ///*DEBUG*/ESP_LOGI(TAG, "Got command %d", next_cmd);
            secret_p = registration_getSecret(registration_commandLengths[next_cmd], REG_TERMINATOR);

            drv_keys_buffer_t current_buffer_used = current_used_keys[next_cmd].key_flash_idx;
            assemble_key_name(flash_pointer_name,registration_commands[next_cmd],current_buffer_used);
            drv_nvs_set(flash_pointer_name, secret_p);

            registration_printACK();
            registration_logSecret(registration_commands[next_cmd], secret_p);
            break;
        case APSSID_CMD:
        case APPSWD_CMD:
            secret_p = connection_getConfig(registration_commandLengths[next_cmd], REG_TERMINATOR);
            drv_nvs_set(registration_commands[next_cmd], secret_p);
            registration_printACK();
            connection_logConfig(registration_commands[next_cmd]);
            break;
        case HELPME_CMD:
            registration_logHelp();
            break;
        default:
            ESP_LOGE(TAG, "Unknown key");
            break;
    }

    /*DEBUG*/ESP_LOGI(TAG, "~dispatchCommand()");
}

char *registration_getSecret(uint32_t secretMinLen, char *terminator) {
    ///*DEBUG*/ESP_LOGI(TAG, "getSecret()");

    char *secret = NULL;

    drv_uart_fetch(terminator, &secret);

    ///*DEBUG*/ESP_LOGI(TAG, "%s", secret);


    ///*DEBUG*/ESP_LOGI(TAG, "~getSecret()");

    return secret;
}

void registration_set_layout_version(char * version) {
    drv_nvs_set(MAPIDX_IDX, version);
}

void delete_layout(void){
    drv_nvs_set(MAPIDX_IDX,"undefined");
}

void registration_printSecret(char *secret_p) {
    /*DEBUG*/ESP_LOGI(TAG, "printSecret()");

    for (unsigned int j = 0; secret_p[j] != 00; j++) {
        printf("%c", (char) secret_p[j]);
    }
    printf("\n");

    /*DEBUG*/ESP_LOGI(TAG, "~printSecret()");
}

void registration_getMAC(uint8_t *mac_buffer_p) {
    /*DEBUG*/ESP_LOGI(TAG, "getMAC()");

    esp_efuse_mac_get_default(mac_buffer_p);

    /*DEBUG*/ESP_LOGI(TAG, "~getMAC()");
}

void registration_printMAC(uint8_t *mac_buffer_p) {
    /*DEBUG*/ESP_LOGI(TAG, "printMAC()");

    const char prefix[6] = SECRET_PREFIX;

    for (uint8_t prefix_idx = 0; prefix_idx < 6; prefix_idx++) {
        printf("%c", prefix[prefix_idx]);
    }

    for (uint8_t mac_idx = 0; mac_idx < MAC_BUFFER_LENGTH; mac_idx++) {
        printf("%02x", mac_buffer_p[mac_idx]);
    }
    printf("\n");

    /*DEBUG*/ESP_LOGI(TAG, "~printMAC()");
}

void registration_printACK() {
    printf(REG_ACK);
}

void registration_storeParams() {
    /*DEBUG*/ESP_LOGI(TAG, "storeParams()");

    char _hubId[40] = {0};

    drv_nvs_get(HUBZID_IDX, _hubId);
    params_set(HUBID_PARAM, _hubId);

    char _stage[40] = {0};
    drv_nvs_get(SYSSTG_IDX, _stage);
    params_set(STAGE_PARAM, _stage);

    char _hardv[20] = {0};
    drv_nvs_get(MAPIDX_IDX, _hardv);
    params_set(HARDV_PARAM, _hardv);
    dispatch_hardware_version(_hardv);

    /*DEBUG*/ESP_LOGI(TAG, "~storeParams()");
}

static void check_publish(void) {
    /*DEBUG*/ESP_LOGI(TAG, "publishEvent()");
    masterFSM_events main_mode_trigger = crtld_Mevent;
    char last_loaded_event[8] = {0};
    registration_storeParams();

    if (!is_hardware_version_selected) {
        ESP_LOGW(TAG, "No hardware version selected. Setting idle mode");
        main_mode_trigger = idtrg_Mevent;

    } else {
        if (drv_nvs_check(HUB_LAST_MODE_KEY) != SUCCESS) {
            ESP_LOGI(TAG, "Taking [%s] as default", HUB_DEFAULT_STARTUP_MODE);
        } else {
            drv_nvs_get(HUB_LAST_MODE_KEY, last_loaded_event);
            ESP_LOGI(TAG, "Engaging for [%s]", last_loaded_event);

            if (!strcmp(last_loaded_event, HUB_NORMAL_MODE_KEY)) {
                main_mode_trigger = crtld_Mevent;
            } else if (!strcmp(last_loaded_event, HUB_FALLBACK_MODE_KEY)) {
                main_mode_trigger = trgfb_Mevent;
            }
        }
    }
    isStageFInished = TRUE;
    fsm_q_evt(main_mode_trigger);
    /*DEBUG*/ESP_LOGI(TAG, "~publishEvent()");
}

void registration_sleep(uint8_t ticks) {
    if (isStageFInished) {
        vTaskDelay(suspendedThreadDelay[registration_flag] * ticks * 25 / portTICK_RATE_MS);
    } else {
        vTaskDelay(suspendedThreadDelay[registration_flag] * ticks / portTICK_RATE_MS);
    }
    set_ram_usage(uxTaskGetStackHighWaterMark(NULL), registration_task_idx);
}

static void check_sleep(uint8_t ticks) {
    vTaskDelay(suspendedThreadDelay[registration_flag] * ticks / portTICK_RATE_MS);
}

void registration_logSecret(const char *secretId, char *secret_p) {
    /*DEBUG*/ESP_LOGI(TAG, "logSecret() %s", secretId);

    drv_nvs_get(secretId, secret_p);
    registration_printSecret(secret_p);

    /*DEBUG*/ESP_LOGI(TAG, "~logSecret()");
}

void registration_logHelp() {
    /*DEBUG*/ESP_LOGI(TAG, "logHelp()");

    ESP_LOGI(TAG, "Now on registration console, press 'q' for exit");

    /*DEBUG*/ESP_LOGI(TAG, "~logHelp()");
}

char *connection_getConfig(uint32_t configMinLen, char *terminator) {
    /*DEBUG*/ESP_LOGI(TAG, "getConfig()");

    char *config = NULL;

    drv_uart_fetch(terminator, &config);

    /*DEBUG*/ESP_LOGI(TAG, "~getConfig()");

    return config;
}

void connection_logConfig(const char *configId) {
    /*DEBUG*/ESP_LOGI(TAG, "logConfig() %s", configId);

    drv_nvs_get(configId, config_p);
    connection_printConfig(config_p);

    /*DEBUG*/ESP_LOGI(TAG, "~logConfig()");
}

void connection_clearKeys(void) {
    drv_nvs_clear(registration_commands[APSSID_CMD]);
    drv_nvs_clear(registration_commands[APPSWD_CMD]);
}
