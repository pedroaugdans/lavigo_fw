//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____ _   __            _____  ______ ____                                 //
//    / __ \ /  _// | / /           / ___/ / ____// __ \                                //
//   / /_/ / / / /  |/ /  ______    \__ \ / __/  / / / /                                //
//  / ____/_/ / / /|  /  /_____/   ___/ // /___ / /_/ /                                 //
// /_/    /___//_/ |_/            /____//_____/ \___\_\                                 //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "pin_seq.h"

#include "string.h"

#include "clk_pit.h"
#include "pin_evt.h"
#include "params.h"
#include "drv_locks.h"

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define DONE     3              //Last piece of the sequence has been recollected, sequence is ready to be dispatched or absorved
#define PENDING  2              //Sequence remains undetected or unfinished pieces
#define ACTIVE   1              //Sequence remains undetected or unfinished piece, initial status
#define INACTIVE 0              //Sequence was dispatched or absorved

#define PIN_SEQ_SIGNAL_MIN  65    // 65 == 'A'
#define PIN_SEQ_SIGNAL_MAX  90    // 90 == 'Z'

#define PIN_SEQ_SIGNAL_SET_HI 'H' //Output event, pin set high
#define PIN_SEQ_SIGNAL_SET_LO 'L' //Output event, pin set Low
#define PIN_SEQ_SIGNAL_GET_HI 'h' //Input event, pin set High
#define PIN_SEQ_SIGNAL_GET_LO 'l' //Input event, pin set Low

#define PIN_SEQ_DELAY_SET_HMS 37  // 37 == '%' : set  x100 milliseconds
#define PIN_SEQ_DELAY_SET_SEC 35  // 35 == '#' : set  x1   second
#define PIN_SEQ_DELAY_SET_MIN 38  // 38 == '&' : set  x1   minute

#define PIN_SEQ_DELAY_MIN 48      // 48 == '0'
#define PIN_SEQ_DELAY_MAX 57      // 57 == '9'

#define PIN_SEQ_DELIMITER_SEQUENCE "|"
#define PIN_SEQ_DELIMITER_EVENT    ":"

/*** VARIABLES **************************************************************************/

static char *TAG = "[PIN-SEQ]";
static const bool DEBUG = 1;

/*Pin sequence buffer.*/
static pin_seq_sequence_t pin_seq_sequences   [PIN_SEQ_MAX_SEQUENCES];

/*Array for the report status of the sequencies - Whererver there is a 1,
that sequence will be reported*/
static bool pin_seq_sequences_report[PIN_SEQ_MAX_SEQUENCES];
static uint16_t pin_seq_sequence_idx;
SemaphoreHandle_t seq_lock = NULL;

/*** DECLARATIONS ***********************************************************************/

static void pin_seq_update(void);

static bool pin_seq_isDelay (char key);
static bool pin_seq_isSignal(char key);

static uint16_t pin_seq_getTicks(char multiplier, char *base);
static uint8_t  pin_seq_getLevel(char level);
static uint8_t  pin_seq_getDir  (char key);

static void pin_seq_event (uint8_t idx);
static void pin_seq_delay (uint8_t idx);
static void pin_seq_signal(uint8_t idx, uint8_t port, uint8_t level);

static void pin_seq_sleep(uint8_t ticks);
void pin_seq_forget(uint8_t idx);

static void pin_seq_register(machines_machine_t *machine_p, machines_sequence_t *sequence_p);

/*** DEFINITIONS ************************************************************************/

uint8_t pin_seq_report_machine_seq_set(uint8_t machine_name){
  if(sph_step_retries(&seq_lock) == TRUE){
    ESP_LOGI(TAG,"Looking for sequencies on [%d]",machine_name);
    for(uint16_t k = 0; k < PIN_SEQ_MAX_SEQUENCES;k++){
      if((pin_seq_sequences[k].status == ACTIVE || pin_seq_sequences[k].status == PENDING)&&
      pin_seq_sequences[k].machine_p->deployment_info.resource == machine_name){
        ESP_LOGI(TAG,"Reporting sequence [%d] belonging to machine [%d]",k,machine_name);
        pin_seq_sequences_report[k] = true;
      }
    }
    sph_give(&seq_lock);
    return SUCCESS;
  } else {
    ESP_LOGE(TAG,"report machine set lock untaken");
    return FAILURE;
  }
}

