/*
  Copyright (c) 2010 - 2017, Nordic Semiconductor ASA
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

  2. Redistributions in binary form, except as embedded into a Nordic
     Semiconductor ASA integrated circuit in a product or a software update for
     such product, must reproduce the above copyright notice, this list of
     conditions and the following disclaimer in the documentation and/or other
     materials provided with the distribution.

  3. Neither the name of Nordic Semiconductor ASA nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

  4. This software, with or without modification, must only be used with a
     Nordic Semiconductor ASA integrated circuit.

  5. Any software provided in binary form under this license must not be reverse
     engineered, decompiled, modified and/or disassembled.

  THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
  OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include "app_util_platform.h"
#include "m_sound.h"
#include "drv_speaker.h"
#include "drv_mic.h"
#include "ble_tss.h"
#define  NRF_LOG_MODULE_NAME "m_sound       "
#include "nrf_log.h"
#include "macros_common.h"


#define SPEAKER_FREQ 440 //speaker frequency in hertz
#define SPEAKER_LEN 250 // speaker duration in millisceconds
#define SPEAKER_VOL 99 //maximum loudness

static ble_tss_t              m_tss;  /**< Structure to identify the Thingy Sound Service. */


void beep(void)
{
    drv_speaker_sample_play(0);
    return;
}
//#########################################################################
/*
event handler needed for the speaker to perform operation
*/
static void drv_speaker_evt_handler(drv_speaker_evt_t evt)
{
    switch(evt)
    {
        case DRV_SPEAKER_EVT_FINISHED:
        {
            NRF_LOG_DEBUG("drv_speaker_evt_handler: DRV_SPEAKER_EVT_FINISHED\r\n");
            (void)ble_tss_spkr_stat_set(&m_tss, BLE_TSS_SPKR_STAT_FINISHED);
        }
        break;
        //
        case DRV_SPEAKER_EVT_BUFFER_WARNING:
        {
            NRF_LOG_WARNING("drv_speaker_evt_handler: DRV_SPEAKER_EVT_BUFFER_WARNING\r\n");
            (void)ble_tss_spkr_stat_set(&m_tss, BLE_TSS_SPKR_STAT_BUFFER_WARNING);
        }
        break;
        //
        case DRV_SPEAKER_EVT_BUFFER_READY:
        {
            NRF_LOG_DEBUG("drv_speaker_evt_handler: DRV_SPEAKER_EVT_BUFFER_READY\r\n");
            (void)ble_tss_spkr_stat_set(&m_tss, BLE_TSS_SPKR_STAT_BUFFER_READY);
        }
        break;
        //
        default:
            APP_ERROR_CHECK_BOOL(false);
            break;
    }
}
//#####################################################



uint32_t m_sound_init(void)
{
    uint32_t           err_code;
    drv_speaker_init_t speaker_init;
   speaker_init.evt_handler = drv_speaker_evt_handler;


    NRF_LOG_INFO("Sound_init \r\n");

    return err_code = drv_speaker_init(&speaker_init);
}
