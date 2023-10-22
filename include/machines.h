  //////////////////////////////////////////////////////////////////////////////////////////
//     __  ___ ___    ______ __  __ ____ _   __ ______ _____                            //
//    /  |/  //   |  / ____// / / //  _// | / // ____// ___/                            //
//   / /|_/ // /| | / /    / /_/ / / / /  |/ // __/   \__ \                             //
//  / /  / // ___ |/ /___ / __  /_/ / / /|  // /___  ___/ /                             //
// /_/  /_//_/  |_|\____//_/ /_//___//_/ |_//_____/ /____/                              //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __MACHINES_H__
#define __MACHINES_H__

#include <stdint.h>

#include "esp_system.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hub_error.h"

#define MAX_ACTIONS 128
#define MAX_RESULTS 128
#define MAX_EVENTS   64

#define MACHINES_MAX_MACHINES 32

#define MACHINE_FLAGS_BATCH     1

#define MACHINES_MAX_PORTS      2
#define MACHINES_MAX_TARGETS    2
#define MACHINES_MAX_SIGNALS    2
#define MACHINES_MAX_SEQUENCES  4
#define MACHINES_MAX_CHARACTERS 9
#define MACHINES_MAX_EVENTS     8
#define MACHINES_EVENT_SIZE     5

#define ROW_1       "A"
#define ROW_2       "B"
#define ROW_3       "C"
#define ROW_4       "D"
#define ROW_MAX     4

#define COLUMN_1    "rose"
#define COLUMN_2    "rouge"
#define COLUMN_3    "orange"
#define COLUMN_4    "jaune"
#define COLUMN_5    "vert"
#define COLUMN_6    "cyan"
#define COLUMN_7    "bleu"
#define COLUMN_8    "violet"
#define COLUMN_MAX  8

#define ODIR  0
#define IDIR  1

#define DIRECTION_BIT  3
#define DIRECTION_MASK 0x08

/**/
#define DEFAULT_STATIC_MACHINE_FILE_NAME_ROOT   "mch-"
#define DEFAULT_DINAMIC_MACHINE_FILE_NAME_ROOT  "flg-"
#define MACHINE_METADATA_NVS_KEY                "MCHMTD"

#define MACHINE_ENABLE_ACTION   "allow"
#define MACHINE_DISABLE_ACTION  "deny"
#define MACHINE_RUNNING         "running"
#define MACHINE_IDLE            "idle"
#define DEFAULT_MACHINE_ACTION  "none"

/*Physical pin definitions*/
typedef enum {
    pA = 0,
    pB = 1
} pin_reg_t;

typedef enum {
    p0 = 0,
    p1 = 1,
    p2 = 2,
    p3 = 3,
    p4 = 4,
    p5 = 5,
    p6 = 6,
    p7 = 7,
} pin_bit_t;

typedef enum {
    odir = 0,
    idir = 1
} pin_dir_t;

typedef struct {
    pin_reg_t ioreg;
    pin_bit_t iobit;
    pin_dir_t iodir;
} pin_spec_t;

/*Port definitions and types*/
typedef enum {
    Action = 0,
    Event = 1,
    NOF_SEQUENCE_CHANNELS,
} machines_channel_t;

typedef enum {
    Resource = 0,
    Retrofit = 1,
    NOF_MACHINE_TARGETS,
} machines_target_t;

typedef struct {
    char name;        // Single character signals
    uint8_t port;     // up to 8 adapters with 8 input and 8 output each
} machines_signal_t;  // 2B per signal

/*Action definitions and types*/
typedef enum {
    on_enabled_disabled,
    on_enabled,
    enabling_condition_dont_care,
    total_valid_enabled
} enabling_condition_t;

typedef enum {
    on_idle,
    on_idle_running,
    idle_condition_dont_care,
    total_valid_idle
} idle_condition_t;

typedef enum {
    cleared = 0,
    idle = 1,
    running = 2,
    broken = 3,
    NOF_MACHINE_STATUS,
} machines_status_t;

typedef struct {
    char name [MACHINES_MAX_CHARACTERS]; //  8B per name
    char pattern[MACHINES_MAX_EVENTS*MACHINES_EVENT_SIZE]; // 40B per pattern
    char status [MACHINES_MAX_CHARACTERS]; //  8B per status
    uint8_t target; // (almost) bool
    enabling_condition_t    enabling_condition;
    idle_condition_t        idle_condition;
    machines_channel_t      channel; //Action or Event
} machines_sequence_t; // 49B per sequence

/* machine Status definitions and types*/
typedef enum {
    machine_disabled = 0,
    machine_enabled = 1
} machines_hab_t;

typedef enum {
    is_running = 0,
    is_idle
} machines_idle_t;

typedef enum {
    control_none = 0,
    control_hub,
    control_txn,
    TOTAL_CONTROL_FLAGS
} machines_control_t;

typedef enum {
  same_sequence = 0,
  new_sequence = 1
} machines_new_sequence_t;