uint8_t pin_seq_report_seq_unset(uint16_t seq_index){
  if(sph_step_retries(&seq_lock) == TRUE){
    pin_seq_sequences_report[seq_index] = false;
    sph_give(&seq_lock);
    return SUCCESS;
  } else {
    ESP_LOGE(TAG,"report machine unset lock untaken");
    return FAILURE;
  }
  return SUCCESS;
}

uint8_t pin_seq_report_get_next(pin_seq_sequence_t* seq_to_report){
  for(uint16_t k = 0; k < PIN_SEQ_MAX_SEQUENCES; k++){
    if(pin_seq_sequences_report[k] == true){
      pin_seq_report_get(k,seq_to_report);
      return (uint8_t)k;
    }
  }
  return FAILURE;
}

uint8_t any_sequence_need_report_get(void){
  for(uint16_t k = 0;k < PIN_SEQ_MAX_SEQUENCES;k++){
    if(pin_seq_sequences_report[k]){
      ESP_LOGW(TAG,"Reporting sequence [%d]",k);
      return SUCCESS;
    }
  }
  ESP_LOGW(TAG,"No sequence to report");
  return FAILURE;
}

uint8_t pin_seq_report_get(uint16_t seq_index,pin_seq_sequence_t* seq_to_report){
  uint8_t op_result =FAILURE;
  if(sph_step_retries(&seq_lock) == TRUE){
    seq_to_report->machine_p = pin_seq_sequences[seq_index].machine_p;
    seq_to_report->sequence_p = pin_seq_sequences[seq_index].sequence_p;
    seq_to_report->index = pin_seq_sequences[seq_index].index;
    seq_to_report->status = pin_seq_sequences[seq_index].status;

    if(pin_seq_sequences_report[seq_index]){
      op_result = SUCCESS;
    }
    pin_seq_sequences_report[seq_index] = false;
    sph_give(&seq_lock);
    return op_result;
  } else {
    ESP_LOGE(TAG,"report machine unset lock untaken");
    return op_result;
  }
}

void pin_seq_init(void) {
  if(DEBUG){
    /*DEBUG*/ESP_LOGI(TAG, "init()");
  }
  sph_create(&seq_lock);
  pin_seq_flush();
  if(DEBUG){
    ///*DEBUG*/ESP_LOGI(TAG, "~init()");
  }
}

void pin_seq_task(void *params) {
  /*DEBUG*/ESP_LOGI(TAG, "task()");

  FOREVER{
    pin_seq_update();
    pin_seq_sleep(PIN_SEQ_REFRESH_TIME);
  }

  /*DEBUG*/ESP_LOGI(TAG, "~task()");
}

void queues_flush(void){
    pin_evt_flush();
    clk_pit_flush();
    pin_seq_flush();
}

void pin_seq_flush(void){
    ESP_LOGI(TAG,"Flushing PIN sequencies...");
    for (uint16_t k = 0;k < PIN_SEQ_MAX_SEQUENCES;k++){
        pin_seq_forget(k);
    }
    pin_seq_sequence_idx = 0;
}

void pin_seq_forget(uint8_t idx){
    pin_seq_sequences[idx].status = INACTIVE;
}

