/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MFSMEffects.h
 * Author: independent contractor
 *
 * Created on August 18, 2019, 12:39 PM
 */

#ifndef MFSMEFFECTS_H
#define MFSMEFFECTS_H

#include "stdint.h"

typedef struct{
    uint8_t current_status;
    uint8_t previous_status;
    uint8_t current_event;
} effectParam;


typedef uint8_t(*fsmEffect)(effectParam *param);

uint8_t erro_Meffect    (effectParam *param);
uint8_t noop_Meffect    (effectParam *param);
uint8_t rgstr_Meffect   (effectParam *param);
uint8_t cnnctn_Meffect  (effectParam *param);
uint8_t vldtn_Meffect   (effectParam *param);
uint8_t engag_Meffect   (effectParam *param);
uint8_t updat_Meffect   (effectParam *param);
uint8_t trgAp_effect    (effectParam *param);
uint8_t disen_Meffect   (effectParam *param);
uint8_t trgRunning      (effectParam *param);
uint8_t apeng_Meffect   (effectParam * param);
uint8_t apdng_Meffect   (effectParam * param);
uint8_t trgfb_Meffect   (effectParam *param);
uint8_t fbSet_Meffect   (effectParam *param);
uint8_t fbdis_Meffect   (effectParam *param);
uint8_t trgid_Meffect   (effectParam *param);
uint8_t idset_Meffect   (effectParam *param);
uint8_t iddis_Meffect   (effectParam *param);

#endif /* MFSMEFFECTS_H */

