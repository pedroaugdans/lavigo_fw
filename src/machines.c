//////////////////////////////////////////////////////////////////////////////////////////
//     __  ___ ___    ______ __  __ ____ _   __ ______ _____                            //
//    /  |/  //   |  / ____// / / //  _// | / // ____// ___/                            //
//   / /|_/ // /| | / /    / /_/ / / / /  |/ // __/   \__ \                             //
//  / /  / // ___ |/ /___ / __  /_/ / / /|  // /___  ___/ /                             //
// /_/  /_//_/  |_|\____//_/ /_//___//_/ |_//_____/ /____/                              //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "machines.h"
#include "string.h"
#include "pin_xio.h"
#include "pin_seq.h"
#include "messages.h"
#include "base64_coding.h"
#include "string.h"
#include "drv_spiffs.h"
#include "drv_nvs.h"
#include "params.h"
#include "freertos/semphr.h"
#include "machines_mapping.h"

#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00


/*** VARIABLES **************************************************************************/

static const char *TAG = "[RSRCS]";

const char *machines_rows[ROW_MAX] = {
    ROW_1,
    ROW_2,
    ROW_3,
    ROW_4,
};

const char *machines_columns[COLUMN_MAX] = {
    COLUMN_1,
    COLUMN_2,
    COLUMN_3,
    COLUMN_4,
    COLUMN_5,
    COLUMN_6,
    COLUMN_7,
    COLUMN_8
};

const uint8_t machines_adapters_mapping[TOTAL_LAYOUT_VERSIONS][ROW_MAX][COLUMN_MAX] = {{
   /*--------------------------------------------*/  /*-----------------------*/
   {0x00, 0x00, 0x20, 0x20, 0x40, 0x40, 0x60, 0x60}, /* C C | A A | A A | A A */
   /*                                            */  /*     |     |     |     */
   {0x00, 0x00, 0x20, 0x20, 0x40, 0x40, 0x60, 0x60}, /* C C | A A | A A | A A */
   /*                                            */  /*     |     |     |     */
   /*--------------------------------------------*/  /*-----------------------*/
   {0x10, 0x10, 0x30, 0x30, 0x50, 0x50, 0x70, 0x70}, /* A A | A A | A A | A A */
   /*                                            */  /*     |     |     |     */
   {0x10, 0x10, 0x30, 0x30, 0x50, 0x50, 0x70, 0x70}, /* A A | A A | A A | A A */
   /*                                            */  /*     |     |     |     */
   /*--------------------------------------------*/  /*-----------------------*/
},{
   /*--------------------------------------------*/  /*-----------------------*/
   {0x00, 0x00, 0x20, 0x20, 0x40, 0x40, 0x60, 0x60}, /* C C | A A | A A | A A */
   {0x00, 0x00, 0x20, 0x20, 0x40, 0x40, 0x60, 0x60}, /* C C | A A | A A | A A */
   {0x00, 0x00, 0x20, 0x20, 0x40, 0x40, 0x60, 0x60}, /* C C | A A | A A | A A */
   {0x00, 0x00, 0x20, 0x20, 0x40, 0x40, 0x60, 0x60}, /* C C | A A | A A | A A */
   /*--------------------------------------------*/  /*-----------------------*/
   /*                                            */  /*     |     |     |     */
   /*                                            */  /*     |     |     |     */
   /*                                            */  /*     |     |     |     */
   /*                                            */  /*     |     |     |     */
   /*--------------------------------------------*/  /*-----------------------*/
}};
const uint8_t machines_adapters_delta[TOTAL_LAYOUT_VERSIONS][ROW_MAX][COLUMN_MAX] = {{
   /*--------------------------------------------*/  /*-----------------------*/
   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* C C | A A | A A | A A */
   /*                                            */  /*     |     |     |     */
   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* C C | A A | A A | A A */
   /*                                            */  /*     |     |     |     */
   /*--------------------------------------------*/  /*-----------------------*/
   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* A A | A A | A A | A A */
   /*                                            */  /*     |     |     |     */
   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* A A | A A | A A | A A */
   /*                                            */  /*     |     |     |     */
   /*--------------------------------------------*/  /*-----------------------*/
},{
   /*--------------------------------------------*/  /*-----------------------*/
   {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10}, /* C C | A A | A A | A A */
   {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10}, /* C C | A A | A A | A A */
   {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10}, /* C C | A A | A A | A A */
   {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10}, /* C C | A A | A A | A A */
   /*--------------------------------------------*/  /*-----------------------*/
   /*                                            */  /*     |     |     |     */
   /*                                            */  /*     |     |     |     */
   /*                                            */  /*     |     |     |     */
   /*                                            */  /*     |     |     |     */
   /*--------------------------------------------*/  /*-----------------------*/
}};

const uint8_t machines_machines_mapping[TOTAL_LAYOUT_VERSIONS][ROW_MAX][COLUMN_MAX] = {{
   /*--------------------------------------------*/  /*-----------------------*/
   {0x07, 0x06, 0x07, 0x06, 0x07, 0x06, 0x07, 0x06}, /* C C | A A | A A | A A */
   /*                                            */  /*     |     |     |     */
   {0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02}, /* C C | A A | A A | A A */
   /*                                            */  /*     |     |     |     */
   /*--------------------------------------------*/  /*-----------------------*/
   {0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01}, /* A A | A A | A A | A A */
   /*                                            */  /*     |     |     |     */
   {0x04, 0x05, 0x04, 0x05, 0x04, 0x05, 0x04, 0x05}, /* A A | A A | A A | A A */
   /*                                            */  /*     |     |     |     */
   /*--------------------------------------------*/  /*-----------------------*/
},{
   /*--------------------------------------------*/  /*-----------------------*/
   {0x07, 0x06, 0x07, 0x06, 0x07, 0x06, 0x07, 0x06}, /* C C | A A | A A | A A */
   {0x05, 0x04, 0x05, 0x04, 0x05, 0x04, 0x05, 0x04}, /* C C | A A | A A | A A */
   {0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02}, /* C C | A A | A A | A A */
   {0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00}, /* C C | A A | A A | A A */
   /*--------------------------------------------*/  /*-----------------------*/
   /*                                            */  /*     |     |     |     */
   /*                                            */  /*     |     |     |     */
   /*                                            */  /*     |     |     |     */
   /*                                            */  /*     |     |     |     */
   /*--------------------------------------------*/  /*-----------------------*/
}};
const uint8_t machines_machines_delta[TOTAL_LAYOUT_VERSIONS][ROW_MAX][COLUMN_MAX] = {{
   /*--------------------------------------------*/  /*-----------------------*/
   {0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE}, /* C C | A A | A A | A A */
   /*                                            */  /*     |     |     |     */
   {0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE}, /* C C | A A | A A | A A */
   /*                                            */  /*     |     |     |     */
   /*--------------------------------------------*/  /*-----------------------*/
   {0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02}, /* A A | A A | A A | A A */
   /*                                            */  /*     |     |     |     */
   {0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02}, /* A A | A A | A A | A A */
   /*                                            */  /*     |     |     |     */
   /*--------------------------------------------*/  /*-----------------------*/
},{
   /*--------------------------------------------*/  /*-----------------------*/
   {0xF9, 0xFB, 0xF9, 0xFB, 0xF9, 0xFB, 0xF9, 0xFB}, /* C C | A A | A A | A A */
   {0xFD, 0xFF, 0xFD, 0xFF, 0xFD, 0xFF, 0xFD, 0xFF}, /* C C | A A | A A | A A */
   {0x01, 0x03, 0x01, 0x03, 0x01, 0x03, 0x01, 0x03}, /* C C | A A | A A | A A */
   {0x05, 0x07, 0x05, 0x07, 0x05, 0x07, 0x05, 0x07}, /* C C | A A | A A | A A */
   /*--------------------------------------------*/  /*-----------------------*/
   /*                                            */  /*     |     |     |     */
   /*                                            */  /*     |     |     |     */
   /*                                            */  /*     |     |     |     */
   /*                                            */  /*     |     |     |     */
   /*--------------------------------------------*/  /*-----------------------*/
}};

