//////////////////////////////////////////////////////////////////////////////////////////
//     ____   ____ _   __            _  __  ____ ____                                   //
//    / __ \ /  _// | / /           | |/ / /  _// __ \                                  //
//   / /_/ / / / /  |/ /  ______    |   /  / / / / / /                                  //
//  / ____/_/ / / /|  /  /_____/   /   | _/ / / /_/ /                                   //
// /_/    /___//_/ |_/            /_/|_|/___/ \____/                                    //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "pin_xio.h"

#include "drv_i2c.h"
#include "machines.h"

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define PIN_XIO_ADAPTER_ADDRESS I2C_MCP23017_ADDRESS

#define PIN_XIO_IODIRA_REGISTER I2C_MCP23017_IODIRA
#define PIN_XIO_IODIRB_REGISTER I2C_MCP23017_IODIRB
#define PIN_XIO_GPIOA_REGISTER  I2C_MCP23017_GPIOA
#define PIN_XIO_GPIOB_REGISTER  I2C_MCP23017_GPIOB
#define PIN_XIO_OLATA_REGISTER  I2C_MCP23017_OLATA
#define PIN_XIO_OLATB_REGISTER  I2C_MCP23017_OLATB

#define STATUS_TO_PORT_A(sts)    ((sts     ) & 0xFF  )
#define STATUS_TO_PORT_B(sts)    ((sts >> 8) & 0xFF  )
#define PORTS_TO_STATUS(pA, pB) (((pB  << 8) & 0xFF00) | (pA & 0x00FF))

#define DELAY 10

/*** VARIABLES **************************************************************************/

static const char *TAG = "[PIN-XIO]";

typedef struct {
    uint16_t osts;
    uint16_t ists;
} pin_sts_t;

pin_spec_t pin_xio_layout[] = {
    /*******************************************/
    /*     |    JST     |  |    JST     |      */
    /*-----------------------------------------*/
    /* 0*/
    {pA, p4, odir},
    {pA, p0, odir}, /* 1*/
    /* 2*/
    {pA, p5, odir},
    {pA, p1, odir}, /* 3*/
    /* 4*/
    {pA, p6, odir},
    {pA, p3, odir}, /* 5*/
    /* 6*/
    {pA, p7, odir},
    {pA, p2, odir}, /* 7*/
    /*-----------------------------------------*/
    /*     |            FLEX            |      */
    /*******************************************/

    /*******************************************/
    /*     |    JST     |  |    JST     |      */
    /*-----------------------------------------*/
    /* 8*/
    {pB, p3, idir},
    {pB, p7, idir}, /* 9*/
    /*10*/
    {pB, p2, idir},
    {pB, p6, idir}, /*11*/
    /*12*/
    {pB, p1, idir},
    {pB, p4, idir}, /*13*/
    /*14*/
    {pB, p0, idir},
    {pB, p5, idir}, /*15*/
    /*-----------------------------------------*/
    /*     |            FLEX            |      */
    /*******************************************/
};



static bool pin_xio_isPresent [PIN_XIO_MAX_ADAPTERS];
static bool pin_xio_isEnabled [PIN_XIO_MAX_ADAPTERS];
static pin_sts_t pin_xio_status [PING_PONG_ID][PIN_XIO_MAX_ADAPTERS][MAX_INPUT_TIMESTAMP];
static bool ping_pong_idx = ping_pong_0;

static uint8_t pin_xio_nof_adapters;

SemaphoreHandle_t xio_lock;
SemaphoreHandle_t buffer_lock;
SemaphoreHandle_t ping_pong_lock;
SemaphoreHandle_t xio_timestamp_lock[MAX_INPUT_TIMESTAMP];
SemaphoreHandle_t xio_timestamp_idx_lock;

static xio_refresh_cb refresh_cv = NULL;

static input_timestamp_t input_timestamp_place = 0;
static input_timestamp_t output_timestamp_next[PIN_XIO_MAX_ADAPTERS] = {0};
static input_timestamp_t output_timestamp_current[PIN_XIO_MAX_ADAPTERS] = {0};
static input_timestamp_t output_timestamp_overflow[PIN_XIO_MAX_ADAPTERS] = {0};
/*** DECLARATIONS ***********************************************************************/

