//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____ _   __            ______ _    __ ______                              //
//    / __ \ /  _// | / /           / ____/| |  / //_  __/                              //
//   / /_/ / / / /  |/ /  ______   / __/   | | / /  / /                                 //
//  / ____/_/ / / /|  /  /_____/  / /___   | |/ /  / /                                  //
// /_/    /___//_/ |_/           /_____/   |___/  /_/                                   //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "pin_evt.h"
#include "pin_xio.h"
#include "port_xio.h"
#include "machines.h"
#include "params.h"

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define ACTIVE   2  //Event happened and/or was detected, ready to be dispatched or absorved
#define PENDING  1  //Still waiting for event to happen
#define INACTIVE 0  //Event already dispatched or absorved
#define NDIRS 2

/*** VARIABLES **************************************************************************/

static const char *TAG = "[PIN-EVT]";
static const bool DEBUG = 0;

/*Event possible targets*/
typedef struct {
    uint8_t port;
} pin_evt_pin_t;

static pin_evt_pin_t pin_evt_pins [PIN_EVT_MAX_PINS];
static bool pin_evt_status [PIN_EVT_MAX_PINS]__attribute__((unused));
static uint8_t pin_evt_pin_idx;

// TODO: Change all this to buffer-circ and cuffer-frag
static pin_evt_event_t pin_evt_events [NDIRS][PIN_EVT_MAX_EVENTS];
static uint16_t pin_evt_event_idx [NDIRS];

/*** DECLARATIONS ***********************************************************************/

static void pin_evt_update(void);

static void pin_evt_sleep(uint8_t ticks);

static uint8_t port2pin(uint8_t port);
void pin_evt_forget(uint16_t idx);
void pin_evt_unregister(void);

/*** DEFINITIONS ************************************************************************/

void pin_evt_init(void) {
  if(DEBUG){
    /*DEBUG*/ESP_LOGI(TAG, "init()");
  }
  if(DEBUG){
    /*DEBUG*/ESP_LOGI(TAG, "~init()");
  }
}

void pin_evt_task(void *params) {
    if(DEBUG){
    /*DEBUG*/ESP_LOGI(TAG, "task()");
    }
    FOREVER{
        pin_evt_update();
        pin_evt_sleep(PIN_EVT_REFRESH_TIME);
      }
}

void pin_evt_flush(void) {
    ESP_LOGI(TAG, "Flushing PIN EVENTS...");
    pin_evt_unregister();
    for (uint16_t k = 0; k < PIN_EVT_MAX_EVENTS; k++) {
        pin_evt_forget(k);
    }
    pin_evt_event_idx[0] = 0;
    pin_evt_event_idx[1] = 0;

}

void pin_evt_forget(uint16_t idx) {
    pin_evt_events[1][idx].status = INACTIVE;
    pin_evt_events[0][idx].status = INACTIVE;
}

void pin_evt_unregister(void) {
    for (uint16_t k = 0; k < PIN_EVT_MAX_PINS; k++) {
        pin_evt_pins[k].port = 0;
    }
    pin_evt_pin_idx = 0;
}

bool is_pin_registered(uint8_t port) {
    for (uint8_t k = 0; k < pin_evt_pin_idx; k++) {
        if (pin_evt_pins[k].port == port) {
            return TRUE;
        }
    }
    if(DEBUG){
    ESP_LOGI(TAG,"Registering new port");
    }
    return FALSE;
}

void pin_evt_register(uint8_t port) {
  /*DEBUG*///ESP_LOGI(TAG, "register()");

  if (!is_pin_registered(port)) {
    pin_evt_pins[pin_evt_pin_idx].port = port;
    pin_evt_pin_idx++;
  } else {
    if(DEBUG){
      ESP_LOGW(TAG,"Pin already registered !");
    }
  }
  if(pin_evt_pin_idx > PIN_EVT_MAX_PINS){
    ESP_LOGE(TAG,"[OVERFLOW] Pin buffer overflow");
  }
  if(DEBUG){
    ESP_LOGI(TAG, "[Pin REG] Registering port [%d] [%d] / [%d]", port, pin_evt_pin_idx, PIN_EVT_MAX_PINS);
  }
}