machines_machine_t *machines_machines[MACHINES_MAX_MACHINES];
uint16_t machines_nof_machines;
bool machines_loaded = FALSE;
SemaphoreHandle_t machines_flags_changed_lock = NULL;
SemaphoreHandle_t machines_nof_lock = NULL;
SemaphoreHandle_t machines_loaded_lock = NULL;
SemaphoreHandle_t machines_need_recover_lock = NULL;
SemaphoreHandle_t machines_need_report_lock = NULL;
bool machine_need_recover[MACHINES_MAX_MACHINES] = {0};
bool machine_need_report[MACHINES_MAX_MACHINES] = {0};
bool machine_need_save[MACHINES_MAX_MACHINES] = {0};


/*** DECLARATIONS ***********************************************************************/
static uint8_t machines_metadata_update();
static uint8_t machines_metadata_get();
static void machines_fileName(char * name_buffer, uint8_t machine_no, const char * root_name);
static machines_machine_t * machine_load(uint8_t number);
static uint8_t machines_order(machines_machine_t *machine_pp);
static uint8_t machines_name_order(uint8_t name);
void offline_logs_load(void);
void offline_logs_ram(void);
static void rewind_ram(void);

/*** DEFINITIONS ************************************************************************/
void set_machines_nof(uint16_t new_number){
  if(sph_step_retries(machines_nof_lock) == TRUE){
    machines_nof_machines = new_number;
    sph_give(machines_nof_lock);
  } else {
    ESP_LOGW(TAG,"machine nof set lock untaken");
  }
}

uint16_t get_machines_nof(void){
  uint16_t machines_nof_handle = 0;
  if(sph_step_retries(machines_nof_lock) == TRUE){
    machines_nof_handle = machines_nof_machines;
    sph_give(machines_nof_lock);
  } else {
    ESP_LOGW(TAG,"machine nof set lock untaken");
  }
  return machines_nof_handle;
}
/*Initialization*/
void machines_init(void) {
    //machines_nof_machines = machines_metadata_get();
    sph_create(&machines_flags_changed_lock);
    sph_create(&machines_nof_lock);
    sph_create(&machines_loaded_lock);
    sph_create(&machines_need_recover_lock);
    sph_create(&machines_need_report_lock);
    set_machines_nof(machines_metadata_get());
    logs_init();
    /*DEBUG*/ ESP_LOGI(TAG, "Metadata [%d] machines", get_machines_nof());

}

void machines_prepare(void) {
    machines_machine_t * machine_p = NULL;
    uint16_t total_machines = get_machines_nof();
    for (uint8_t k = 0; k < total_machines; k++) {
        machine_p = machines_machines[k];
        machine_single_prepare(machine_p);
    }
}


void machine_single_prepare(machines_machine_t * machine_p) {
    //ESP_LOGI(TAG, "machine_single_prepare()");
    //ESP_LOGI(TAG, "pushing resource EVENTS");
    for (uint8_t j = 0; j < machine_p->deployment_info.nof_sequences[Resource][Event]; j++) {
        pin_seq_push(machine_p, &(machine_p->deployment_info.sequences[Resource][Event][j]));
    }

    //ESP_LOGI(TAG, "pushing Retrofit EVENTS");
    for (uint8_t j = 0; j < machine_p->deployment_info.nof_sequences[Retrofit][Event]; j++) {
        pin_seq_push(machine_p, &(machine_p->deployment_info.sequences[Retrofit][Event][j]));
    }

    //ESP_LOGI(TAG, "pushing current machine STATE");
    /*PUSH ALLOW OR DENY*/
    machines_hab_t is_machine_enabled;
    machine_flags_get(machine_p, ENABLED_FLAG, &is_machine_enabled);
    for (uint8_t j = 0; j < machine_p->deployment_info.nof_sequences[Retrofit][Action]; j++) {
        switch (is_machine_enabled) {
            case 0:
                if (!strcmp(machine_p->deployment_info.sequences[Retrofit][Action][j].name,
                        MACHINE_DISABLE_ACTION)) {
                    pin_seq_push(machine_p, &(machine_p->deployment_info.sequences[Retrofit][Action][j]));
                }
                break;
            case 1:
                if (!strcmp(machine_p->deployment_info.sequences[Retrofit][Action][j].name,
                        MACHINE_ENABLE_ACTION)) {
                    pin_seq_push(machine_p, &(machine_p->deployment_info.sequences[Retrofit][Action][j]));
                }
                break;
        }
    }
    //ESP_LOGI(TAG, "~machine_single_prepare()");
}

void machines_machine_enable(machines_machine_t * machine_p) {
    ESP_LOGI(TAG, "[EDIT MACHINE] Now enabled");
    machines_hab_t should_enable_machine = machine_enabled;
    machine_flags_set(machine_p, ENABLED_FLAG, &should_enable_machine);
}

void machines_machine_disable(machines_machine_t * machine_p) {
    ESP_LOGI(TAG, "[EDIT MACHINE] Now disabled");
    machines_hab_t should_disable_machine = machine_disabled;
    machine_flags_set(machine_p, ENABLED_FLAG, &should_disable_machine);
}

void machine_single_event_prepare(machines_machine_t *machine_p, machines_sequence_t* sequence_p) {
    pin_seq_push(machine_p, sequence_p);
}

bool machines_loaded_get(void){
  bool machines_loaded_handle = FALSE;
  if(sph_step_retries(&machines_loaded_lock) == TRUE){
    machines_loaded_handle = machines_loaded;
    sph_give(&machines_loaded_lock);
  } else{
    ESP_LOGE(TAG,"Machines get loaded lock untaken");
  }
  return machines_loaded_handle;
}

void machines_loaded_set(bool are_machines_loaded){
  if(sph_step_retries(&machines_loaded_lock) == TRUE){
    machines_loaded = are_machines_loaded;
    sph_give(&machines_loaded_lock);
  } else{
    ESP_LOGE(TAG,"Machines get loaded lock untaken");
  }
}

void machines_load(void) {
    if (machines_loaded == FALSE) {
        uint16_t total_machines = get_machines_nof();
        for (uint16_t k = 0; k < total_machines; k++) {
            ESP_LOGI(TAG, "Loading machine [%d]", k);
            machines_machines[k] = machine_load(k);
        }
        machines_loaded = TRUE;
    }
}

void machines_clear_next(void) {
  uint16_t total_machines = get_machines_nof();
  if (total_machines > 0) {
      machines_free(&(machines_machines[total_machines - 1]));
  } else {
      ESP_LOGE(TAG, "NO machines to erase");
  }
}

void machines_clear_all(void) {
  uint16_t total_machines = get_machines_nof();
    for (int16_t k = total_machines - 1; k >= 0; k--) {
        machines_free(&(machines_machines[k]));
    }
    set_machines_nof(0);
    machines_metadata_update();
}