bool pin_seq_push(machines_machine_t *machine_p, machines_sequence_t* sequence_p) {
  ///*DEBUG*/ESP_LOGI(TAG, "push()");
  if(sph_step_retries(&seq_lock) == TRUE){
    if(pin_seq_sequence_idx > PIN_SEQ_MAX_SEQUENCES - 1) {
      ESP_LOGE(TAG,"Pin sequence overflow");
      pin_seq_sequence_idx = 0;
    } // TODO: Handle overflows & fragmentation
    if(DEBUG){
      ESP_LOGI(TAG,"[Seq Push] seq_idx : [%d] / [%d]: [%s] for machine [%d]",pin_seq_sequence_idx,PIN_SEQ_MAX_SEQUENCES,sequence_p->name,machine_p->deployment_info.resource);
    }
    pin_seq_register(machine_p, sequence_p);
    pin_seq_sequences[pin_seq_sequence_idx].machine_p  = machine_p;
    pin_seq_sequences[pin_seq_sequence_idx].sequence_p = sequence_p;
    pin_seq_sequences[pin_seq_sequence_idx].index      = 0;
    pin_seq_sequences[pin_seq_sequence_idx].status     = ACTIVE;

    do {
      pin_seq_sequence_idx++;
    }
    while(pin_seq_sequences[pin_seq_sequence_idx].status != INACTIVE);
    sph_give(&seq_lock);
    return SUCCESS;
  } else {
    ESP_LOGE(TAG,"Push lock untaken");
    return FAILURE;
  }
}


uint8_t pin_seq_next(pin_seq_sequence_t *sequence) {
  /*DEBUG*///ESP_LOGI(TAG, "next()");
  if(sph_step_retries(&seq_lock) == TRUE){
    for (uint8_t idx = 0; idx < PIN_SEQ_MAX_SEQUENCES; idx++) {
      if (pin_seq_sequences[idx].status == DONE) {
        ESP_LOGI(TAG,"Done sequence [%s]",pin_seq_sequences[idx].sequence_p->name);
        *sequence = pin_seq_sequences[idx];

        pin_seq_sequences[idx].status = INACTIVE;
        sph_give(&seq_lock);
        return SUCCESS;
      }
    }

    /*DEBUG*///ESP_LOGI(TAG, "~next()");
    sph_give(&seq_lock);
    return FAILURE;
  } else {
    ESP_LOGE(TAG,"Next Lock untaken");
    return SUCCESS;
  }
}

static void pin_seq_update(void) {
  /*DEBUG*///ESP_LOGI(TAG, "update()");

  clk_pit_event_t clk_pit_event;
  pin_evt_event_t pin_evt_event;
  if(sph_step_retries(&seq_lock) == TRUE){
    while (clk_pit_next(&clk_pit_event) == SUCCESS) {
      pin_seq_delay(clk_pit_event.ref);
    }

    while (pin_evt_next(&pin_evt_event) == SUCCESS) {
      pin_seq_signal(pin_evt_event.ref, pin_evt_event.port, pin_evt_event.level);
    }

    for(uint16_t idx = 0; idx < PIN_SEQ_MAX_SEQUENCES; idx++) {
      //ESP_LOGI(TAG, "update() idx:[%d]", idx);
      if(pin_seq_sequences[idx].status == ACTIVE) {
        pin_seq_event(idx);
      }
    }
    set_ram_usage(uxTaskGetStackHighWaterMark(NULL), pin_seq_task_idx);
    sph_give(&seq_lock);
  } else {
    ESP_LOGE(TAG,"Update lock untaken");
  }
}

static void pin_seq_delay(uint8_t idx) {
  /*DEBUG*///ESP_LOGI(TAG, "<< delay() idx:[%d]", idx);

  char pattern[PIN_SEQ_MAX_CHARS_PER_SEQUENCE] = { 0 };

  strlcpy(pattern, pin_seq_sequences[idx].sequence_p->pattern, strlen(pin_seq_sequences[idx].sequence_p->pattern) + 1);

  char *event = strtok(pattern, PIN_SEQ_DELIMITER_SEQUENCE);

  for(uint8_t i = 0; i < pin_seq_sequences[idx].index; i++) {
    event = strtok(NULL, PIN_SEQ_DELIMITER_SEQUENCE);
  }

  char *keys = strtok(event, PIN_SEQ_DELIMITER_EVENT);

  if (keys) {
    if (pin_seq_isDelay(event[0])) {
      pin_seq_sequences[idx].index++;
      pin_seq_sequences[idx].status = ACTIVE;
    }
  }

  //ESP_LOGI(TAG, "<< delay()");
}

