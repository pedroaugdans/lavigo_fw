/*
 * LAVIGO PROJECT CODE
 * Each line should be prefixed with  *
 */

/*
 * File:   lavigo_masterFSM.h
 * Author: independent contractor
 *
 * Created on May 18, 2019, 9:36 AM
 */

#ifndef LAVIGO_MASTERFSM_H
#define LAVIGO_MASTERFSM_H
#include "esp_log.h"
#include "launcher.h"
#include "MFSMEffects.h"

typedef enum {
/*0*/ flash_mstate = 0,
/*1*/ regis_mstate,
/*2*/ engag_mstate,
/*3*/ runn_mstate,
/*4*/ diseng_mstate,
/*5*/ engup_mstate,
/*6*/ updat_mstate,
/*7*/ engap_mstate,
/*8*/ apSet_Mstate,
/*9*/ enfba_Mstate,
/*10*/fbset_Mstate,
/*11*/enidl_Mstate,
/*12*/idset_Mstate,
/*13*/error_Mstate,
/*14*/total_Mstate
} masterFSM_state;

typedef enum {
/*0*/ flashOk_Mevent = 0,
/*1*/ crtld_Mevent,
/*2*/ crtmss_Mevent,
/*3*/ cnnctd_Mevent,
/*4*/ cnter_Mevent,
/*5*/ vldsync_Mevent,
/*6*/ vlder_Mevent,
/*7*/ engaged_Mevent,
/*8*/ diseng_Mevent,
/*9*/ trgup_Mevent,
/*10*/apSet_Mevent,
/*11*/trgfb_Mevent,
/*12*/fbset_Mevent,
/*13*/idtrg_Mevent,
/*14*/idset_Mevent,
/*15*/error_Mevent,
/*16*/total_Mevent
} masterFSM_events;

typedef struct {
    masterFSM_events nextEvent;
    bool pending;
} FSMQueue;

extern masterFSM_state nextState[total_Mstate][total_Mevent];
extern fsmEffect nextEffect[total_Mstate][total_Mevent];
extern masterFSM_state masterState;

#define MAXFSMEVTS 10

void fsm_q_evt(masterFSM_events newEvent);
void soft_reset(void);

void master_fsm_init(void);
uint8_t runMasterFSM();


#endif /* LAVIGO_MASTERFSM_H */