static void machine_static_load(machines_machine_t * machine_p, uint8_t number) {
    char machine_file_name[20] = {0};
    char * machine_encoded_info = (char*) calloc(2 * sizeof (machines_machine_t), 1);
    machines_fileName(machine_file_name, number, DEFAULT_STATIC_MACHINE_FILE_NAME_ROOT);

    drv_nvs_get(machine_file_name, machine_encoded_info);
    b64_decode((const char *) machine_encoded_info, (unsigned char *) &(machine_p->deployment_info));
    free(machine_encoded_info);
}

static void machine_flags_load(machines_machine_t * machine_p, uint8_t number) {
    char machine_file_name[20] = {0};
    char * machine_encoded_info = (char*) calloc(2 * sizeof (machines_flags_t), 1);
    machines_fileName(machine_file_name, number, DEFAULT_DINAMIC_MACHINE_FILE_NAME_ROOT);

    drv_nvs_get(machine_file_name, machine_encoded_info);
    b64_decode((const char *) machine_encoded_info, (unsigned char *) &(machine_p->machine_flags));
    free(machine_encoded_info);
}

static machines_machine_t * machine_load(uint8_t number) {
    machines_machine_t * new_machine_space = NULL;
    machines_make(&new_machine_space);
    machine_flags_load(new_machine_space, number);
    machine_static_load(new_machine_space, number);


    ESP_LOGI(TAG, "Loaded machine of size [%d]", sizeof (machines_machine_t));
    machine_show(new_machine_space);
    return new_machine_space;
}

void machines_make(machines_machine_t **machine_pp) {
    *machine_pp = (machines_machine_t *) calloc(sizeof (machines_machine_t), 1);
    (*machine_pp)->deployment_info.nof_signals [Resource] = 0;
    (*machine_pp)->deployment_info.nof_signals [Retrofit] = 0;
    (*machine_pp)->deployment_info.nof_sequences[Resource][Action] = 0;
    (*machine_pp)->deployment_info.nof_sequences[Resource][Event] = 0;
    (*machine_pp)->deployment_info.nof_sequences[Retrofit][Action] = 0;
    (*machine_pp)->deployment_info.nof_sequences[Retrofit][Event] = 0;
}

void machine_show(machines_machine_t *machine_pp) {
    ESP_LOGI(TAG, "******************Printing machine*******************");
    ESP_LOGI(TAG, "Resource [%d]", machine_pp->deployment_info.resource);
    ESP_LOGI(TAG, "Activation status: [%s]", machine_pp->machine_flags.is_allowed == machine_enabled ? MACHINE_ENABLE_ACTION : MACHINE_DISABLE_ACTION);
    ESP_LOGI(TAG, "Running status: [%s]", machine_pp->machine_flags.is_idle == is_idle ? MACHINE_IDLE : MACHINE_RUNNING);
    ESP_LOGI(TAG, "port [%d] [%d]", machine_pp->deployment_info.signals[0]->port, machine_pp->deployment_info.signals[1]->port);
    ESP_LOGI(TAG, "Current action: [%s]", machine_pp->machine_flags.current_action_name);
    ESP_LOGI(TAG, "Nof signals [%d]", machine_pp->deployment_info.nof_signals[0]);
    ESP_LOGI(TAG, "Nof sequencies @ target 0 [%d]", machine_pp->deployment_info.nof_sequences[0][0]);
    ESP_LOGI(TAG, "*********resource sequencies*************");
    for (uint8_t k = 0; k < machine_pp->deployment_info.nof_sequences[Resource][Action]; k++) {
        ESP_LOGI(TAG, "Sequence [%d]: [%s]:[%s]", k, machine_pp->deployment_info.sequences[Resource][Action][k].name,
                machine_pp->deployment_info.sequences[Resource][Action][k].pattern);
        ESP_LOGI(TAG, "Triggered if status: %d", machine_pp->deployment_info.sequences[Resource][Action][k].enabling_condition);
    }
    for (uint8_t k = 0; k < machine_pp->deployment_info.nof_sequences[Resource][Event]; k++) {
        ESP_LOGI(TAG, "Sequence [%d]: [%s]:[%s]", k, machine_pp->deployment_info.sequences[Resource][Event][k].name,
                machine_pp->deployment_info.sequences[Resource][Event][k].pattern);
        ESP_LOGI(TAG, "Triggered if status: %d", machine_pp->deployment_info.sequences[Resource][Event][k].enabling_condition);
    }
    ESP_LOGI(TAG, "********retrofit sequencies************");
    for (uint8_t k = 0; k < machine_pp->deployment_info.nof_sequences[Retrofit][Action]; k++) {
        ESP_LOGI(TAG, "Sequence [%d]: [%s]:[%s]", k, machine_pp->deployment_info.sequences[Retrofit][Action][k].name,
                machine_pp->deployment_info.sequences[Retrofit][Action][k].pattern);
        ESP_LOGI(TAG, "Triggered if status: %d", machine_pp->deployment_info.sequences[Retrofit][Action][k].enabling_condition);
    }
    for (uint8_t k = 0; k < machine_pp->deployment_info.nof_sequences[Retrofit][Event]; k++) {
        ESP_LOGI(TAG, "Sequence [%d]: [%s]:[%s]", k, machine_pp->deployment_info.sequences[Retrofit][Event][k].name,
                machine_pp->deployment_info.sequences[Retrofit][Event][k].pattern);
        ESP_LOGI(TAG, "Triggered if status: %d", machine_pp->deployment_info.sequences[Retrofit][Event][k].enabling_condition);
    }

    ESP_LOGI(TAG, "****~~~~**********Printing machine*******~~~~******");
}

static uint8_t machines_metadata_update() {
    char number_of_machines[5] = {0};
    if (!drv_nvs_check(MACHINE_METADATA_NVS_KEY)) {
    uint16_t total_machines = get_machines_nof();
        snprintf(number_of_machines, 6, "%03d", total_machines);
        /*DEBUG*/ESP_LOGI(TAG, "New saved number of machines: %s", number_of_machines);
        drv_nvs_set(MACHINE_METADATA_NVS_KEY, number_of_machines);
    }
    return SUCCESS;
}

static uint8_t machines_metadata_get() {
    char number_of_machines[5] = {0};
    if (!drv_nvs_check(MACHINE_METADATA_NVS_KEY)) {
        drv_nvs_get(MACHINE_METADATA_NVS_KEY, number_of_machines);
        return atoi(number_of_machines);
    } else {
        ESP_LOGI(TAG, "Creating space for machine metadata on NVS");
        drv_nvs_set(MACHINE_METADATA_NVS_KEY, "000");
        return 0;
    }
}

void machine_free_archive(const char * machine_static_file_name, const char * machine_dynamic_file_name, bool is_last) {
    char machine_static_reorder_name[20] = {0};
    char machine_dynamic_reorder_name[20] = {0};

    if (drv_nvs_check(machine_static_file_name) == SUCCESS) {
        drv_nvs_clear(machine_static_file_name);
        drv_nvs_clear(machine_dynamic_file_name);
    } else {
        //machines_metadata_update();
        ESP_LOGW(TAG, "Machine not saved yet");
        return;
    }

    if (!is_last) {
        uint16_t total_machines = get_machines_nof();
        machines_fileName(machine_static_reorder_name, total_machines - 1,
                DEFAULT_STATIC_MACHINE_FILE_NAME_ROOT);
        machines_fileName(machine_dynamic_reorder_name, total_machines - 1,
                DEFAULT_DINAMIC_MACHINE_FILE_NAME_ROOT);
        drv_nvs_rename(machine_static_reorder_name, machine_static_file_name);
        drv_nvs_rename(machine_dynamic_reorder_name, machine_dynamic_file_name);
    }

    //machines_metadata_update();
}

