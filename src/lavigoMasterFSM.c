/*
 * LAVIGO PROJECT CODE
 *
 *
 * Each line should be prefixed with  *
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lavigoMasterFSM.h"
#include "launcher.h"
#include "monitoring.h"
#include "hubware.h"
#include "MFSMEffects.h"
#include "drv_locks.h"
#include "params.h"
#include "esp_log.h"

#define TRUE 1
#define FALSE 0

masterFSM_state masterState = flash_mstate;
static const char *TAG = "[MFSM]";
FSMQueue masterEventQ[MAXFSMEVTS];
static effectParam railingParam;
static uint8_t FSMEvtIdx = 0;
static uint8_t currentFSMEvtIdx = 0;
SemaphoreHandle_t master_evt_lock = NULL;


masterFSM_state nextState[total_Mstate][total_Mevent] = {
    /*stat/	/evt*/    /*flashOk*/   /*certsload*/ /*certsmiss*//*connsuccess*//*connfailed*//*validsuccess*//*validFailed*//*Engaged*//*Disengaged*/   /*trgup*/      /*apSet*/   /*trgfb*/     /*fbset*/     /*trg idle*/   /*idle set*/  /*errorEv*/
    /*flash*/       { regis_mstate, engag_mstate, regis_mstate, flash_mstate, flash_mstate, flash_mstate, flash_mstate, runn_mstate,  error_Mstate,   error_Mstate, error_Mstate, error_Mstate, error_Mstate, error_Mstate, error_Mstate, error_Mstate}, /*flash*/
    /*registration*/{ regis_mstate, engag_mstate, regis_mstate, regis_mstate, regis_mstate, regis_mstate, regis_mstate, runn_mstate,  error_Mstate,   error_Mstate, regis_mstate, enfba_Mstate, regis_mstate, enidl_Mstate, regis_mstate, error_Mstate}, /*registration*/
    /*engagement*/  { diseng_mstate,error_Mstate, error_Mstate, engag_mstate, engag_mstate, engag_mstate, engag_mstate, runn_mstate,  error_Mstate,   engag_mstate, engag_mstate, engag_mstate, engag_mstate, engag_mstate, engag_mstate, error_Mstate}, /*engagement*/
    /*running*/     { diseng_mstate,error_Mstate, error_Mstate, runn_mstate,  runn_mstate,  runn_mstate,  runn_mstate,  runn_mstate,  runn_mstate,    engup_mstate, engap_mstate, enfba_Mstate, enfba_Mstate, enidl_Mstate, runn_mstate, error_Mstate}, /*running*/
    /*disengage*/   { error_Mstate, error_Mstate, error_Mstate, diseng_mstate,diseng_mstate,diseng_mstate,diseng_mstate,error_Mstate, regis_mstate,   diseng_mstate,diseng_mstate,diseng_mstate,diseng_mstate,diseng_mstate,diseng_mstate, error_Mstate}, /*disengage*/
    /*engage updat*/{ error_Mstate, error_Mstate, error_Mstate, engup_mstate, engup_mstate, engup_mstate, engup_mstate, error_Mstate, updat_mstate,   engup_mstate, engup_mstate, engup_mstate, engup_mstate, engup_mstate, engup_mstate, error_Mstate}, /*engage update*/
    /*update*/      { error_Mstate, updat_mstate, error_Mstate, updat_mstate, updat_mstate, updat_mstate, updat_mstate, error_Mstate, updat_mstate,   updat_mstate, updat_mstate, updat_mstate, updat_mstate, updat_mstate, updat_mstate, error_Mstate}, /*update*/
    /*engAp*/       { error_Mstate, error_Mstate, error_Mstate, engap_mstate, engap_mstate, engap_mstate, engap_mstate, error_Mstate, apSet_Mstate,   engap_mstate, engap_mstate, engap_mstate, engap_mstate, engap_mstate, engap_mstate, error_Mstate}, /*engage AP*/
    /*setAp*/       { error_Mstate, error_Mstate, apSet_Mstate, apSet_Mstate, apSet_Mstate, apSet_Mstate, apSet_Mstate, apSet_Mstate, regis_mstate,   apSet_Mstate, regis_mstate, regis_mstate, apSet_Mstate, regis_mstate, apSet_Mstate, error_Mstate}, /*setAp*/
    /*engage fback*/{ error_Mstate, enfba_Mstate, enfba_Mstate, enfba_Mstate, enfba_Mstate, enfba_Mstate, enfba_Mstate, enfba_Mstate, regis_mstate,   enfba_Mstate, regis_mstate, enfba_Mstate, fbset_Mstate, fbset_Mstate, enfba_Mstate, error_Mstate}, /*engage fallback*/
    /*set fallback*/{ diseng_mstate,fbset_Mstate, fbset_Mstate, fbset_Mstate, fbset_Mstate, fbset_Mstate, fbset_Mstate, fbset_Mstate, regis_mstate,   engup_mstate, fbset_Mstate, diseng_mstate,fbset_Mstate, regis_mstate, fbset_Mstate, error_Mstate}, /*fallback set*/
    /*engage idle */{ error_Mstate, enidl_Mstate, enidl_Mstate, enidl_Mstate, enidl_Mstate, enidl_Mstate, enidl_Mstate, enidl_Mstate, regis_mstate,   enidl_Mstate, enidl_Mstate, enidl_Mstate, enidl_Mstate, enidl_Mstate, idset_Mstate, error_Mstate}, /*engage idle*/
    /*idle set    */{ error_Mstate, idset_Mstate, idset_Mstate, idset_Mstate, idset_Mstate, idset_Mstate, idset_Mstate, idset_Mstate, idset_Mstate,   engup_mstate, idset_Mstate, enfba_Mstate, idset_Mstate, idset_Mstate, idset_Mstate, error_Mstate}, /*idle set*/
    /*error*/       { error_Mstate, error_Mstate, error_Mstate, error_Mstate, error_Mstate, error_Mstate, error_Mstate, error_Mstate, error_Mstate,   error_Mstate, error_Mstate, error_Mstate, error_Mstate, error_Mstate, error_Mstate, error_Mstate}/*error*/
};

