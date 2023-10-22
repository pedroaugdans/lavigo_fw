#include "pin_xio.h"
#include "machines.h"
#include "params.h"
#include "port_xio.h"
#include "drv_locks.h"

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define UNREAD  01
#define READ    00

#define XIO_MAX_MSGS   64

/*Variables***********************************************************/
/*Circular buffer indexes*/
static port_status_t port_queued_msgs[XIO_MAX_MSGS];
/*xio_msg_sent points to the place of the buffer of the last message sent*/
static uint8_t xio_msg_sent = 0;
/*xio_msg_idx points to the place of the buffer of the last message logged*/
static uint8_t xio_msg_idx = 0;
/*xio_index_overflow amount of times a buffer difference has been higher than the buffer size*/
static uint8_t xio_index_overflow = 0;
/*Enable or disable port event logging*/
static uint8_t port_logging = TRUE;
SemaphoreHandle_t xio_msg_lock = NULL;

/*Port management variables*/
uint8_t hub_machines_input_ports_list[32];
uint8_t hub_machines_output_ports_list[32];
uint8_t hub_retrofit_input_ports_list[32];
uint8_t hub_retrofit_output_ports_list[32];
uint8_t fallback_for_machines[32] = {0};
uint8_t fallback_for_retrofit[32] = {0};

/*BUffer control variables*/
port_status_t hub_ports_status[PIN_EVT_MAX_PINS];
/*This variable is meant to become true each time the xio control deeper layer
finishes filling one of the ping-pong buffers, then, the filled buffer
becomes available for reading.*/
bool should_we_refresh_ports = TRUE;
SemaphoreHandle_t refresh_buffer_lock = NULL;
SemaphoreHandle_t buffer_read_lock = NULL;

//static bool ping_pong_idx = ping_pong_1;
static const char * TAG = "[PORT XIO]";


/*Declarations***********************************************************/
void port_unset_refresh();
bool port_get_refresh(void);

/***
 *     _                       _
 *    | |                     (_)
 *    | |     ___   __ _  __ _ _ _ __   __ _
 *    | |    / _ \ / _` |/ _` | | '_ \ / _` |
 *    | |___| (_) | (_| | (_| | | | | | (_| |
 *    \_____/\___/ \__, |\__, |_|_| |_|\__, |
 *                  __/ | __/ |         __/ |
 *                 |___/ |___/         |___/
 */

uint8_t set_logging(uint8_t should_i_log){
  if(should_i_log == TRUE) {
    ESP_LOGI(TAG,"Now logging port messages");
  } else {
    ESP_LOGI(TAG,"Now NOT logging port messages");
    xio_index_overflow = 0;
    xio_msg_idx = 0;
    xio_msg_sent = 0;
  }
  if(sph_step_retries(&xio_msg_lock)==TRUE){
    port_logging = should_i_log;
    sph_give(&xio_msg_lock);
  } else {
    ESP_LOGE(TAG,"[SET LOGGING] Port msg lock UNTAKEN");
  }
  return SUCCESS;
}

bool is_any_msg(void){
  bool msg_status_holder= FALSE;
  if (sph_step_retries(&xio_msg_lock) == pdTRUE) {
    if ((xio_msg_sent < xio_msg_idx) ||
    (xio_index_overflow > 0)) {
      msg_status_holder = TRUE;
    }
    else {
      msg_status_holder = FALSE;
    }
    sph_give(&xio_msg_lock);
  } else {
    ESP_LOGE(TAG,"[MSG CHECK] lock untaken");
  }
  return msg_status_holder;
}

uint8_t get_logging(void){
  uint8_t logging_handler = FALSE;
  if(sph_step_retries(&xio_msg_lock)==TRUE){
    logging_handler = port_logging;
    sph_give(&xio_msg_lock);
  } else {
    ESP_LOGE(TAG,"[SET LOGGING] Port msg lock UNTAKEN");
  }
  return logging_handler;
}


uint8_t get_next_xio_msg(port_status_t * port_status_to_return){
  uint8_t response = FAILURE;
  if (sph_step_retries(&xio_msg_lock) == pdTRUE) {
      if ((xio_msg_sent < xio_msg_idx) ||
              (xio_index_overflow > 0)) {
          port_status_to_return->port = port_queued_msgs[xio_msg_sent].port;
          port_status_to_return->status = port_queued_msgs[xio_msg_sent].status;
          port_status_to_return->timestamp = port_queued_msgs[xio_msg_sent].timestamp;

          ESP_LOGI(TAG, "[DeQueing msg], Idx [%d] / [%d]",xio_msg_sent,xio_msg_idx);
          if (xio_msg_sent >= (XIO_MAX_MSGS-1)) {
              xio_msg_sent = 0;
              xio_index_overflow--;
              ESP_LOGW(TAG,"DEOverflowing xio Queue [%d]",xio_index_overflow);
          }
        xio_msg_sent++;
        response = SUCCESS;
      } else {
        response = FAILURE;
      }
      sph_give(&xio_msg_lock);
  } else {
      ESP_LOGE(TAG, "[DEQ MSG] Lock untaken");
      response = FAILURE;
  }
  return response;
}