bool pin_evt_push(uint16_t ref, uint8_t port, uint8_t dir, uint8_t level) {
  /*DEBUG*///ESP_LOGI(TAG, "push()");

  if (pin_evt_event_idx[dir] > PIN_EVT_MAX_EVENTS - 1) {
    pin_evt_event_idx[dir] = 0;
  } // TODO: Handle overflows & fragmentation

  pin_evt_events[dir][pin_evt_event_idx[dir]].ref = ref;
  pin_evt_events[dir][pin_evt_event_idx[dir]].port = port;
  pin_evt_events[dir][pin_evt_event_idx[dir]].pin = port2pin(port);
  pin_evt_events[dir][pin_evt_event_idx[dir]].dir = dir;
  pin_evt_events[dir][pin_evt_event_idx[dir]].level = level;
  pin_evt_events[dir][pin_evt_event_idx[dir]].status = PENDING;

  pin_evt_event_idx [dir]++;

  if(DEBUG){
    ESP_LOGI(TAG, "[Pin evt push] Pushing [%s] evt [%d] / [%d] at port [%d]", dir ? "IN" : "OUT", pin_evt_event_idx[dir], PIN_EVT_MAX_EVENTS, port);
  }
/*DEBUG*///ESP_LOGI(TAG, "~push()");

return SUCCESS;
}

bool pin_evt_next(pin_evt_event_t *event) {
    /*DEBUG*///ESP_LOGI(TAG, "next()");

    for (uint16_t i = 0; i < PIN_EVT_MAX_EVENTS; i++) {
        if (pin_evt_events[IDIR][i].status == ACTIVE) {
            *event = pin_evt_events[IDIR][i];

            pin_evt_events[IDIR][i].status = INACTIVE;

            return SUCCESS;
        }
    }

    /*DEBUG*///ESP_LOGI(TAG, "~next()");

    return FAILURE;
}

static void pin_evt_update(void) {
    for (uint16_t i = 0; i < PIN_EVT_MAX_EVENTS; i++) {
        if (pin_evt_events[ODIR][i].status == PENDING) {
          if(DEBUG){
            ESP_LOGI(TAG, "SET [0x%X] [%d], @ port [%d]", pin_evt_events[ODIR][i].port, pin_evt_events[ODIR][i].level,pin_evt_events[ODIR][i].port);
          }
            port_set(
                    pin_evt_events[ODIR][i].port,
                    pin_evt_events[ODIR][i].level
                    );

            pin_evt_events[ODIR][i].status = INACTIVE;
        }
    }

    for (uint16_t i = 0; i < PIN_EVT_MAX_EVENTS; i++) {
        if (pin_evt_events[IDIR][i].status == PENDING) {
          if(port_look_for_status(pin_evt_events[IDIR][i].port, pin_evt_events[IDIR][i].level) == SUCCESS){
            pin_evt_events[IDIR][i].status = ACTIVE;
          }
        }
    }
    /*DEBUG*///ESP_LOGI(TAG, "~update()");
}

static void pin_evt_sleep(uint8_t ticks) {
    /*DEBUG*///ESP_LOGI(TAG, "sleep()");

    vTaskDelay(ticks / portTICK_RATE_MS);

    /*DEBUG*///ESP_LOGI(TAG, "~sleep()");
}

static uint8_t port2pin(uint8_t port) {
    for (uint8_t i = 0; i < pin_evt_pin_idx; i++) {
        if (pin_evt_pins[i].port == port) {
            return i;
        }
    }
    ESP_LOGE(TAG,"PORT 2 PIN FAILED");
    return FAILURE;
}