fsmEffect nextEffect[total_Mstate][total_Mevent] = {
    /*stat//evt*/       /*flashOk*/ /*certsload*/ /*certsmiss*/ /*connsuccess*/ /*connfailed*//*validsuccess*//*validFailed*//*Engaged*/  /*Disengaged*/ /*trgup*/       /*apSet*/     /*trgfb*/   /*fbset */      /*en idle*/  /*idle set*/  /*errorEv*/
    /*flash*/       { rgstr_Meffect, rgstr_Meffect, rgstr_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, trgRunning,   noop_Meffect, erro_Meffect,  erro_Meffect,  erro_Meffect, erro_Meffect, erro_Meffect, erro_Meffect, noop_Meffect}, /*flash*/
    /*regis*/       { noop_Meffect,  engag_Meffect, rgstr_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, trgRunning,   noop_Meffect, erro_Meffect,  noop_Meffect,  trgfb_Meffect,noop_Meffect, trgid_Meffect,noop_Meffect, noop_Meffect}, /*registration*/
    /*engage*/      { noop_Meffect,  noop_Meffect,  erro_Meffect,  noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, trgRunning,   noop_Meffect, noop_Meffect,  noop_Meffect,  noop_Meffect, noop_Meffect,  noop_Meffect,noop_Meffect,noop_Meffect}, /*engagement*/
    /*runn*/        { disen_Meffect,  noop_Meffect,  erro_Meffect,  noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, trgRunning,  noop_Meffect, disen_Meffect,disen_Meffect, trgfb_Meffect,noop_Meffect, trgid_Meffect,noop_Meffect, noop_Meffect}, /*running*/
    /*diseng*/      { noop_Meffect,  noop_Meffect,  erro_Meffect,  noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, rgstr_Meffect, noop_Meffect,  noop_Meffect,  noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect}, /*disengage*/
    /*engUpd*/      { noop_Meffect,  noop_Meffect,  erro_Meffect,  noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, updat_Meffect,noop_Meffect,  noop_Meffect,  noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect}, /*engage update*/
    /*updat*/       { noop_Meffect,  noop_Meffect,  erro_Meffect,  noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect,  noop_Meffect,  noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect}, /*update*/
    /*engAp*/       { noop_Meffect,  noop_Meffect,  erro_Meffect,  noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, apeng_Meffect,noop_Meffect,  noop_Meffect,  noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect}, /*engAp*/
    /*setAp*/       { noop_Meffect,  noop_Meffect,  erro_Meffect,  noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect,  apdng_Meffect, apdng_Meffect, noop_Meffect,apdng_Meffect, apdng_Meffect,noop_Meffect}, /*setAp*/
    /*engage fback*/{ erro_Meffect,  erro_Meffect,  erro_Meffect,  noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, erro_Meffect, noop_Meffect, noop_Meffect,  noop_Meffect,  noop_Meffect, fbSet_Meffect,noop_Meffect, fbSet_Meffect,erro_Meffect}, /*engage fallback*/
    /*set fallback*/{ disen_Meffect,  erro_Meffect,  erro_Meffect,  noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, erro_Meffect, erro_Meffect,disen_Meffect, disen_Meffect, fbdis_Meffect,noop_Meffect, disen_Meffect,noop_Meffect, erro_Meffect}, /*set fallback*/
    /*engage idle*/ { erro_Meffect,  erro_Meffect,  erro_Meffect,  noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, erro_Meffect, noop_Meffect, noop_Meffect,  noop_Meffect,  noop_Meffect, fbSet_Meffect,noop_Meffect, idset_Meffect,erro_Meffect}, /*engage idle*/
    /*idle set*/    { erro_Meffect,  erro_Meffect,  erro_Meffect,  noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, erro_Meffect, erro_Meffect, disen_Meffect, disen_Meffect, trgfb_Meffect,noop_Meffect, iddis_Meffect,noop_Meffect, erro_Meffect}, /*idle set*/
    /*error*/       { erro_Meffect,  erro_Meffect,  erro_Meffect,  noop_Meffect, noop_Meffect, noop_Meffect, noop_Meffect, erro_Meffect, erro_Meffect, erro_Meffect,  erro_Meffect,  erro_Meffect, erro_Meffect, erro_Meffect, erro_Meffect, erro_Meffect}/*error*/
};