/*Machine flag to control editing*/
typedef enum {
    IDLE_FLAG,
    ENABLED_FLAG,
    CONTROL_FLAG,
    CURRENT_ACTION_FLAG,
    SAME_ACTION_FLAG,
    TOTAL_FLAGS
} machines_flag_type_t;

typedef struct {
    machines_hab_t is_allowed;
    machines_idle_t is_idle;
    machines_control_t transaction_control;
    machines_new_sequence_t sequence_changed;
    char current_action_name[MACHINES_MAX_CHARACTERS];
} machines_flags_t;

#define IGNORE              0
#define DONT_IGNORE         1

#define ALLOWED_AND_IDLE    3
#define NOT_ALLOWED         1
#define RUNNING             0

typedef struct {
    uint8_t resource;
    machines_signal_t signals [NOF_MACHINE_TARGETS][MACHINES_MAX_SIGNALS];
    uint8_t nof_signals [NOF_MACHINE_TARGETS];
    machines_sequence_t sequences[NOF_MACHINE_TARGETS][NOF_SEQUENCE_CHANNELS][MACHINES_MAX_SEQUENCES];
    uint8_t nof_sequences[NOF_MACHINE_TARGETS][NOF_SEQUENCE_CHANNELS];
    bool flags_changed;
} machines_static_information_t;

typedef struct {
  /*machines_static_information_t is the deployment FIXED information - Ports,
  actions, signals, name*/
    machines_static_information_t deployment_info;
  /*machines_flags_t is the information that changes on operation - availability,
  current action*/
    machines_flags_t machine_flags;
} machines_machine_t; // 401B per machine

/*Log type definition - this is used to log a screencapture of each moment of
operation. It should have all the necessary information to reconstruct a working
scheme of a resource*/
typedef struct {
  /*Logged resource name*/
    uint8_t resource;
  /*Logged idle status*/
    machines_idle_t is_idle;
    machines_hab_t is_allowed;
    machines_control_t transaction_control;
  /*his flag turns ON if the machine is OFFLINE and a trigger action happened
  it is necessary to distinguish between a short disconnection within an action,
  and a long disconnection*/
    machines_new_sequence_t sequence_changed;
    char name[MACHINES_MAX_CHARACTERS];
} hub_offline_log;

#define LOGS_PER_PAGE           5
#define MAX_LOGS_PAGES          10

extern const char *machines_rows[ROW_MAX];
extern const char *machines_columns[COLUMN_MAX];


/*machines_update_tsk Task loop to update flags*/
void machines_update_tsk(void *pvParameters);
/*machines_init Init logs(dump saved pages + last RAM page)*/
void machines_init(void);

/***
 *                _   _
 *               | | (_)
 *      __ _  ___| |_ _  ___  _ __  ___
 *     / _` |/ __| __| |/ _ \| '_ \/ __|
 *    | (_| | (__| |_| | (_) | | | \__ \
 *     \__,_|\___|\__|_|\___/|_| |_|___/
 *
 *
 */
void machines_prepare(void);
void machine_single_prepare(machines_machine_t * machine_p);
void machine_single_event_prepare(machines_machine_t *machine_p, machines_sequence_t* sequence_p);
void machines_machine_enable(machines_machine_t * machine_p);
void machines_machine_disable(machines_machine_t * machine_p);
/*machine_control_task_dispatcher update the machine flags according to the detected new finished event*/
void machine_control_task_dispatcher(machines_machine_t *machine_p, machines_sequence_t *sequence_p, bool should_i_log);


/***
*      __ _
*     / _| |
*    | |_| | __ _  __ _ ___
*    |  _| |/ _` |/ _` / __|
*    | | | | (_| | (_| \__ \
*    |_| |_|\__,_|\__, |___/
*                  __/ |
*                 |___/
*/
/*machine_flags_set set flags on machine*/
void machine_flags_set(machines_machine_t *machine_p, machines_flag_type_t flag_type, void * flag_destination);
/*machine_flags_get set flags on machine*/
void machine_flags_get(machines_machine_t *machine_p, machines_flag_type_t flag_type, void * flag_destination);
void machine_flags_clear_action(machines_machine_t *machine_p);
/*machine_flags_dispatcher allor or prevent the next action according to the machine flags*/
bool machine_flags_dispatcher(machines_machine_t *machine_p, machines_sequence_t *sequence_p);
void fix_all_resources_cmd(void);
hub_error_t machines_clear_all_status(machines_machine_t *machine_p);

void machines_update_changed(void);
uint8_t machine_update(machines_machine_t *machine_p);
uint8_t update_machine_flags(machines_machine_t *machine_p, machines_sequence_t *sequence_p, bool should_i_log);