static void xio_push_msg(uint8_t toggled_port, bool new_value) {
  if(!get_logging()){
    return;
  }
    if (sph_step_retries(&xio_msg_lock) == pdTRUE) {
        port_queued_msgs[xio_msg_idx].port = toggled_port;
        port_queued_msgs[xio_msg_idx].status = new_value;
        port_queued_msgs[xio_msg_idx].timestamp = get_hub_timestamp();
        ESP_LOGI(TAG, "[Queing msg], Idx [%d]",xio_msg_idx);

        if (xio_msg_idx >= (XIO_MAX_MSGS-1)) {
            xio_msg_idx = 0;
            xio_index_overflow++;
            ESP_LOGW(TAG,"Overflowing xio Queue [%d]",xio_index_overflow);
        } else {
          xio_msg_idx++;
        }
        sph_give(&xio_msg_lock);
    } else {
        ESP_LOGE(TAG, "[Q MSG] Lock untaken");
    }
}







/***
 *     _____         _
 *    |_   _|       | |
 *      | | __ _ ___| | __
 *      | |/ _` / __| |/ /
 *      | | (_| \__ \   <
 *      \_/\__,_|___/_|\_\
 *
 *
 */
 void port_refresh(void){
   if(sph_step_retries(&buffer_read_lock) == TRUE){
     for(uint16_t k = 0;k<PIN_EVT_MAX_PINS;k++){
       hub_ports_status[k].has_been_read = UNREAD;
       hub_ports_status[k].last_idx = 0;
     }
     sph_give(&buffer_read_lock);
   } else {
     ESP_LOGE(TAG,"Port refresh failed");
   }
 }

 static void port_evt_update(void){
   if(port_get_refresh()){
     port_refresh();
     //ESP_LOGI(TAG,"Refreshing");
     port_unset_refresh();
   } else {
     vTaskDelay(10/portTICK_RATE_MS);
   }
 }

 void port_set_refresh(void){
   if(sph_step_retries(&refresh_buffer_lock) == TRUE){
     should_we_refresh_ports = TRUE;
     sph_give(&refresh_buffer_lock);
   }else {
     ESP_LOGE(TAG,"Port refresh failed");
   }
 }

 bool port_get_refresh(void){
   bool refresh_port_handle = FALSE;
   if(sph_step_retries(&refresh_buffer_lock) == TRUE){
     refresh_port_handle = should_we_refresh_ports;
     sph_give(&refresh_buffer_lock);
   }else {
     ESP_LOGE(TAG,"Port refresh failed");
   }
   return refresh_port_handle;
 }

 void port_unset_refresh(void){
   if(sph_step_retries(&refresh_buffer_lock) == TRUE){
     should_we_refresh_ports = FALSE;
     sph_give(&refresh_buffer_lock);
   }else {
     ESP_LOGE(TAG,"Port refresh failed");
   }
 }

static void fill_ports_lists(void){
  for (uint8_t k = 0; k < COLUMN_MAX; k++) {
      for (uint8_t j = 0; j < ROW_MAX; j++) {
        hub_machines_input_ports_list[k] = machines_assemble_port(j, k, IDIR, Resource);
        hub_machines_output_ports_list[k] = machines_assemble_port(j, k, ODIR, Resource);
        hub_retrofit_input_ports_list[k] = machines_assemble_port(j, k, IDIR, Retrofit);
        hub_retrofit_output_ports_list[k] = machines_assemble_port(j, k, ODIR, Retrofit);
      }
    }
}

void port_xio_init(void){
  assign_refresh_cv(port_set_refresh);
  fill_ports_lists();
  sph_create(&xio_msg_lock);
  sph_create(&refresh_buffer_lock);
  sph_create(&buffer_read_lock);
}

void port_xio_task(void *params){
  while(1){
    port_evt_update();
  }
}





/***
 *    ______          _                                                                 _
 *    | ___ \        | |                                                               | |
 *    | |_/ /__  _ __| |_   _ __ ___   __ _ _ __   __ _  __ _  ___ _ __ ___   ___ _ __ | |_
 *    |  __/ _ \| '__| __| | '_ ` _ \ / _` | '_ \ / _` |/ _` |/ _ \ '_ ` _ \ / _ \ '_ \| __|
 *    | | | (_) | |  | |_  | | | | | | (_| | | | | (_| | (_| |  __/ | | | | |  __/ | | | |_
 *    \_|  \___/|_|   \__| |_| |_| |_|\__,_|_| |_|\__,_|\__, |\___|_| |_| |_|\___|_| |_|\__|
 *                                                       __/ |
 *                                                      |___/
 */

 uint8_t port_look_for_status(uint8_t port,bool status){
   bool status_handler = false;
   if(sph_step_retries(&buffer_read_lock) == TRUE){
     if(hub_ports_status[port].has_been_read == READ) {
       sph_give(&buffer_read_lock);
       return FAILURE;
     }
     uint8_t k;
     for(k = hub_ports_status[port].last_idx;k < MAX_INPUT_TIMESTAMP; k++ ){
       pin_xio_get(PORT_TO_ADAPTER(port),PORT_TO_PIN(port),
       &status_handler,k);

       if(status_handler == status){
         hub_ports_status[port].last_idx = k;
         ESP_LOGI(TAG,"idx [%d]",k);
         xio_push_msg(port,status);
         sph_give(&buffer_read_lock);
         return SUCCESS;
       }
     }
     hub_ports_status[port].has_been_read = READ;
     sph_give(&buffer_read_lock);

     return FAILURE;
   } else {
     ESP_LOGE(TAG,"Port look for status untaken");
   }
   return FAILURE;
 }

 static uint8_t port_get (uint8_t port)__attribute__((unused));
 static uint8_t port_get (uint8_t port){
   return 1;
 }

 void port_set(uint8_t port, bool new_status){
   xio_push_msg(port,new_status);
   pin_xio_set(PORT_TO_ADAPTER(port), PORT_TO_PIN(port), new_status);
 }
