/*
 * LAVIGO PROJECT CODE
 * Each line should be prefixed with  *
 */

/*
 * File:   lavigo_debug.h
 * Author: independent contractor
 *
 * Created on May 18, 2019, 7:27 AM
 */

#ifndef LAVIGO_DEBUG_H
#define LAVIGO_DEBUG_H
#include "drv_uart.h"

#define HUB_PROMPT_STR      "hub> "

#define STRTCN_IDX "cmdstr" /*<!start command line */
#define DLTKYS_IDX "kysdlt" /*<!Delete keys */
#define CLRCNT_IDX "cntdlt" /*<!delete connection keys */
#define CLRALL_IDX "lnddlt" /*<!delete all machines */
#define SHWMCH_IDX "mchshw" /*<!show all machines */
#define EXCTST_IDX "exctst" /*<!test execution */
#define MNTTST_IDX "mtrtst" /*<!test monitoring */
#define RAMUSE_IDX "ramuse" /*<!show ram usage */
#define DPLTST_IDX "dpltst" /*<!test deployment */
#define MCHDLT_IDX "mchdlt" /*<!delete last machine */
#define MCHFIX_IDX "lndfix" /*<!fix all resources */
#define SETLY1_IDX "ly1set" /*<!start command line */
#define SETLY2_IDX "ly2set" /*<!start command line */
#define CLRLYT_IDX "lytclr" /*<!start command line */
#define LOGRST_IDX "logrst"
#define SFTRST_IDX "sftrst" /**/


void provisory_console_task(void *param);
void consoleInit();
#endif /* LAVIGO_DEBUG_H */