/***
*                                                __ _           _
*                                               / _| |         | |
*     _ __ ___  ___  ___  _   _ _ __ ___ ___   | |_| | __ _ ___| |__
*    | '__/ _ \/ __|/ _ \| | | | '__/ __/ _ \  |  _| |/ _` / __| '_ \
*    | | |  __/\__ \ (_) | |_| | | | (_|  __/  | | | | (_| \__ \ | | |
*    |_|  \___||___/\___/ \__,_|_|  \___\___|  |_| |_|\__,_|___/_| |_|
*
*
*/
/*machines_loaded setter and getter of machines loaded variables*/
bool machines_loaded_get(void);
void machines_loaded_set(bool are_machines_loaded);
/*machines_make Initialize machine space*/
void machines_make(machines_machine_t **machine_pp);
void machines_free(machines_machine_t **machine_pp);
uint8_t machines_save(machines_machine_t *machine_p);
void machines_load(void);
void machines_clear_all(void);
void machines_clear_RAM(void);
void machines_clear_next(void);

/***
 *                          _     _                                                  _
 *                         | |   (_)                                                | |
 *     _ __ ___   __ _  ___| |__  _ _ __   ___    ______   _ __ ___ _ __   ___  _ __| |_
 *    | '_ ` _ \ / _` |/ __| '_ \| | '_ \ / _ \  |______| | '__/ _ \ '_ \ / _ \| '__| __|
 *    | | | | | | (_| | (__| | | | | | | |  __/           | | |  __/ |_) | (_) | |  | |_
 *    |_| |_| |_|\__,_|\___|_| |_|_|_| |_|\___|           |_|  \___| .__/ \___/|_|   \__|
 *                                                                 | |
 *                                                                 |_|
 */
/*machine_need_report_set/UNSET/get sets machine report flag for posting on next report event of monitoring*/
bool any_machine_need_report_get(void);
machines_machine_t * machine_report(void);
void machine_need_report_set(uint8_t machine_name);
void machine_need_report_unset(uint8_t machine_idx);
bool machine_need_report_get(uint8_t machine_idx);

/*Logs*/
void assemble_log(machines_machine_t *machine_p, hub_offline_log * target_log);
/***
 *     _
 *    | |
 *    | |     ___   __ _ ___    ______    _ __ ___  ___ _____   _____ _ __
 *    | |    / _ \ / _` / __|  |______|  | '__/ _ \/ __/ _ \ \ / / _ \ '__|
 *    | |___| (_) | (_| \__ \            | | |  __/ (_| (_) \ V /  __/ |
 *    \_____/\___/ \__, |___/            |_|  \___|\___\___/ \_/ \___|_|
 *                  __/ |
 *                 |___/
 */

 /*machine_need_recover_set/UNSET/get sets machine recover flag for posting on next recover event of monitoring*/
 void machine_need_recover_set(machines_machine_t *machine_p);
 void machine_need_recover_unset(machines_machine_t *machine_p);
 bool machine_need_recover_get(uint8_t machine_idx);

/***
 *     _                                      _
 *    | |                                    | |
 *    | |     ___   __ _ ___    ______     __| |_   _ _ __ ___  _ __
 *    | |    / _ \ / _` / __|  |______|   / _` | | | | '_ ` _ \| '_ \
 *    | |___| (_) | (_| \__ \            | (_| | |_| | | | | | | |_) |
 *    \_____/\___/ \__, |___/             \__,_|\__,_|_| |_| |_| .__/
 *                  __/ |                                      | |
 *                 |___/                                       |_|
 */
void logs_init(void);
void offline_logs_ram(void);
void unlog_transaction_step(hub_offline_log * last_log);
void log_transaction_step(machines_machine_t *machine_p, bool should_i_log);
uint8_t get_remaining_offline_logs(void);
bool unlog_machine(hub_offline_log * unlogged_machine);
/*Clear logs from RAM, save them to FLASH*/
void logs_flush_ram(void);
/*cmd_reset_logs resets log status rewinding everything back to 0*/
void cmd_reset_logs(void);

/***
 *     _              _
 *    | |            | |
 *    | |_ ___   ___ | |___
 *    | __/ _ \ / _ \| / __|
 *    | || (_) | (_) | \__ \
 *     \__\___/ \___/|_|___/
 *
 *
 */
 /*Get machine port given row, column, direction and if the retrofit port is desired
 or the resource port is.*/
 uint8_t machines_layout(char *row, char *column, uint8_t dir, uint8_t retrofit);
 uint8_t machines_assemble_port(uint8_t row_idx, uint8_t column_idx,uint8_t dir, uint8_t retrofit);
 /*machines_machine search for machine resource and return it in machine_pp*/
 uint8_t machines_machine(machines_machine_t **machine_pp, uint8_t resource);
 /*machines_machine search for machine resource and return success or failure for existance*/
 uint8_t machine_check_existance(uint8_t resource);
 /*Get the target (resource or retrofit) of the action the NAME is NAME of the action*/
 uint8_t machines_target(machines_machine_t *machine_p, char *name);
 uint8_t machines_signal(machines_machine_t *machine_p, machines_target_t target, char name);
 uint8_t machines_sequence(machines_machine_t *machine_p, machines_target_t target, machines_channel_t channel, char *name);
 void machine_show(machines_machine_t *machine_pp);
 uint8_t machines_check_port_free(uint8_t port_toCheck);


#endif
