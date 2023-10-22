//////////////////////////////////////////////////////////////////////////////////////////
//    ______ __     __ __             ____   ____ ______                                //
//   / ____// /    / //_/            / __ \ /  _//_  __/                                //
//  / /    / /    / ,<     ______   / /_/ / / /   / /                                   //
// / /___ / /___ / /| |   /_____/  / ____/_/ /   / /                                    //
// \____//_____//_/ |_|           /_/    /___/  /_/                                     //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include "clk_pit.h"
#include "params.h"

#define FOREVER while (1)
#define SUCCESS 0x00
#define FAILURE 0xFF
#define TRUE    0x01
#define FALSE   0x00

#define DONE     2  /*No more ticks left, ready to be acknowledged*/
#define ACTIVE   1  /*Still discounting ticks*/
#define INACTIVE 0  /*Event already acknowledged and ready to be overwritten*/

#define CLK_PIT_PRESCALER (CLK_PIT_MIN_INTERVAL/CLK_PIT_REFRESH_TIME)

/*** VARIABLES **************************************************************************/

static const char *TAG = "[CLK-PIT]";

// TODO: Change all this to buffer-circ and cuffer-frag
static clk_pit_event_t clk_pit_events[CLK_PIT_MAX_EVENTS];
static uint16_t clk_pit_event_idx;

/*** DECLARATIONS ***********************************************************************/

/*Function to be called on each cycle of refresh*/
static void clk_pit_update(void);
static void clk_pit_sleep(uint8_t ticks);

/*** DEFINITIONS ************************************************************************/

void clk_pit_init(void) {

}

/***
 *     _            _                 _           _       _     _             _   _
 *    | |          | |               | |         (_)     (_)   | |           | | (_)
 *    | |_ __ _ ___| | __    __ _  __| |_ __ ___  _ _ __  _ ___| |_ _ __ __ _| |_ _  ___  _ __
 *    | __/ _` / __| |/ /   / _` |/ _` | '_ ` _ \| | '_ \| / __| __| '__/ _` | __| |/ _ \| '_ \
 *    | || (_| \__ \   <   | (_| | (_| | | | | | | | | | | \__ \ |_| | | (_| | |_| | (_) | | | |
 *     \__\__,_|___/_|\_\   \__,_|\__,_|_| |_| |_|_|_| |_|_|___/\__|_|  \__,_|\__|_|\___/|_| |_|
 *
 *
 */

 /*Main executable task in freeRTOS*/
void clk_pit_task(void *params) {
    /*DEBUG*/ESP_LOGI(TAG, "task()");
    FOREVER{
        clk_pit_update();
        clk_pit_sleep(CLK_PIT_REFRESH_TIME);}

    /*DEBUG*/ESP_LOGI(TAG, "~task()");
}

void clk_pit_forget(uint16_t idx) {
    clk_pit_events[idx].status = INACTIVE;
}

static uint8_t clk_print_divider = 0;
static void clk_pit_update(void) {
    /*DEBUG*///ESP_LOGI(TAG, "update()");

    for (uint16_t i = 0; i < CLK_PIT_MAX_EVENTS; i++) {
        if (clk_pit_events[i].status == ACTIVE) {
            if (!(clk_print_divider % 50)) {
                ESP_LOGI(TAG, "Remaining for [%d]: [%d] ticks",
                        clk_pit_events[i].ref, clk_pit_events[i].ticks);
            }
            if (clk_pit_events[i].ticks-- == 0) {
                clk_pit_events[i].status = DONE;
            }
        }
    }
    clk_print_divider++;
    /*DEBUG*///ESP_LOGI(TAG, "~update()");
}

/***
 *                          _                             _             _       _   _
 *                         | |                           (_)           | |     | | (_)
 *      _____   _____ _ __ | |_     _ __ ___   __ _ _ __  _ _ __  _   _| | __ _| |_ _  ___  _ __
 *     / _ \ \ / / _ \ '_ \| __|   | '_ ` _ \ / _` | '_ \| | '_ \| | | | |/ _` | __| |/ _ \| '_ \
 *    |  __/\ V /  __/ | | | |_    | | | | | | (_| | | | | | |_) | |_| | | (_| | |_| | (_) | | | |
 *     \___| \_/ \___|_| |_|\__|   |_| |_| |_|\__,_|_| |_|_| .__/ \__,_|_|\__,_|\__|_|\___/|_| |_|
 *                                                         | |
 *                                                         |_|
 */

/*Delete all events from queue*/
void clk_pit_flush(void) {
    ESP_LOGI(TAG, "Flushing CLK EVENTS...");
    for (uint16_t k = 0; k < CLK_PIT_MAX_EVENTS; k++) {
        clk_pit_forget(k);
    }
    clk_pit_event_idx = 0;

}

/*Push a new event*/
uint8_t clk_pit_push(uint16_t ref, uint16_t hms) {
    /*DEBUG*///ESP_LOGI(TAG, "push()");

    if (clk_pit_event_idx > CLK_PIT_MAX_EVENTS - 1) {
        clk_pit_event_idx = 0;
    } // TODO: Handle overflows & fragmentation (look for next "INACTIVE element and overwrite")


    ESP_LOGI(TAG, "Ticks : [%d]", hms * CLK_PIT_PRESCALER);

    clk_pit_events[clk_pit_event_idx].ref = ref;
    clk_pit_events[clk_pit_event_idx].ticks = hms *CLK_PIT_PRESCALER;
    clk_pit_events[clk_pit_event_idx].status = ACTIVE;

    clk_pit_event_idx++;

    /*DEBUG*///ESP_LOGI(TAG, "~push()");

    return SUCCESS;
}

/*Detect next finished event*/
bool clk_pit_next(clk_pit_event_t *event) {
    /*DEBUG*///ESP_LOGI(TAG, "next()");

    for (uint16_t i = 0; i < CLK_PIT_MAX_EVENTS; i++) {
        if (clk_pit_events[i].status == DONE) {
            *event = clk_pit_events[i];

            clk_pit_events[i].status = INACTIVE;

            return SUCCESS;
        }
    }

    /*DEBUG*///ESP_LOGI(TAG, "~next()");

    return FAILURE;
}



static void clk_pit_sleep(uint8_t ticks) {
    /*DEBUG*///ESP_LOGI(TAG, "sleep(%d)", ticks / portTICK_RATE_MS);
    vTaskDelay(ticks / portTICK_RATE_MS);

    /*DEBUG*///ESP_LOGI(TAG, "~sleep()");
}