static void pin_seq_signal(uint8_t idx, uint8_t port, uint8_t level) {
  /*DEBUG*///ESP_LOGI(TAG, "<< signal() idx:[%d] port:[0x%X] level:[0x%X]", idx, port, level);

  char pattern [PIN_SEQ_MAX_CHARS_PER_SEQUENCE] = { 0 };

  strlcpy(pattern, pin_seq_sequences[idx].sequence_p->pattern, strlen(pin_seq_sequences[idx].sequence_p->pattern) + 1);

  char *event = strtok(pattern, PIN_SEQ_DELIMITER_SEQUENCE);

  for(uint8_t i = 0; i < pin_seq_sequences[idx].index; i++) {
    event = strtok(NULL, PIN_SEQ_DELIMITER_SEQUENCE);
  }

  //ESP_LOGI(TAG, "<< event() idx:[%d] index:[%d] evt:[%s]", idx, pin_seq_sequences[idx].index, event);

  char *keys = strtok(event, PIN_SEQ_DELIMITER_EVENT);

  keys = strtok(NULL, PIN_SEQ_DELIMITER_EVENT);

  if (keys) {
    if (pin_seq_isSignal(event[0])) {
      if ( port == machines_signal(pin_seq_sequences[idx].machine_p, pin_seq_sequences[idx].sequence_p->target, event[0])
       || level == pin_seq_getLevel(*keys)) {
        pin_seq_sequences[idx].index++;
        pin_seq_sequences[idx].status = ACTIVE;
      }
    }
  }

  //ESP_LOGI(TAG, "<< event()");
}

static void pin_seq_event(uint8_t idx) {
  //ESP_LOGI(TAG, ">> event()");

  char pattern [PIN_SEQ_MAX_CHARS_PER_SEQUENCE] = { 0 };

  strlcpy(pattern, pin_seq_sequences[idx].sequence_p->pattern, strlen(pin_seq_sequences[idx].sequence_p->pattern) + 1);

  //ESP_LOGI(TAG, ">> pattern:[%s]", pattern);

  char *event = strtok(pattern, PIN_SEQ_DELIMITER_SEQUENCE);

  for(uint8_t i = 0; i < pin_seq_sequences[idx].index; i++) {
    //ESP_LOGI(TAG, ">> [%s]", event);

    event = strtok(NULL, PIN_SEQ_DELIMITER_SEQUENCE);
  }

  if (!event) {
    pin_seq_sequences[idx].status = DONE;

    return;
  }

  //ESP_LOGI(TAG, ">> event() idx:[%d] index:[%d] evt:[%s]", idx, pin_seq_sequences[idx].index, event);

  char *keys = strtok(event, PIN_SEQ_DELIMITER_EVENT);

  keys = strtok(NULL, PIN_SEQ_DELIMITER_EVENT);

  if (pin_seq_isDelay(event[0])) {
    if (pin_seq_sequences[idx].status == ACTIVE) {
      pin_seq_sequences[idx].status = PENDING;
      if(DEBUG){
        //ESP_LOGI(TAG,"Multiplier [%c], base [%s]",event[0], keys);
      }
      uint16_t hms = pin_seq_getTicks(event[0], keys);
      if(DEBUG){
        //ESP_LOGI(TAG, ">> delay() idx:[%d] index:[%d] evt:[%s] [%c:%s], HMS: [%d]", idx, pin_seq_sequences[idx].index, event, event[0], keys,
        //hms);
      }
      clk_pit_push(idx, hms);
    }
  }
  else if (pin_seq_isSignal(event[0])) {
    if (pin_seq_sequences[idx].status == ACTIVE) {

      uint8_t port  = machines_signal(pin_seq_sequences[idx].machine_p, pin_seq_sequences[idx].sequence_p->target, event[0]);
      uint8_t dir   = pin_seq_getDir(*keys);
      uint8_t level = pin_seq_getLevel(*keys);

      //ESP_LOGI(TAG, ">> signal() index:[%d] signal:[%c:%s] port:[0x%X] dir:[%d] level:[%d]", pin_seq_sequences[idx].index, event[0], keys, port, dir, level);

      if (dir == IDIR) {
        pin_seq_sequences[idx].status = PENDING;
      }
      else if (dir == ODIR) {
        pin_seq_sequences[idx].index++;
      }

      pin_evt_push(idx, port, dir, level);
    }
  }
  //ESP_LOGI(TAG, ">> ~event()");
}