void fsm_q_evt(masterFSM_events newEvent) {
    if (sph_step_retries(&master_evt_lock) == TRUE) {
        masterEventQ[FSMEvtIdx].nextEvent = newEvent;
        masterEventQ[FSMEvtIdx].pending = 1;
        ESP_LOGI(TAG, "Pushing [%d] @ idx [%d]", newEvent,FSMEvtIdx);
        FSMEvtIdx++;
        if (FSMEvtIdx == MAXFSMEVTS) {
            FSMEvtIdx = 0;
        }
        sph_give(&master_evt_lock);
        return;
    } else {
        ESP_LOGE(TAG, "[Q] lOCK untacken");
    }
}

static masterFSM_events fsm_get_evt() {
    masterFSM_events toreturn;
    if (sph_step_retries(&master_evt_lock) == TRUE) {
        masterFSM_events toreturn = masterEventQ[currentFSMEvtIdx].nextEvent;
        masterEventQ[currentFSMEvtIdx].pending = 0;
        ESP_LOGI(TAG,"Deq from MFSFM queue at idx [%d]",currentFSMEvtIdx);
        currentFSMEvtIdx++;

        if (currentFSMEvtIdx == MAXFSMEVTS) {
            currentFSMEvtIdx = 0;
        }
        sph_give(&master_evt_lock);
        return toreturn;
    } else {
        ESP_LOGE(TAG, "[get]Lock untaken");
        toreturn = 0;
        return toreturn;
    }
}

static bool fsm_check_evt() {
    if (sph_step_retries(&master_evt_lock) == TRUE) {
        bool response = masterEventQ[currentFSMEvtIdx].pending;
        sph_give(&master_evt_lock);
        return response;
    } else {
        ESP_LOGE(TAG, "[check]Lock untaken");
        return FALSE;
    }

}

void master_fsm_init(void){
  sph_create(&master_evt_lock);
  fsm_q_evt((masterFSM_events)flashOk_Mevent);
}

uint8_t runMasterFSM() {
    (void) TAG;
    //TODO: ADD A FLAG TO STOP
    master_fsm_init();
    masterFSM_events nextEvent;
    while (1) {
        if (fsm_check_evt()) {
            nextEvent = fsm_get_evt();
            ESP_LOGI(TAG, "Next evt [%d], master state [%d]", nextEvent, masterState);
            railingParam.current_event = nextEvent;
            nextEffect[masterState][nextEvent](&railingParam);
            masterState = nextState[masterState][nextEvent];
            ESP_LOGI(TAG, "New state[%d]", masterState);
            railingParam.previous_status = railingParam.current_status;
            railingParam.current_status = masterState;

            vTaskDelay(500 / portTICK_RATE_MS);
        } else {
            set_ram_usage(uxTaskGetStackHighWaterMark(NULL), FSM_task);
            vTaskDelay(500 / portTICK_RATE_MS);
        }
    }
    return 0;
}

void soft_reset(void){
  esp_restart();
}