void machines_clear_RAM(void) {
    uint16_t total_machines = get_machines_nof();
    for (uint8_t k = 0; k < total_machines; k++) {
        if (machines_machines[k] != NULL) {
            free(machines_machines[k]);
        } else {
            ESP_LOGW(TAG, "NUll machine [%d]?", k);
        }
    }
    machines_loaded = FALSE;
    set_machines_nof(0);
}

void machine_free_from_ram(uint8_t machine_idx){
  uint16_t total_machines = get_machines_nof();
  if(sph_step_retries(&machines_flags_changed_lock) == TRUE){
      free(machines_machines[machine_idx]);
      machines_machines[machine_idx] = machines_machines[total_machines - 1];
      machines_machines[total_machines - 1] = NULL;
    sph_give(&machines_flags_changed_lock);
  } else {
    ESP_LOGE(TAG,"Machine freeing lock untaken");
  }
}

void machines_free(machines_machine_t **machine_pp) {
    char machine_static_file_name[20] = {0};
    char machine_dynamic_file_name[20] = {0};
    uint16_t total_machines = get_machines_nof();
    uint8_t machine_idx = machines_order(*machine_pp);
    ESP_LOGI(TAG,"Initial nof machines [%d]",total_machines);
    machines_fileName(machine_static_file_name, machine_idx, DEFAULT_STATIC_MACHINE_FILE_NAME_ROOT);
    machines_fileName(machine_dynamic_file_name, machine_idx, DEFAULT_DINAMIC_MACHINE_FILE_NAME_ROOT);
    bool is_last = (machine_idx == (total_machines - 1)) ?
            1 : 0;
    ESP_LOGI(TAG, "[unloading] unloading machine resource:[%d] idx:[%d] file name:[%s] [%s]", (*machine_pp)->deployment_info.resource,
            machine_idx, machine_static_file_name, is_last ? " LAST" : " NOT LAST");
    machine_free_archive(machine_static_file_name, machine_dynamic_file_name, is_last);
    machine_free_from_ram(machine_idx);

    total_machines--;
    set_machines_nof(total_machines);
    machines_metadata_update();
}

static uint8_t machines_order(machines_machine_t *machine_pp) {
    uint16_t total_machines = get_machines_nof();
    for (uint8_t k = 0; k < total_machines; k++) {
        if (machines_machines[k]->deployment_info.resource == machine_pp->deployment_info.resource) {
            return k;
        }
    }
    return FAILURE;
}

static uint8_t machines_name_order(uint8_t name){
  uint16_t total_machines = get_machines_nof();
  for (uint8_t k = 0; k < total_machines; k++) {
      if (machines_machines[k]->deployment_info.resource == name) {
          return k;
      }
  }
  return FAILURE;
}

static void machines_fileName(char * name_buffer, uint8_t machine_no, const char * root_name) {
    snprintf(name_buffer, strlen(root_name) + 3 + 2,
            "%s%03d", root_name, machine_no);
}

uint8_t machine_dynamic_dump(machines_machine_t *machine_p, uint16_t machine_idx) {
    char * machine_encoded_info = (char*) calloc(2 * sizeof (machines_flags_t), 1);
    char machine_file_name[20] = {0};
    machines_fileName(machine_file_name, machine_idx, DEFAULT_DINAMIC_MACHINE_FILE_NAME_ROOT);
    ESP_LOGI(TAG, "[dynamic] Clearing OLD [%s] machine information", machine_file_name);
    b64_encode((const unsigned char *) &(machine_p->machine_flags), machine_encoded_info, sizeof (machines_flags_t));

    uint8_t result = drv_nvs_set(machine_file_name, machine_encoded_info);
    free(machine_encoded_info);

    return result;
}

uint8_t machine_static_dump(machines_machine_t *machine_p, uint16_t machine_idx) {
    char * machine_encoded_info = (char*) calloc(2 * sizeof (machines_static_information_t), 1);
    char machine_file_name[20] = {0};
    machines_fileName(machine_file_name, machine_idx, DEFAULT_STATIC_MACHINE_FILE_NAME_ROOT);

    if (drv_nvs_check(machine_file_name) == SUCCESS) {
        ESP_LOGI(TAG, "[static] Clearing OLD [%s] machine information", machine_file_name);
        ///drv_nvs_clear(machine_file_name);
    }

    b64_encode((const unsigned char *) &(machine_p->deployment_info), machine_encoded_info, sizeof (machines_static_information_t));
    uint8_t result = drv_nvs_set(machine_file_name, machine_encoded_info);
    free(machine_encoded_info);

    return result;
}

uint8_t machine_encode_dump(machines_machine_t *machine_p, uint16_t machine_idx) {
    if (machine_static_dump(machine_p, machine_idx) != SUCCESS) {
        return FAILURE;
    }
    if (machine_dynamic_dump(machine_p, machine_idx) != SUCCESS) {
        return FAILURE;
    }
    return SUCCESS;
}

uint8_t machine_update(machines_machine_t *machine_p) {
  if (sph_step_retries(&machines_flags_changed_lock) == TRUE) {
    machine_p->deployment_info.flags_changed = TRUE;
    sph_give(&machines_flags_changed_lock);
  } else {
    ESP_LOGE(TAG,"(Machines update) flag untaken");
  }
  return SUCCESS;
}

uint8_t machines_save(machines_machine_t *machine_p) {
    uint16_t total_machines = get_machines_nof();
    if (total_machines > MACHINES_MAX_MACHINES - 1) {
        ESP_LOGE(TAG,"Machines overflow!");
    } // TODO: Handle overflows & fragmentation
    ESP_LOGI(TAG, "Saving machine of size [%d]", sizeof (machines_machine_t));
    machines_machines[total_machines] = machine_p;
    if (machine_encode_dump(machine_p, total_machines) != SUCCESS) {
        return FAILURE;
    }
    machine_show(machine_p);
    total_machines++;
    set_machines_nof(total_machines);
    machines_metadata_update();
    return SUCCESS;
}

uint8_t machines_layout(char *row, char *column, uint8_t dir, uint8_t retrofit) {
    uint8_t row_idx = 0xFF;
    uint8_t column_idx = 0xFF;

    for (uint8_t idx = 0; idx < ROW_MAX; idx++) {
        if (!strcmp(machines_rows[idx], row)) {
            row_idx = idx;
        }
    }

    if (row_idx == 0xFF) {
        return FAILURE;
    }

    for (uint8_t idx = 0; idx < COLUMN_MAX; idx++) {
        if (!strcmp(machines_columns[idx], column)) {
            column_idx = idx;
        }
    }

    if (column_idx == 0xFF) {
        return FAILURE;
    }

    uint8_t port = machines_assemble_port(row_idx,
            column_idx, dir, retrofit);

    return port;
}

uint8_t machines_assemble_port(uint8_t row_idx, uint8_t column_idx, uint8_t dir, uint8_t retrofit) {
    uint8_t assembled_adapter = machines_adapters_mapping[current_layout_version][row_idx][column_idx]
            + (retrofit ? machines_adapters_delta[current_layout_version][row_idx][column_idx] : 0x00);

    uint8_t assembled_port = machines_machines_mapping [current_layout_version][row_idx][column_idx]
            + (retrofit ? machines_machines_delta [current_layout_version][row_idx][column_idx] : 0x00);
    assembled_port = assembled_port & 0x07;

            assembled_port |= ((dir << DIRECTION_BIT) & DIRECTION_MASK);

    return (assembled_adapter | assembled_port);
}

uint8_t machine_check_existance(uint8_t resource){
  uint16_t total_machines = get_machines_nof();
  for (uint8_t i = 0; i < total_machines; i++) {
      if (machines_machines[i]->deployment_info.resource == resource) {
          return SUCCESS;
      }
  }
  return FAILURE;
}

