/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "MFSMEffects.h"
#include "esp_log.h"
#include "connection.h"
#include "registration.h"
#include "monitoring.h"
#include "machines.h"
#include "launcher.h"
#include "hubware.h"
#include "params.h"
#include "validation.h"
#include "drv_nvs.h"

static const char *TAG = "[mfsm-effect]";
#define TRUE 1

uint8_t erro_Meffect(effectParam *param) {
    ESP_LOGE(TAG, "Error on Master FSM");
#ifdef __TESTING_MASTERFSM__
    ESP_LOGE(TAG, "from [%d] to [%d] with event [%d]", param->previous_status, param->current_status, param->current_event);
#endif
    return 0;
}

uint8_t noop_Meffect(effectParam *param) {
    ESP_LOGI(TAG, "triggering {noop}");
    //ESP_LOGI(TAG, "~triggering {noop}");
    return 0;
}

uint8_t rgstr_Meffect(effectParam *param) {
    ESP_LOGI(TAG, "triggering {registration}");
    check_activation_flags[registation_check_flag] = 1;
    //ESP_LOGI(TAG, "~triggering {registration}");

    return 0;
}

uint8_t cnnctn_Meffect(effectParam *param) {
    ESP_LOGI(TAG, "triggering {connection}");
    //ESP_LOGI(TAG, "~triggering {connection}");
    return 0;
}

uint8_t vldtn_Meffect(effectParam *param) {
    ESP_LOGI(TAG, "triggering {validation}");
    //ESP_LOGI(TAG, "~triggering {validation}");
    return 0;
}

uint8_t engag_Meffect(effectParam *param) {
    ESP_LOGI(TAG, "triggering {engaging}");
    check_activation_flags[registation_check_flag] = 0;
    check_activation_flags[internet_check_flag] = 1;
    run_activation_flags[mqtt_run_flag] = 1;
    check_activation_flags[cloud_check_flag] = 1;

    setDesired(running_mode);
    run_activation_flags[launcher_run_flag] = 1;
    check_activation_flags[update_check_flag] = 1;
    //ESP_LOGI(TAG, "~triggering {engaging}");
    return 0;
}

uint8_t disen_Meffect(effectParam *param) {
    ESP_LOGI(TAG, "triggering {disengaging}");
    run_activation_flags[monitor_run_flag] = 0;
    run_activation_flags[deploy_run_flag] = 0;
    run_activation_flags[execute_run_flag] = 0;
    run_activation_flags[console_run_flag] = 0;
    run_activation_flags[fallback_run_flag] = 0;

    setDesired(no_one_running);
    run_activation_flags[launcher_run_flag] = 1;
    //ESP_LOGI(TAG, "~triggering {disengaging}");

    return 0;
}

uint8_t trgRunning(effectParam *param) {
    ESP_LOGI(TAG, "triggering {running}");

    run_activation_flags[launcher_run_flag] = 0;
    run_activation_flags[monitor_run_flag] = 1;
    run_activation_flags[deploy_run_flag] = 1;
    run_activation_flags[execute_run_flag] = 1;
    run_activation_flags[console_run_flag] = 1;
    data_to_report[running_mode_data] = 1;
    set_hub_report();
    //ESP_LOGI(TAG, "~triggering {running}");
    return 0;
}

uint8_t updat_Meffect(effectParam *param) {
    ESP_LOGI(TAG, "triggering {update}");
    machines_update_changed();
    machines_clear_RAM();

    run_activation_flags[launcher_run_flag] = 0;
    run_activation_flags[update_run_flag] = 1;
    //ESP_LOGI(TAG, "~triggering {update}");
    return 0;
}

uint8_t apeng_Meffect(effectParam * param) {
    ESP_LOGI(TAG, "triggering {AP engage}");
    run_activation_flags[launcher_run_flag] = 0;

    check_activation_flags[internet_check_flag] = 0;
    run_activation_flags[mqtt_run_flag] = 0;
    check_activation_flags[cloud_check_flag] = 0;
    connection_engage_ap();
    //ESP_LOGI(TAG, "~triggering {AP engage}");
    return 0;
}

uint8_t apdng_Meffect(effectParam * param) {
    ESP_LOGI(TAG, "triggering {AP disengage}");
    validation_failure_cb();
    connection_re_engage_sta();
    check_activation_flags[registation_check_flag] = 1;
    //ESP_LOGI(TAG, "~triggering {AP disengage}");
    return 0;
}

uint8_t trgAp_effect(effectParam *param) {
    ESP_LOGI(TAG, "triggering {AP mode}");
    run_activation_flags[launcher_run_flag] = 0;
    //ESP_LOGI(TAG, "~triggering {AP mode}");
    return 0;
}

uint8_t fbdis_Meffect(effectParam *param) {
    ESP_LOGI(TAG, "triggering {fallback_disengage}");
    disen_Meffect(param);
    drv_nvs_set(HUB_LAST_MODE_KEY, HUB_NORMAL_MODE_KEY);
    //ESP_LOGI(TAG, "~triggering {fallback_disengage}");
    return 0;
}

uint8_t trgfb_Meffect(effectParam *param) {
    ESP_LOGI(TAG, "triggering {fallback engage}");
    drv_nvs_set(HUB_LAST_MODE_KEY, HUB_FALLBACK_MODE_KEY);
    check_activation_flags[registation_check_flag] = 0;
    run_activation_flags[execute_run_flag] = 0;
    check_activation_flags[internet_check_flag] = 1;
    run_activation_flags[mqtt_run_flag] = 1;
    check_activation_flags[cloud_check_flag] = 1;

    setDesired(fallback_mode);
    run_activation_flags[launcher_run_flag] = 1;
    check_activation_flags[update_check_flag] = 1;
    data_to_report[running_mode_data] = 1;
    //ESP_LOGI(TAG, "~triggering {fallback engage}");
    return 0;
}

uint8_t fbSet_Meffect(effectParam *param) {
    ESP_LOGI(TAG, "triggering {fbSet}");
    run_activation_flags[launcher_run_flag] = 0;

    run_activation_flags[monitor_run_flag] = 1;
    run_activation_flags[deploy_run_flag] = 1;
    run_activation_flags[console_run_flag] = 1;
    run_activation_flags[fallback_run_flag] = 1;
    set_hub_report();
    //ESP_LOGI(TAG, "~triggering {fbSet}");

    return 0;
}

uint8_t trgid_Meffect(effectParam *param) {
    ESP_LOGI(TAG, "triggering {trgid}");
    check_activation_flags[registation_check_flag] = 0;
    run_activation_flags[execute_run_flag] = 0;
    check_activation_flags[internet_check_flag] = 1;
    run_activation_flags[mqtt_run_flag] = 1;
    check_activation_flags[cloud_check_flag] = 1;

    setDesired(idle_mode);
    run_activation_flags[launcher_run_flag] = 1;
    check_activation_flags[update_check_flag] = 1;
    //ESP_LOGI(TAG, "~triggering {trgid}");
    return 0;

}

uint8_t idset_Meffect(effectParam *param) {
    ESP_LOGI(TAG, "triggering {idset}");
    run_activation_flags[launcher_run_flag] = 0;

    run_activation_flags[monitor_run_flag] = 1;
    run_activation_flags[console_run_flag] = 1;
    //ESP_LOGI(TAG, "~triggering {idset}");
    return 0;
}

uint8_t iddis_Meffect(effectParam *param) {
    check_activation_flags[registation_check_flag] = 1;
    return 0;
}