static void pin_in_update(void);
static void pin_out_update(void);
static void pin_xio_adapter_bootup(uint8_t adapter);
static uint8_t pin_xio_check_status(uint8_t adapter);
static void pin_xio_sleep(uint8_t ticks);

/*** DEFINITIONS ************************************************************************/

void assign_refresh_cv(xio_refresh_cb new_refresh_cv){
  refresh_cv = new_refresh_cv;
}

static void start_timestamp_sphs(void){
  sph_create(&xio_timestamp_idx_lock);
  for(uint8_t k = 0; k < MAX_INPUT_TIMESTAMP; k++){
    sph_create(&xio_timestamp_lock[k]);
    input_timestamp_place = 0;
  }
}

void pin_xio_init(void) {
  /*DEBUG*/ESP_LOGI(TAG, "init()");
  sph_create(&xio_lock);
  sph_create(&buffer_lock);
  sph_create(&ping_pong_lock);
  start_timestamp_sphs();
  pin_xio_nof_adapters = 0;

  for (uint8_t adapter = 0; adapter < PIN_XIO_MAX_ADAPTERS; adapter++) {
    pin_xio_isPresent[adapter] = FALSE;
    if (sph_step_retries(&xio_lock) == TRUE) {
      if (pin_xio_check_status(adapter) == SUCCESS) {
        ESP_LOGI(TAG, "(200) Adapter [%d]", adapter);
        pin_xio_isPresent[adapter] = TRUE; // TODO: Also check adapter type.
        pin_xio_isEnabled[adapter] = TRUE; // TODO: Define rule for this (based on type?).
        pin_xio_adapter_bootup(adapter);
        pin_xio_nof_adapters++;
        //pin_xio_adapter_bootup(adapter);

      } else {
      }
      sph_give(&xio_lock);
    } else {
      ESP_LOGI(TAG, "[XIO INIT] Lock untaken");
    }
  }

  /*DEBUG*/ESP_LOGI(TAG, "~init()");
}
static uint8_t pin_xio_check_status(uint8_t adapter){
  uint8_t read_iodira,read_iodirb;
  esp_err_t ret = drv_i2c_read(PIN_XIO_ADAPTER_ADDRESS + adapter, PIN_XIO_IODIRA_REGISTER, &read_iodira); // IODIRA = IOIOIOIO
  if(ret == ESP_FAIL){
    ESP_LOGE(TAG,"Adapter not found");
    return FAILURE;
  }
  ESP_LOGI(TAG,"Read IODIRA: [%d]",read_iodira);
  drv_i2c_read(PIN_XIO_ADAPTER_ADDRESS + adapter, PIN_XIO_IODIRB_REGISTER, &read_iodirb); // IODIRB = OIOIOIOI
  ESP_LOGI(TAG,"Read IODIRB: [%d]",read_iodirb);
  if((read_iodira == PIN_XIO_IODIRA_MASK) && (read_iodirb == PIN_XIO_IODIRB_MASK)){
    ESP_LOGI(TAG,"Not reconfiguring");
  } else {
    ESP_LOGW(TAG,"Reconfiguring");
    drv_i2c_write(PIN_XIO_ADAPTER_ADDRESS + adapter, PIN_XIO_IODIRA_REGISTER, PIN_XIO_IODIRA_MASK); // IODIRA = 0x00
    drv_i2c_write(PIN_XIO_ADAPTER_ADDRESS + adapter, PIN_XIO_IODIRB_REGISTER, PIN_XIO_IODIRB_MASK); // IODIRB = 0xFF
  }
  return SUCCESS;
}