uint8_t machines_machine(machines_machine_t **machine_pp, uint8_t resource) {
    uint16_t total_machines = get_machines_nof();
    ESP_LOGI(TAG,"[%d] amount of machines",total_machines);
    for (uint8_t i = 0; i < total_machines; i++) {
        if (machines_machines[i]->deployment_info.resource == resource) {
            *machine_pp = machines_machines[i];
            return SUCCESS;
        }
    }
    *machine_pp = NULL;
    return FAILURE;
}

uint8_t machines_target(machines_machine_t *machine_p, char *name) {
    for (uint8_t i = 0; i < machine_p->deployment_info.nof_sequences[Resource][Action]; i++) {
        if (!strcmp(machine_p->deployment_info.sequences[Resource][Action][i].name, name)) {
            return Resource;
        }
    }

    for (uint8_t i = 0; i < machine_p->deployment_info.nof_sequences[Retrofit][Action]; i++) {
        if (!strcmp(machine_p->deployment_info.sequences[Retrofit][Action][i].name, name)) {
            return Retrofit;
        }
    }

    return FAILURE;
}

uint8_t machines_signal(machines_machine_t *machine_p, machines_target_t target, char name) {
    for (uint8_t i = 0; i < machine_p->deployment_info.nof_signals[target]; i++) {
        //ESP_LOGI(TAG, "target:[%d] signal:[0x%X] name:[%c]", target, i, machine_p->signals[target][i].name);
        if (machine_p->deployment_info.signals[target][i].name == name) {
            return machine_p->deployment_info.signals[target][i].port;
        }
    }

    return FAILURE;
}

uint8_t machines_sequence(machines_machine_t *machine_p, machines_target_t target, machines_channel_t channel, char *name) {
    ESP_LOGI(TAG,"target [%d] channel [%d], %p",target,channel,machine_p);
    for (uint8_t i = 0; i < machine_p->deployment_info.nof_sequences[target][channel]; i++) {
        ESP_LOGI(TAG, "sequence [0x%X]", i);
        ESP_LOGI(TAG, "name [%s]", machine_p->deployment_info.sequences[target][channel][i].name);
        if (!strcmp(machine_p->deployment_info.sequences[target][channel][i].name, name)) {
            return i;
        }
    }

    return FAILURE;
}

uint8_t machines_check_port_free(uint8_t port_toCheck) {
    uint16_t total_machines = get_machines_nof();
    for (uint8_t k = 0; k < total_machines; k++) {
        for (uint8_t j = 0; j < machines_machines[k]->deployment_info.nof_signals[Resource]; j++) {
            if (port_toCheck == machines_machines[k]->deployment_info.signals[Resource][j].port) {
                return SUCCESS;
            }
        }
        for (uint8_t j = 0; j < machines_machines[k]->deployment_info.nof_signals[Retrofit]; j++) {
            if (port_toCheck == machines_machines[k]->deployment_info.signals[Retrofit][j].port) {
                return SUCCESS;
            }
        }
    }
    return FAILURE;
}

void machine_flags_get(machines_machine_t *machine_p, machines_flag_type_t flag_type, void * flag_destination) {
  if(sph_step_retries(&machines_flags_changed_lock) == TRUE){
    switch (flag_type) {
        case IDLE_FLAG:
            *((machines_idle_t*) flag_destination) = machine_p->machine_flags.is_idle;
            break;
        case ENABLED_FLAG:
            *((machines_hab_t*) flag_destination) = machine_p->machine_flags.is_allowed;
            break;
        case CONTROL_FLAG:
            *((machines_control_t*) flag_destination) = machine_p->machine_flags.transaction_control;
            break;
        case CURRENT_ACTION_FLAG:
            *((char **) flag_destination) = machine_p->machine_flags.current_action_name;
            break;
        case SAME_ACTION_FLAG:
            *((machines_new_sequence_t*) flag_destination) = machine_p->machine_flags.sequence_changed;
            break;
        default:
            flag_destination = NULL;
    }
    sph_give(&machines_flags_changed_lock);
  } else {
    ESP_LOGE(TAG,"Flags set lock untaken");
  }
}

void machine_flags_set(machines_machine_t *machine_p, machines_flag_type_t flag_type, void * flag_destination) {
  if(sph_step_retries(&machines_flags_changed_lock) == TRUE){
    //ESP_LOGE(TAG, "now setting flag [%d] to ", flag_type);
    switch (flag_type) {
        case IDLE_FLAG:
            //ESP_LOGI(TAG, "IDLE: [%d]", *((machines_idle_t*) flag_destination));
            machine_p->machine_flags.is_idle = *((machines_idle_t*) flag_destination);
            break;
        case ENABLED_FLAG:
            //ESP_LOGI(TAG, "ENABLE: [%d]", *((machines_hab_t*) flag_destination));
            machine_p->machine_flags.is_allowed = *((machines_hab_t*) flag_destination);
            break;
        case CONTROL_FLAG:
            //ESP_LOGI(TAG, "CONTROL: [%d]", *((machines_control_t*) flag_destination));
            machine_p->machine_flags.transaction_control = *((machines_control_t*) flag_destination);
            break;
        case CURRENT_ACTION_FLAG:
            strlcpy(machine_p->machine_flags.current_action_name, (char *) flag_destination, strlen(flag_destination) + 1);
            //ESP_LOGI(TAG, "ACTION: [%s]", machine_p->machine_flags.current_action_name);
            break;
        case SAME_ACTION_FLAG:
            //ESP_LOGI(TAG, "SAME ACTION: [%d]", *((machines_new_sequence_t*) flag_destination));
            machine_p->machine_flags.sequence_changed = *((machines_new_sequence_t*) flag_destination);
            break;
        default:
            flag_destination = NULL;
    }
    sph_give(&machines_flags_changed_lock);
  } else {
    ESP_LOGE(TAG,"Flags set lock untaken");
  }
}

void fix_all_resources_cmd(void){
  ESP_LOGI(TAG,"fixing all machines");
  uint16_t total_machines = get_machines_nof();
  for(uint8_t k = 0; k < total_machines; k++){
    machines_clear_all_status(machines_machines[k]);
  }
}

hub_error_t machines_clear_all_status(machines_machine_t *machine_p){
    ESP_LOGW(TAG,"Clearing Machine");
    machines_hab_t set_disabled_state = machine_disabled;
    machine_flags_set(machine_p, ENABLED_FLAG, &set_disabled_state);
    machines_idle_t set_idle_state = is_idle;
    machine_flags_set(machine_p, IDLE_FLAG, &set_idle_state);
    machines_control_t set_no_control = control_none;
    machine_flags_set(machine_p, CONTROL_FLAG, &set_no_control);
    machine_flags_clear_action(machine_p);
    machines_new_sequence_t set_no_new_action = same_sequence;
    machine_flags_set(machine_p, SAME_ACTION_FLAG, &set_no_new_action);

    machine_update(machine_p);
    return HUB_OK;
}

void machine_flags_clear_action(machines_machine_t *machine_p) {
    char clear_action[MACHINES_MAX_CHARACTERS];
    for (uint8_t k = 0; k < MACHINES_MAX_CHARACTERS; k++) {
        clear_action[k] = 0;
    }
    strlcpy(clear_action, DEFAULT_MACHINE_ACTION, MACHINES_MAX_CHARACTERS);
    machine_flags_set(machine_p, CURRENT_ACTION_FLAG, &(clear_action));
}

