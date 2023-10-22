
/*
 * LAVIGO PROJECT CODE
 * Each line should be prefixed with  *
 */

#include "drv_console.h"
#include "drv_console_effects.h"

#include "esp_log.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include <string.h>
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_fat.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "launcher.h"
#include "params.h"

#define FOREVER while(1)

/*** VARIABLES **************************************************************************/

static const char *TAG = "[DRV-CONSOLE]";

typedef enum {
    CNL_START_CMD,
    CNL_ERASE_KEYS,
    CNL_ERASE_CNCTN_KEYS_CMD,
    CNL_ERASE_MACHINES,
    CNL_SHOW_MACHINES,
    CNL_EXC_TEST,
    CNL_MTR_TEST,
    CNL_DPL_TEST,
    CNL_DPL_DLT,
    CNL_RAM_USE,
    CNL_SET_LY1,
    CNL_SET_LY2,
    CNL_DEL_LY,
    CNL_FIX_ALL,
    CNL_SOFT_RST,
    CNL_LOGS_RST,
    CNL_HELPME_CMD,
    CNL_NOF_CMDS
} console_command_t;

static const char *console_commands[] = {
    STRTCN_IDX,
    DLTKYS_IDX,
    CLRCNT_IDX,
    CLRALL_IDX,
    SHWMCH_IDX,
    EXCTST_IDX,
    MNTTST_IDX,
    DPLTST_IDX,
    MCHDLT_IDX,
    RAMUSE_IDX,
    SETLY1_IDX,
    SETLY2_IDX,
    CLRLYT_IDX,
    MCHFIX_IDX,
    SFTRST_IDX,
    LOGRST_IDX,
    "helpme"
};

/*** DECLARATIONS ***********************************************************************/
static void handle_next_cmd(void);
static void console_dispatch_command(console_command_t next_cmd);
void console_task(void *param);
int cmd_dummy(int argc, char** argv);

static void task_run(void);
static void task_idle(void);
static bool task_disengage(void);
static bool task_isDisengage(void);
static bool task_isEngage(void);
static bool task_isRun(void);
static bool task_engage(void);
static void task_init(void);
static void task_sleep(uint8_t ticks);

/*** DEFINITIONS ************************************************************************/

const esp_console_cmd_t lavigo_commands[] = {
    {"cmd_dummy", "cmd line debug", "dontUse", cmd_dummy, NULL}
};

const esp_console_config_t console_config = {
    .max_cmdline_args = 8,
    .max_cmdline_length = 256
};

void consoleInit() {
    ESP_ERROR_CHECK(esp_console_init(&console_config));
    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     *single line.
     */
    linenoiseSetMultiLine(1);
    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    //esp_console_cmd_register(&lavigo_commands[0]);
    esp_console_register_help_command();
    int probe_status = linenoiseProbe();
    if (probe_status) { /* zero indicates success */
        printf("\n"
                "Your terminal application does not support escape sequences.\n"
                "Line editing and history features are disabled.\n"
                "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
    }
    xTaskCreatePinnedToCore(&console_task, "console_task", 2048, NULL, 5, NULL, 1);
}

void consoleExec() {
    /* Get a line using linenoise.
     *The line is returned when ENTER is pressed.
     */
    while (1) {
        char *line = linenoise(HUB_PROMPT_STR);
        if (line == NULL) { /* Ignore empty lines */
            continue;
        }

        /* Try to run the command */
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        } else if (err == ESP_ERR_INVALID_ARG) {
            // command was empty
        } else if (err == ESP_OK && ret != ESP_OK) {
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
        } else if (err != ESP_OK) {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
    }

}

int cmd_dummy(int argc, char** argv) {
    return 0;
}

void console_task(void *param) {
    consoleExec();
}

void provisory_console_task(void *param) {
    task_init();
    FOREVER{
        if (task_isEngage()) {
            bool check_running_flag = task_engage();
            if (check_running_flag) {
                run_confirmation_flag[console_current] = 1;
            }

        } else if (task_isRun()) {
            task_run();

        } else if (task_isDisengage()) {
            bool uncheck_running_flag = task_disengage();
            if (uncheck_running_flag) {
                run_confirmation_flag[console_current] = 0;
            }

        } else {
            task_idle();
        }
        task_sleep(2);}
}

static void task_init(void) {

}

static bool task_engage(void) {
    run_confirmation_flag[console_current] = 1;
    return true;
}

static void task_run(void) {
    handle_next_cmd();
}

static bool task_disengage(void) {
    run_confirmation_flag[console_current] = 0;
    return true;
}

static bool task_isRun(void) {
    return run_activation_flags[console_run_flag];
}

static bool task_isEngage(void) {
    return engage_activation_flags[console_engage_flag];
}

static bool task_isDisengage(void) {
    return disengage_activation_flags[console_disengage_flag];
}

static void task_idle(void) {
    ESP_LOGI(TAG, "idle()");
    task_sleep(5);
}

static void task_sleep(uint8_t ticks) {
    set_ram_usage(uxTaskGetStackHighWaterMark(NULL), console_task_idx);
    vTaskDelay(suspendedThreadDelay[console_flag] * ticks / portTICK_RATE_MS);
}

static void handle_next_cmd(void) {
    console_command_t next_cmd;
    next_cmd = (console_command_t) drv_uart_next(console_commands, CNL_NOF_CMDS);
    if (next_cmd < CNL_NOF_CMDS) {
        console_dispatch_command(next_cmd);
        ESP_LOGI(TAG, "got console command [%d]", next_cmd);
    }
}

static void console_dispatch_command(console_command_t next_cmd) {
    switch (next_cmd) {
        case CNL_START_CMD:
            break;
        case CNL_ERASE_KEYS:
            cmd_registration_delete();
            break;
        case CNL_ERASE_CNCTN_KEYS_CMD:
            cmd_connection_clear();
            break;
        case CNL_ERASE_MACHINES:
            cmd_machines_clear();
            break;
        case CNL_SHOW_MACHINES:
            ESP_LOGW(TAG, "Unimplemented");
            break;
        case CNL_EXC_TEST:
            cmd_execution_test();
            break;
        case CNL_MTR_TEST:
            cmd_monitor_test();
            break;
        case CNL_DPL_TEST:
            cmd_deply_test();
            break;
        case CNL_DPL_DLT:
            cmd_erase_machine();
            break;
        case CNL_RAM_USE:
            print_usages();
            break;
        case CNL_SET_LY1:
            cmd_set_layout(version_5_0);
            break;
        case CNL_SET_LY2:
            cmd_set_layout(version_5_1);
            break;
        case CNL_DEL_LY:
            cmd_erase_layout();
            break;
        case CNL_FIX_ALL:
            cmd_fix_machines();
            break;
        case CNL_SOFT_RST:
            cmd_soft_reset();
            break;
        case CNL_LOGS_RST:
            cmd_logs_reset();
            break;
        case CNL_HELPME_CMD:
            ESP_LOGE(TAG, "Theres no help for you.");
            break;

        default:
            break;
    }
}
