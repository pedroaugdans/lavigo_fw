/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include "drv_console_effects.h"
#include  "deployment.h"

#include "connection.h"
#include "deployment.h"
#include "machines.h"
#include "execution.h"
#include "monitoring.h"
#include "registration.h"
#include "lavigoMasterFSM.h"

void cmd_connection_clear(void) {
    connection_clearKeys();
}

void cmd_registration_delete(void) {
    delete_all_keys();
}

void cmd_machines_clear(void) {
    machines_clear_all();
}

void cmd_execution_test(void) {
    trigger_test();
}

void cmd_monitor_test(void) {
    dump_recovery_test();
}

void cmd_deply_test(void) {
    deployment_machine_4_test();
}

void cmd_erase_machine(void) {
    deployment_erase_last();
}

void cmd_set_layout(layout_version_t new_layout) {
    switch (new_layout) {
        case version_5_0:
            registration_set_layout_version(VERSION_0_5_0);
            break;
        case version_5_1:
            registration_set_layout_version(VERSION_0_5_1);
            break;
        default:
            ESP_LOGE("console effect", "error");
    }
    set_layout_version(new_layout);
    fsm_q_evt(flashOk_Mevent);
}
void cmd_erase_layout(void){
    delete_layout();
}

void cmd_fix_machines(void){
  fix_all_resources_cmd();
}

void cmd_soft_reset(void){
  soft_reset();
}

void cmd_logs_reset(void){
  cmd_reset_logs();
}