bool machine_action_matches_event(machines_machine_t *machine_p, machines_sequence_t *sequence_p) {
    char * machine_action;
    machine_flags_get(machine_p, CURRENT_ACTION_FLAG, &machine_action);
    if (!strcmp(machine_action, sequence_p->name)) {
        ESP_LOGI(TAG, "[%s] matches current action name", sequence_p->channel ? "Action" : "Event");
        return true;
    } else {
        ESP_LOGW(TAG, "Unmatching action name with event / action");
    }
    return false;
}

bool machine_flags_dispatcher(machines_machine_t *machine_p, machines_sequence_t *sequence_p) {
  machines_hab_t is_machine_enabled;
  machines_idle_t is_machine_idle;
  machines_control_t is_hub_control;

  machine_flags_get(machine_p, IDLE_FLAG, &is_machine_idle);
  machine_flags_get(machine_p, ENABLED_FLAG, &is_machine_enabled);
  machine_flags_get(machine_p, CONTROL_FLAG, &is_hub_control);

  if(sequence_p->channel == Event){ /*If sequence is event*/
    if(!strcmp(sequence_p->status,
      execution_status[triggerStatus])){
        if(is_machine_enabled == machine_disabled){
          return IGNORE; /*<! dont allow triggers while deny = true*/
        }
        if(is_hub_control != control_none){/*<! Will this affect the RE-CYCLE pattern?*/
          ESP_LOGW(TAG,"Control already set. Not allowing action");
          return IGNORE;
        }
      } else {
        return DONT_IGNORE; /*never ignore success events yet*/
      }
    } else if (is_machine_idle == is_running) {
      ESP_LOGE(TAG,"Machine is running, not receiving actions");
      return IGNORE; /*Dont perform any action while running*/
    }
    return DONT_IGNORE;
}

#define SEQUENCE_IS_TRIGGER     1
#define SEQUENCE_IS_NOT_TRIGGER 0

void machine_control_task_dispatcher(machines_machine_t *machine_p,
  machines_sequence_t *sequence_p, bool should_i_log) {
    if (sequence_p->channel == Event) {
      if (!strcmp(sequence_p->status, action_status[triggerStatus])) {
        ESP_LOGI(TAG, "Updating control to HUB and action to [%s]", sequence_p->name);
        machines_control_t set_hub_control = control_hub;
        machine_flags_set(machine_p, CONTROL_FLAG, (void*) &set_hub_control);
        machine_flags_set(machine_p, CURRENT_ACTION_FLAG, sequence_p->name);
        log_transaction_step(machine_p, should_i_log);
        if(should_i_log){
          machines_new_sequence_t new_seq_started = new_sequence;
          machine_flags_set(machine_p, SAME_ACTION_FLAG, &new_seq_started);
        }
        /*IF EVENT TRIGGER RECEIVED, ACTION -> ACTION, CONTROL -> HUB*/
      } else if (!strcmp(sequence_p->status, action_status[successStatus])) {
        machines_control_t free_control = control_none;
        machines_idle_t now_idle = is_idle;
        ESP_LOGI(TAG, "Transaction [%s] finished, idle and no control.", sequence_p->name);
        machine_flags_set(machine_p, IDLE_FLAG, (void*) &now_idle);
        machine_flags_set(machine_p, CONTROL_FLAG, (void*) &free_control);
        log_transaction_step(machine_p, should_i_log);
        /*IF EVENT SUCCESS RECEIVED, ACTION -> NONE, CONTROL -> NONE, IDLE = TRUE*/
      }
    } else {
      if (!strcmp(sequence_p->name, MACHINE_ENABLE_ACTION)) {
        ESP_LOGI(TAG, "action [%s] running, machine enabled, no action", sequence_p->name);
        machines_machine_enable(machine_p);
        machine_flags_clear_action(machine_p);
        log_transaction_step(machine_p, should_i_log);
        machines_new_sequence_t get_if_new_seq;
        machine_flags_get(machine_p, SAME_ACTION_FLAG, &get_if_new_seq);
        if(get_if_new_seq == new_sequence){ //if this was arised by a trigger and the action went offline alltogether, unset the recover flag
          machines_new_sequence_t finished_new_seq = same_sequence;
          machine_flags_set(machine_p, SAME_ACTION_FLAG, &finished_new_seq);
          //machine_need_recover_unset(machine_p);
        }
      } else if (!strcmp(sequence_p->name, MACHINE_DISABLE_ACTION)) {
        ESP_LOGI(TAG, "action [%s] running, machine disabled", sequence_p->name);
        machines_machine_disable(machine_p);
        log_transaction_step(machine_p, should_i_log);
      } else if (!strcmp(sequence_p->status, action_status[runningStatus])) {
        ESP_LOGI(TAG, "Transaction [%s] running, status running", sequence_p->name);
        machines_idle_t now_running = is_running;
        machine_flags_set(machine_p, IDLE_FLAG, (void*) &now_running);
        log_transaction_step(machine_p, should_i_log);
        /*IF EVENT RUNNING RECEIVED, IDLE -> RUNNING*/
      }
    }
    machine_update(machine_p);
  }

uint8_t update_machine_flags(machines_machine_t *machine_p,
        machines_sequence_t *sequence_p, bool should_i_log) {
    uint8_t operation_result = HUB_OK;
    if (!strcmp(sequence_p->status, action_status[runningStatus])) {
        ESP_LOGI(TAG,"Received action [%s] running, control HUB",sequence_p->name);
        machines_control_t check_ctrl;
        machine_flags_get(machine_p, CONTROL_FLAG, (void*) (&check_ctrl));
        if(!check_ctrl){
          machines_control_t txn_control = control_txn;
          machine_flags_set(machine_p, CONTROL_FLAG, (void*) (&txn_control));
        } else {
          ESP_LOGW(TAG,"Machine was TRIGGERED, no changing control");
        }
        machine_flags_set(machine_p, CURRENT_ACTION_FLAG, (void*) (&(sequence_p->name)));
        log_transaction_step(machine_p, should_i_log);
    }
    machine_update(machine_p);
    return operation_result;
}

void single_machine_update(machines_machine_t *machine_p, uint16_t machine_idx) {
    if (sph_step_retries(&machines_flags_changed_lock) == TRUE) {
        if (machine_p->deployment_info.flags_changed) {
            machine_dynamic_dump(machine_p, machine_idx);
            ESP_LOGI(TAG, "[PUPD] updated resource %d", machine_p->deployment_info.resource);
            machine_p->deployment_info.flags_changed = FALSE;
        }
        sph_give(&machines_flags_changed_lock);
    } else {
        ESP_LOGE(TAG, "[Flag UPD] lock untaken");
    }

}

void machines_update_changed(void) {
    ESP_LOGI(TAG, "Updating machines");
    //print_usages();
    uint16_t total_machines = get_machines_nof();
    for (uint8_t k = 0; k < total_machines; k++) {
      if(machines_machines[k] != NULL){
        single_machine_update(machines_machines[k], k);
        vTaskDelay(200 / portTICK_RATE_MS);
      } else {
        ESP_LOGW(TAG,"Machine removed during update");
      }
    }
}

#define FOREVER while (1)

SemaphoreHandle_t machines_log_semaphore = NULL;
uint8_t ram_offline_logs = 0;
uint8_t offline_log_page = 0;
hub_offline_log offline_logs[LOGS_PER_PAGE];

#define STATS_TICKS         pdMS_TO_TICKS(1000)
void machines_update_tsk(void *pvParameters) {
    FOREVER{
        if (machines_loaded_get()) {
            machines_update_changed();
            set_ram_usage(uxTaskGetStackHighWaterMark(NULL), machines_update_task);
            //print_usages();
        }
        vTaskDelay(10000 / portTICK_RATE_MS);
    }
}

