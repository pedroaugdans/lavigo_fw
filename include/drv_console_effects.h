/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   drv_console_effects.h
 * Author: independent contractor
 *
 * Created on November 6, 2019, 6:18 AM
 */

#ifndef DRV_CONSOLE_EFFECTS_H
#define DRV_CONSOLE_EFFECTS_H
#include "params.h"

void cmd_connection_clear(void);
void cmd_registration_delete(void);
void cmd_machines_clear(void);
void cmd_execution_test(void);
void cmd_monitor_test(void);
void cmd_deply_test(void);
void cmd_erase_machine(void);
void cmd_set_layout(layout_version_t new_layout);
void cmd_erase_layout(void);
void cmd_fix_machines(void);
void cmd_soft_reset(void);
void cmd_logs_reset(void);

#endif /* DRV_CONSOLE_EFFECTS_H */