static void pin_xio_adapter_bootup(uint8_t adapter){
  /*DEBUG*///ESP_LOGI(TAG, "update()");
  uint8_t dataA, dataB;
  drv_i2c_read(PIN_XIO_ADAPTER_ADDRESS + adapter, PIN_XIO_GPIOA_REGISTER, &dataA);
  ESP_LOGI(TAG,"Read DATAA: [%d]",dataA);
  drv_i2c_read(PIN_XIO_ADAPTER_ADDRESS + adapter, PIN_XIO_GPIOB_REGISTER, &dataB);
  ESP_LOGI(TAG,"Read DATAB: [%d]",dataB);
  //pin_xio_status[adapter].ists = PORTS_TO_STATUS(dataA, ~dataB);
  for(uint8_t k = 0; k < MAX_INPUT_TIMESTAMP ; k++){
    pin_xio_status[0][adapter][k].osts = PORTS_TO_STATUS(dataA, dataB);
  }

}

void pin_in_task(void *params) {
    /*DEBUG*/ESP_LOGI(TAG, "task()");

    FOREVER{
        pin_in_update();

        pin_xio_sleep(PIN_XIO_REFRESH_TIME);}

    /*DEBUG*/ESP_LOGI(TAG, "~task()");
}

void pin_out_task(void *params) {
    /*DEBUG*/ESP_LOGI(TAG, "task()");

    FOREVER{
        pin_out_update();

        pin_xio_sleep(PIN_XIO_REFRESH_TIME);}

    /*DEBUG*/ESP_LOGI(TAG, "~task()");
}

bool get_process_ping_pong(void){
  bool ping_pong_handle = FALSE;
  if(sph_step_retries(&ping_pong_lock)){
    ping_pong_handle = ping_pong_idx;
    sph_give(&ping_pong_lock);
  } else {
    ESP_LOGE(TAG,"Ping pong give untaken");
  }
  return ping_pong_handle;
}

void toggle_ping_pong(void){
  if(sph_step_retries(&ping_pong_lock)){
    ping_pong_idx = !ping_pong_idx;
    sph_give(&ping_pong_lock);
  } else {
    ESP_LOGE(TAG,"Ping pong toggle untaken");
  }
}

void pin_xio_set(uint8_t adapter, uint8_t pin, bool status) {
    ///*DEBUG*/ESP_LOGI(TAG, "set() [0x%X] [%d] [%d]", adapter, pin, status);
    uint8_t shift = (pin_xio_layout[pin].iobit + pin_xio_layout[pin].ioreg * 8);
    if (sph_step_retries(&buffer_lock) == TRUE) {
        //bool ping_pong_handle = !get_process_ping_pong();
        if (status) {
            pin_xio_status[0][adapter][output_timestamp_next[adapter]].osts |= (1 << shift);
        } else {
            pin_xio_status[0][adapter][output_timestamp_next[adapter]].osts &= ~(1 << shift);
        }

        if(output_timestamp_next[adapter] < MAX_INPUT_TIMESTAMP - 1){
            output_timestamp_next[adapter]++;
        } else {
            output_timestamp_next[adapter] = 0;
            output_timestamp_overflow[adapter]++;
        } /*The output buffer works as a circular buffer. Each status written moves
        one step forward in the output buffer index, or starts over.*/
        sph_give(&buffer_lock);
    } else {
        ESP_LOGE(TAG, "[SET] Lock untaken");
    }

    /*DEBUG*///ESP_LOGI(TAG, "~set()");
}

void pin_xio_get(uint8_t adapter, uint8_t pin, bool *status, uint8_t t_idx) {
  /*DEBUG*///ESP_LOGI(TAG, "get()");

  uint8_t shift = (pin_xio_layout[pin].iobit + pin_xio_layout[pin].ioreg * 8);
  uint8_t time_idx_handler = 0;
  bool ping_pong_handle = !get_process_ping_pong();/*Get the last filled buffer
  which is NOT written*/

  if (sph_step_retries(&xio_timestamp_idx_lock) == TRUE) {
    if(t_idx < MAX_INPUT_TIMESTAMP){
      time_idx_handler = t_idx;
    } else {
      ESP_LOGE(TAG,"Unsupported [%d]",t_idx); // case for retrocompatibility
      if(input_timestamp_place < (MAX_INPUT_TIMESTAMP - 1)) {
        time_idx_handler = input_timestamp_place + 1;
      } else {
        time_idx_handler = 0;
      }
    }
    *status = pin_xio_status[ping_pong_handle][adapter][time_idx_handler].ists & (1 << shift);
    sph_give(&xio_timestamp_idx_lock);
  } else {
    ESP_LOGE(TAG,"XIO get Lock untaken");
  }
}