void logs_init(void) {
    /*<! in the future we should save current RAM PAGE of logs when error is asserted and recover here*/
    offline_logs_load();
    offline_logs_ram();

    vSemaphoreCreateBinary(machines_log_semaphore);
    if (machines_log_semaphore == NULL) {
        ESP_LOGE(TAG, "[Logs] unable to create semaphore");
    }
}

void offline_logs_load(void) {
    char total_nof_logs[6] = {0};
    if (!drv_nvs_check(LOGS_METADATA_NVS_KEY)) {
        drv_nvs_get(LOGS_METADATA_NVS_KEY, total_nof_logs);
        offline_log_page = atoi(total_nof_logs);
    } else {
        ESP_LOGW(TAG, "Creating space for logs metadata on NVS");
        drv_nvs_set(LOGS_METADATA_NVS_KEY, "0000");
        offline_log_page = 0;
        return;
    }
}

void offline_logs_ram(void) {
    char total_nof_logs[6] = {0};
    if (!drv_nvs_check(LOGS_RAM_METADATA)) {
        drv_nvs_get(LOGS_RAM_METADATA, total_nof_logs);
        ram_offline_logs = atoi(total_nof_logs);
        if (ram_offline_logs) {
            rewind_ram();
            need_monitor_dump = TRUE;
            ESP_LOGI(TAG,"now re enabling monitor dump");
            ESP_LOGI(TAG,"RAM OFFLINE LOGS REMAINING: [%d]",ram_offline_logs);
        }
    } else {
        ESP_LOGW(TAG, "Creating space for RAM logs metadata on NVS");
        drv_nvs_set(LOGS_RAM_METADATA, "0000");
        ram_offline_logs = 0;
        return;
    }
}

static void logs_fileName(char * name_buffer, uint8_t log_no, bool is_ram_log) {
    if (!is_ram_log) {
        snprintf(name_buffer, strlen(LOG_ROOT_NAME) + 4 + 2,
                "%s%04d", LOG_ROOT_NAME, log_no);
    } else {
        snprintf(name_buffer, strlen(LOG_ROOT_NAME) + 2,
                "%s", RAM_LOG_ROOT);
    }
}

uint8_t log_encode_dump(uint16_t amount_of_logs, bool is_ram_log) {
    char * log_encoded_info = (char*) calloc(2 * LOGS_PER_PAGE, sizeof (hub_offline_log));
    char log_file_name[20] = {0};
    logs_fileName(log_file_name, offline_log_page, is_ram_log);

    b64_encode((const unsigned char *) offline_logs, log_encoded_info, sizeof (hub_offline_log) * LOGS_PER_PAGE);
    uint8_t result = drv_nvs_set(log_file_name, log_encoded_info);
    free(log_encoded_info);
    return result;
}

void log_metadata_update(void) {
    char number_of_pages[6] = {0};
    if (!drv_nvs_check(LOGS_METADATA_NVS_KEY)) {
        snprintf(number_of_pages, 6, "%04d", offline_log_page);
        /*DEBUG*/ESP_LOGI(TAG, "number of log pages: [%s]", number_of_pages);
        drv_nvs_set(LOGS_METADATA_NVS_KEY, number_of_pages);
    }
    return;
}

void log_ram_metadata_save(void) {
    char number_of_logs[6] = {0};
    if (!drv_nvs_check(LOGS_RAM_METADATA)) {
        snprintf(number_of_logs, 6, "%04d", ram_offline_logs);
        /*DEBUG*/ESP_LOGI(TAG, "number of log items: [%s]", number_of_logs);
        drv_nvs_set(LOGS_RAM_METADATA, number_of_logs);
    }
    //ram_offline_logs = 0;
    return;
}

static void rotate_logs(void) {
    if (offline_log_page >= MAX_LOGS_PAGES) {
        ESP_LOGE(TAG, "Cannot keep saving logs. Pissing first logs. Aborting");
        offline_log_page = 0;
    }
    log_encode_dump(offline_log_page, 0);
    ESP_LOGI(TAG, "Rewinding page [%d]", offline_log_page);
    offline_log_page++;
    log_metadata_update();
}

static void rewind_ram(void) {
    char log_file_name[20] = {0};
    char * log_encoded_info = (char*) calloc(2 * LOGS_PER_PAGE, sizeof (hub_offline_log));
    logs_fileName(log_file_name, offline_log_page - 1, 1);

    drv_nvs_get(log_file_name, log_encoded_info);
    b64_decode((const char *) log_encoded_info, (unsigned char *) offline_logs);
    free(log_encoded_info);
}

static void rewind_log(void) {
    char log_file_name[20] = {0};
    char * log_encoded_info = (char*) calloc(2 * LOGS_PER_PAGE, sizeof (hub_offline_log));
    logs_fileName(log_file_name, offline_log_page - 1, 0);

    drv_nvs_get(log_file_name, log_encoded_info);
    b64_decode((const char *) log_encoded_info, (unsigned char *) offline_logs);
    free(log_encoded_info);
    offline_log_page--;
    log_metadata_update();
}

void assemble_log(machines_machine_t *machine_p, hub_offline_log * target_log) {
    target_log->resource = machine_p->deployment_info.resource;

    machines_idle_t get_idle_state;
    machine_flags_get(machine_p, IDLE_FLAG, &get_idle_state);
    target_log->is_idle = get_idle_state;

    machines_hab_t get_habilitation;
    machine_flags_get(machine_p, ENABLED_FLAG, &get_habilitation);
    target_log->is_allowed = get_habilitation;

    machines_control_t get_control;
    machine_flags_get(machine_p, CONTROL_FLAG, &get_control);
    target_log->transaction_control = get_control;

    machines_new_sequence_t get_if_new_sequence;
    machine_flags_get(machine_p, SAME_ACTION_FLAG, &get_if_new_sequence);
    target_log->sequence_changed = get_if_new_sequence;

    char * action_name;
    machine_flags_get(machine_p, CURRENT_ACTION_FLAG, &action_name);
    strlcpy(target_log->name, action_name, MACHINES_MAX_CHARACTERS);
}

void machine_need_report_set(uint8_t machine_name){
  uint8_t machine_idx = machines_name_order(machine_name);
  ESP_LOGI(TAG,"Machine [%d] at idx [%d] needs REPORT",machine_name
  ,machine_idx);
  if(sph_step_retries(&machines_need_report_lock) == TRUE){
    machine_need_report[machine_idx] = 1;
    sph_give(&machines_need_report_lock);
  } else {
    ESP_LOGE(TAG,"[RECOVER SET] Lock untaken");
  }
}

bool any_machine_need_report_get(void){
  for(uint16_t k = 0; k < MACHINES_MAX_MACHINES ; k++){
    if(machine_need_report_get(k)){
      return true;
    }
  }
  return false;
}

machines_machine_t * machine_report(void){
  for(uint16_t k = 0; k < MACHINES_MAX_MACHINES ; k++){
    if(machine_need_report_get(k)){
      machine_need_report_unset(k);
      return machines_machines[k];
    }
  }
  return NULL;
}

void machine_need_report_unset(uint8_t machine_idx){
  if(sph_step_retries(&machines_need_recover_lock) == TRUE){
    machine_need_report[machine_idx] = 0;
    sph_give(&machines_need_recover_lock);
  } else {
    ESP_LOGE(TAG,"[RECOVER SET] Lock untaken");
  }
}