static bool pin_seq_isDelay(char key) {
  if (key == PIN_SEQ_DELAY_SET_HMS
   || key == PIN_SEQ_DELAY_SET_SEC
   || key == PIN_SEQ_DELAY_SET_MIN) {
     return TRUE;
   }

   return FALSE;
}

static bool pin_seq_isSignal(char key) {
  if (key >= PIN_SEQ_SIGNAL_MIN
   && key <= PIN_SEQ_SIGNAL_MAX) {
     return TRUE;
   }

   return FALSE;
}

static uint16_t pin_seq_getTicks(char multiplier, char *base) {
  uint16_t hms = 0;

  for (uint8_t i = 0; base[i] >= PIN_SEQ_DELAY_MIN && base[i] <= PIN_SEQ_DELAY_MAX; i++) {
     hms = hms *10 + (base[i] - PIN_SEQ_DELAY_MIN);
  }

  if (multiplier == PIN_SEQ_DELAY_SET_SEC) {
    hms *= 10;
  }
  else if (multiplier == PIN_SEQ_DELAY_SET_MIN) {
    hms = (hms > 10 ? 10 : hms) *60 *10;
  }

  return hms; // hms = hundreds of milliseconds
}

static uint8_t pin_seq_getDir(char key) {
  if (key == PIN_SEQ_SIGNAL_SET_HI || key == PIN_SEQ_SIGNAL_SET_LO) {
     return 0;
   }

   return 1;
}

static uint8_t pin_seq_getLevel(char value) {
  if (value == PIN_SEQ_SIGNAL_SET_HI || value == PIN_SEQ_SIGNAL_GET_HI) {
    return 1;
  }

  return 0;
}

static void pin_seq_sleep(uint8_t ticks) {
  /*DEBUG*///ESP_LOGI(TAG, "sleep()");

  vTaskDelay(ticks / portTICK_RATE_MS);

  /*DEBUG*///ESP_LOGI(TAG, "~sleep()");
}

static void pin_seq_register(machines_machine_t *machine_p, machines_sequence_t *sequence_p) {
  /*DEBUG*///ESP_LOGI(TAG, "register()");

  char    pattern  [PIN_SEQ_MAX_CHARS_PER_SEQUENCE]   = { 0 };
  char    signals  [PIN_SEQ_MAX_SIGNALS_PER_SEQUENCE] = { 0 };
  uint8_t new_ports[PIN_SEQ_MAX_SIGNALS_PER_SEQUENCE] = { 0 };
  uint8_t nof_signals                                 =   0  ;

  strlcpy(pattern, sequence_p->pattern, strlen(sequence_p->pattern) + 1);

  char *event = strtok(pattern, PIN_SEQ_DELIMITER_SEQUENCE);

  while (event != NULL) {
    //ESP_LOGI(TAG, "evt:[%s]", event);
    if (pin_seq_isSignal(event[0])) {
      //ESP_LOGI(TAG, "[%c] is signal", event[0]);
      for (uint8_t signal_idx = 0; signal_idx <= nof_signals; signal_idx++) {
        if (signals[signal_idx] == event[0]) {
          //ESP_LOGI(TAG, "[%c] is repeated", event[0]);
          break;
        }

        //ESP_LOGI(TAG, "[%c] is new", event[0]);

        if (signal_idx == nof_signals) {
          //ESP_LOGI(TAG, "[%c] is saved", event[0]);
          signals  [nof_signals] = event[0];
          new_ports[nof_signals] = machines_signal(machine_p, sequence_p->target, event[0]);

          nof_signals++;
          break;
        }
      }
    }

    event = strtok(NULL, PIN_SEQ_DELIMITER_SEQUENCE);
  }

  for (int i = 0; i < nof_signals; i++) {
    pin_evt_register(new_ports[i]);  // TODO: Avoid repeated registers across deployments.
  }

  /*DEBUG*///ESP_LOGI(TAG, "~register()");
}