static void pin_in_update(void) {
  /*DEBUG*///ESP_LOGI(TAG, "update()");
  uint8_t dataA, dataB;
  bool ping_pong_handler = FALSE;
  ping_pong_handler = get_process_ping_pong(); /*Get the current buffer which is being written
  to write the next timestamp on all available adapters*/
  for (uint8_t adapter = 0; adapter < PIN_XIO_MAX_ADAPTERS; adapter++) {
    if (pin_xio_isPresent[adapter]) {
      if (pin_xio_isEnabled[adapter]) {
        drv_i2c_read(PIN_XIO_ADAPTER_ADDRESS + adapter, PIN_XIO_GPIOA_REGISTER, &dataA);
        drv_i2c_read(PIN_XIO_ADAPTER_ADDRESS + adapter, PIN_XIO_GPIOB_REGISTER, &dataB);

        if(sph_step_retries(&xio_timestamp_idx_lock)){
          pin_xio_status[ping_pong_handler][adapter][input_timestamp_place].ists = PORTS_TO_STATUS(dataA, ~dataB);
          sph_give(&xio_timestamp_idx_lock);
        } else {
          ESP_LOGE(TAG,"READ LOCK untaken");
        }
      }
    }
  }
  if(input_timestamp_place < (MAX_INPUT_TIMESTAMP - 1)) {
    input_timestamp_place++;

  } else {
    input_timestamp_place = 0;
    if(refresh_cv != NULL){
      refresh_cv();
    }
    toggle_ping_pong();
  }

  /*DEBUG*///ESP_LOGI(TAG, "~update()");
}

static void pin_out_update(void){
  uint8_t dataA, dataB;
  esp_err_t ret;
  //if (pin_xio_shouldRefresh[adapter]) {
  for (uint8_t adapter = 0; adapter < PIN_XIO_MAX_ADAPTERS; adapter++) {
    if (pin_xio_isPresent[adapter]) {
      if (pin_xio_isEnabled[adapter]) {
        if (sph_step_retries(&buffer_lock) == TRUE) {
          //ESP_LOGI(TAG,"[%d] Refreshed",adapter);
          if( (output_timestamp_current[adapter] < output_timestamp_next[adapter])
          || (output_timestamp_overflow[adapter] > 0) ){
            dataA = STATUS_TO_PORT_A(pin_xio_status[0][adapter][output_timestamp_current[adapter]].osts);
            dataB = STATUS_TO_PORT_B(pin_xio_status[0][adapter][output_timestamp_current[adapter]].osts);
            if(output_timestamp_current[adapter] < MAX_INPUT_TIMESTAMP - 1){
              output_timestamp_current[adapter]++;
            } else {
              output_timestamp_current[adapter] = 0;
              output_timestamp_overflow[adapter]--;
            }/*The output buffer works as a circular buffer. Each iteration will drive .*/
            sph_give(&buffer_lock);
            ret = drv_i2c_write(PIN_XIO_ADAPTER_ADDRESS + adapter, PIN_XIO_GPIOA_REGISTER, dataA);
            if (ret == ESP_FAIL) {
              ESP_LOGE(TAG, "[UPDATE] Failed SET operation");
            }
            ret = drv_i2c_write(PIN_XIO_ADAPTER_ADDRESS + adapter, PIN_XIO_GPIOB_REGISTER, dataB);
            if (ret == ESP_FAIL) {
              ESP_LOGE(TAG, "[UPDATE] Failed SET operation"); //send error to monitoring
            }
          } else {
            sph_give(&buffer_lock);
          }
        } else {
          ESP_LOGE(TAG, "[UPDATE] Lock untaken ");
        }
      }
    }
  }
}

static void pin_xio_sleep(uint8_t ticks) {
    /*DEBUG*///ESP_LOGI(TAG, "sleep()");
    vTaskDelay(ticks / portTICK_RATE_MS);
    /*DEBUG*///ESP_LOGI(TAG, "~sleep()");
}