bool machine_need_report_get(uint8_t machine_idx){
  bool get_handle=FALSE;
  if(sph_step_retries(&machines_need_report_lock) == TRUE){
    get_handle = machine_need_report[machine_idx];
    sph_give(&machines_need_report_lock);
  } else {
    ESP_LOGE(TAG,"[RECOVER SET] Lock untaken");
  }
  return get_handle;
}

void machine_need_recover_set(machines_machine_t *machine_p){
  uint8_t machine_idx = machines_order(machine_p);
  ESP_LOGI(TAG,"Machine [%d] at idx [%d] needs recover",machine_p->deployment_info.resource
  ,machine_idx);
  if(sph_step_retries(&machines_need_recover_lock) == TRUE){
    machine_need_recover[machine_idx] = 1;
    sph_give(&machines_need_recover_lock);
  } else {
    ESP_LOGE(TAG,"[RECOVER SET] Lock untaken");
  }
}

void machine_need_recover_unset(machines_machine_t *machine_p){
  uint8_t machine_idx = machines_order(machine_p);
  ESP_LOGI(TAG,"Machine [%d] at idx [%d] doesnt need recover",machine_p->deployment_info.resource
  ,machine_idx);
  if(sph_step_retries(&machines_need_recover_lock) == TRUE){
    machine_need_recover[machine_idx] = 0;
    sph_give(&machines_need_recover_lock);
  } else {
    ESP_LOGE(TAG,"[RECOVER SET] Lock untaken");
  }
}

bool machine_need_recover_get(uint8_t machine_idx){
  bool get_handle=FALSE;
  if(sph_step_retries(&machines_need_recover_lock) == TRUE){
    get_handle =machine_need_recover[machine_idx];
    sph_give(&machines_need_recover_lock);
  } else {
    ESP_LOGE(TAG,"[RECOVER SET] Lock untaken");
  }
  return get_handle;
}

bool unlog_machine(hub_offline_log * unlogged_machine) {
  for(uint8_t k=0; k < MACHINES_MAX_MACHINES; k++){
    if(machine_need_recover_get(k)){
      ESP_LOGI(TAG, "Unlogging MACHINE [%d]", k);
      machines_machine_t *machine_p = machines_machines[k];
      ESP_LOGI(TAG, "Unlogging resource [%d]", machine_p->deployment_info.resource);
      assemble_log(machine_p, unlogged_machine);
      machine_need_recover[k] = FALSE;
      return TRUE;
    }
  }
  return FALSE;
}

void unlog_transaction_step(hub_offline_log * last_log) {
    if (sph_step_retries(&machines_log_semaphore) == pdTRUE) {
        ESP_LOGI(TAG, "[Unlogging] UNlogging step [%d]", ram_offline_logs);
        last_log->is_allowed = offline_logs[ram_offline_logs].is_allowed;
        last_log->is_idle = offline_logs[ram_offline_logs].is_idle;
        last_log->resource = offline_logs[ram_offline_logs].resource;
        last_log->transaction_control = offline_logs[ram_offline_logs].transaction_control;
        last_log->resource = offline_logs[ram_offline_logs].resource;
        last_log->sequence_changed = offline_logs[ram_offline_logs].sequence_changed;
        strlcpy(last_log->name, offline_logs[ram_offline_logs].name, MACHINES_MAX_CHARACTERS);

        ESP_LOGI(TAG, "[Unlogging] Logging [%d] for machine [%d]"
                "with action [%s]", ram_offline_logs, last_log->resource,
                last_log->name);
        ESP_LOGI(TAG, "Allowed [%d], idle [%d],control [%d]", last_log->is_allowed,
                last_log->is_idle, last_log->transaction_control);

        if(ram_offline_logs>0){
          ram_offline_logs--;
          log_ram_metadata_save();
        } else if(offline_log_page >= 1){
          rewind_log();
          ram_offline_logs = LOGS_PER_PAGE - 1;
          log_ram_metadata_save();
        } else {
          ESP_LOGI(TAG,"Finished logging");
          need_monitor_dump = FALSE;
        }
      sph_give(&machines_log_semaphore);
    } else {
        ESP_LOGE(TAG, "[unlogging] Unable to log, locked");
    }
}

//uint8_t get_remaining_offline_logs(void) {
//    if (sph_step_retries(&machines_log_semaphore) == pdTRUE) {
//        if (ram_offline_logs == 0) {
//            if (offline_log_page >= 1) {
//                ESP_LOGI(TAG, "[Unlogging] Offline logs remaining [%d]", offline_log_page);
//                rewind_log();
//                ram_offline_logs = LOGS_PER_PAGE - 1;
//            } else {
//                ESP_LOGI(TAG, "[Unlogging] Finished unlogging, log pages: [%d]",offline_log_page);
//                need_monitor_dump = FALSE;
//                log_ram_metadata_save();
//            }
//        }
//        xSemaphoreGive(machines_log_semaphore);
//        return ram_offline_logs;
//
//    } else {
//        ESP_LOGE(TAG, "[remaining] Unable to log, locked");
//    }
//    return ram_offline_logs;
//}

uint8_t get_remaining_offline_logs(void) {
  uint8_t offline_logs_handle = 0;
  if (sph_step_retries(&machines_log_semaphore) == pdTRUE) {
    if(ram_offline_logs > 0){
      offline_logs_handle = 1;
    } else if(offline_log_page > 0){
      offline_logs_handle = 1;
    } else {
      offline_logs_handle = 0;
    }
    sph_give(&machines_log_semaphore);
  } else {
    ESP_LOGE(TAG, "[remaining] Unable to log, locked");
  }
  return offline_logs_handle;
}

void logs_flush_ram(void) {
    log_encode_dump(offline_log_page, 1);
    log_ram_metadata_save();
    ram_offline_logs = 0;
}

void cmd_reset_logs(void){
  ram_offline_logs = 0;
  log_ram_metadata_save();
  offline_log_page = 0;
  log_metadata_update();
}

void log_transaction_step(machines_machine_t *machine_p, bool should_i_log) {
    if (!should_i_log) {
        ESP_LOGW(TAG, "not logging - online");
        return;
    }
    if (sph_step_retries(&machines_log_semaphore) == pdTRUE) {
        need_monitor_dump = TRUE;

        hub_offline_log new_entry;
        assemble_log(machine_p, &new_entry);

        if (ram_offline_logs > LOGS_PER_PAGE - 1) {
            ram_offline_logs = 0;
            log_ram_metadata_save();
            rotate_logs();
            log_metadata_update();
        }

        ESP_LOGI(TAG, "[LOG] Logging [%d] for machine [%d]"
                "with action [%s]", ram_offline_logs, machine_p->deployment_info.resource,
                new_entry.name);
        ESP_LOGI(TAG, "Allowed [%d], idle [%d],control [%d]", new_entry.is_allowed,
                new_entry.is_idle, new_entry.transaction_control);

        offline_logs[ram_offline_logs].is_allowed = new_entry.is_allowed;
        offline_logs[ram_offline_logs].is_idle = new_entry.is_idle;
        offline_logs[ram_offline_logs].transaction_control = new_entry.transaction_control;
        offline_logs[ram_offline_logs].resource = machine_p->deployment_info.resource;
        offline_logs[ram_offline_logs].sequence_changed = new_entry.sequence_changed;
        strlcpy(offline_logs[ram_offline_logs].name, new_entry.name, MACHINES_MAX_CHARACTERS);
        ram_offline_logs++;
        log_ram_metadata_save();
        xSemaphoreGive(machines_log_semaphore);
    } else {
        ESP_LOGE(TAG, "[logging] Unable to log, locked");
    }
}
