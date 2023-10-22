//
#ifndef __HUBWARE_H__
#define __HUBWARE_H__

//--- STATE MACHINE --------------------------------------------------------------------//
//
//--------------------------------------------------------------------------------------//
//#define __TESTING_MASTERFSM__
//#define __TESTING_CONNECION_FSM__

//#define __TESTING_GPIO__
//#define __TESTING_EXECUTION__
//#define __TESTING_FALLBACK__
//#define __TESTING_DEPLOYMENT__
//#define __TESTING_UPDATE__
//#define __TESTING_REGISTRATION__
//#define __TESTING_ROTATION__
#define __TESTING_MONITORING__
//--------------------------------------------------------------------------------------//

//--- COMMUNICATION --------------------------------------------------------------------//
//#define __TESTING_MSG_FMT__
//#define __TESTING_MSG_RAW__
//--------------------------------------------------------------------------------------//

//--- I-O ------------------------------------------------------------------------------//
//#define __TESTING_PIN_SEQ__
//#define __TESTING_PIN_EVT__
//#define __TESTING_PIN_XIO__
//#define __TESTING_PORT_XIO__
//--------------------------------------------------------------------------------------//

//--- MODELS ---------------------------------------------------------------------------//
//#define __TESTING_MACHINES__
//#define __TESTING_MESSAGES__  // VOID
//#define __TESTING_PARAMS__    // VOID
//--------------------------------------------------------------------------------------//
//#define __TESTING_AUX_EXP_I2C__
//--------------------------------------------------------------------------------------//

//--- UTILITIES ------------------------------------------------------------------------//
//#define __TESTING_CLK_PIT__
//--------------------------------------------------------------------------------------//

//--- DRIVERS --------------------------------------------------------------------------//
//#define __TESTING_DRV_MQTT__
//#define __TESTING_DRV_WIFI__
//#define __TESTING_DRV_UART__
//#define __TESTING_DRV_I2C__
//#define __TESTING_DRV_NVS__
//#define __TESTING_DRV_SPIFFS__
//---------------------------------------------------------------------------------------//

//--- ACCEPTANCE --------------------------------------------------------------------------//
//#define __TESTING_POWER__
//#define __TESTING_ADAPTER__
#define TESTING_TIMING
#define TIME_TEST
//---------------------------------------------------------------------------------------//



void hubware_init(void);
void hubware_run (void);

#endif
