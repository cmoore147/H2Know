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

#include "m_ui.h"
#include "m_motion.h"
#include "m_motion_flash.h"
#include "ble_tms.h"
#include "drv_motion.h"
#include "sdk_errors.h"
#include "pca20020.h"
#define  NRF_LOG_MODULE_NAME "m_motion "
#include "nrf_log.h"
#include "macros_common.h"
#include "SEGGER_RTT.h"
#include "math.h"
#include "m_sound.h"



#define RAW_PARAM_NUM                 9     // Number of raw parameters (3 * acc + 3 * gyro + 3 * compass).
#define RAW_Q_FORMAT_ACC_INTEGER_BITS 6     // Number of bits used for integer part of raw data.
#define RAW_Q_FORMAT_GYR_INTEGER_BITS 11    // Number of bits used for integer part of raw data.
#define RAW_Q_FORMAT_CMP_INTEGER_BITS 12    // Number of bits used for integer part of raw data.

static ble_tms_t              m_tms;
static ble_tms_config_t     * m_config;
static const ble_tms_config_t m_default_config = MOTION_DEFAULT_CONFIG;

#define ACCEL_BUF_SIZE 10
#define ACCEL_HYST 0.1F //was .2

//################
#include "drv_speaker.h"
#include "MyVariable.h"
#include "app_timer.h"
#include "math.h"
APP_TIMER_DEF(drink_it);
int drinkTimeOut = 10000;
int buttonState = 0;
//#################3


double mag_buffer[ACCEL_BUF_SIZE];

//Used for EWMA low pass filter
double ewma_alpha = 0.6;
double prev_ewma = 0;

uint8_t acc_index = 0;

bool sign_state_pos = false;

uint32_t step_count = 0;

bool new_step = false;

//###################################################################
uint32_t process_bottle_data(int16_t x, int16_t y, int16_t z){
  
  int buttonTransition;

  if(buttonState){
    NRF_LOG_DEBUG("x = %d, y=%d, z =%d\r\n",x,y,z);
    if(abs(y)>2000){
        m_ui_led_constant_set(0,0,0);
        app_timer_stop(drink_it);
    }
    buttonTransition = 1;
  }

  if(!buttonState && buttonTransition){
    app_timer_start(drink_it,APP_TIMER_TICKS(drinkTimeOut),NULL);
    buttonTransition = 0;
    return 0;
  }
  
  return 0;
}    
//#############################^^^^^^^^^^^^###########################


static void drv_motion_evt_handler(drv_motion_evt_t const * p_evt, void * p_data, uint32_t size)
{
  switch (*p_evt)
  {
    case DRV_MOTION_EVT_RAW:
    {
      //NRF_LOG_INFO("DRV_MOTION_EVT_RAW:\r\n");

      APP_ERROR_CHECK_BOOL(size == sizeof(int32_t) * RAW_PARAM_NUM);

      //bottleOrientation data;
      double x;
      double y;
      double z;

      int32_t     * p_raw = (int32_t *)p_data;
      char buffer[24];

      //#################################################
     
            x =       (int16_t)(p_raw[3] >> RAW_Q_FORMAT_GYR_INTEGER_BITS);
            y =       (int16_t)(p_raw[4] >> RAW_Q_FORMAT_GYR_INTEGER_BITS);
            z =       (int16_t)(p_raw[5] >> RAW_Q_FORMAT_GYR_INTEGER_BITS);
      
      //sprintf(buffer, "Orrientation %.2f,%.2f,%.2f", 
      //                              x,
      //                              y,
      //                              z);

      //NRF_LOG_INFO(" %s\r\n", buffer);

      process_bottle_data(x,y,z);

      //#########################^^^^^^^####################


      // double x_buf;
      // double y_buf;
      // double z_buf;

      // //convert fixed point from MPU to floating point
      // x_buf = (double)p_raw[0];
      // x_buf = x_buf/(1<<16);
      
      // y_buf = (double)p_raw[1];
      // y_buf = y_buf/(1<<16);

      // z_buf = (double)p_raw[2];
      // z_buf = z_buf/(1<<16);

      // //format as string for debugging purposes and print accelerometer values to segger RTT console channel 1
      // sprintf(buffer, "ACCEL_DATA %.2f,%.2f,%.2f", x_buf,y_buf,z_buf);
      // NRF_LOG_INFO(" %s\r\n", buffer);

      // process_accel_data(x_buf,y_buf,z_buf);
    }
    break;

    case DRV_MOTION_EVT_PEDOMETER:
    {
        APP_ERROR_CHECK_BOOL(size == sizeof(unsigned long) * 2);

        ble_tms_pedo_t  data;
        unsigned long * p_pedo = (unsigned long *)p_data;

        data.steps   = p_pedo[0];
        data.time_ms = p_pedo[1];

        NRF_LOG_INFO("DRV_MOTION_EVT_PEDOMETER: %d steps %d ms\r\n", p_pedo[0],
                                                                      p_pedo[1]);
    }
    break;
    
  }
}


uint32_t m_motion_sleep_prepare(bool wakeup)
{
    return drv_motion_sleep_prepare(wakeup);
}


uint32_t m_motion_init(m_motion_init_t * p_params)
{
    uint32_t err_code;
    drv_motion_twi_init_t motion_params_mpu9250;
    drv_motion_twi_init_t motion_params_lis2dh12;

    static const nrf_drv_twi_config_t twi_config_mpu9250 =
    {
        .scl                = TWI_SCL,
        .sda                = TWI_SDA,
        .frequency          = NRF_TWI_FREQ_400K,
        .interrupt_priority = APP_IRQ_PRIORITY_LOW
    };

    static const nrf_drv_twi_config_t twi_config_lis2dh12 =
    {
        #if defined(THINGY_HW_v0_7_0)
            .scl = TWI_SCL,
            .sda = TWI_SDA,
        #elif  defined(THINGY_HW_v0_8_0)
            .scl = TWI_SCL,
            .sda = TWI_SDA,
        #elif  defined(THINGY_HW_v0_9_0)
            .scl = TWI_SCL,
            .sda = TWI_SDA,
        #else
            .scl = TWI_SCL_EXT,
            .sda = TWI_SDA_EXT,
        #endif
        .frequency          = NRF_TWI_FREQ_400K,
        .interrupt_priority = APP_IRQ_PRIORITY_LOW
    };

    NRF_LOG_INFO("Init \r\n");

    motion_params_mpu9250.p_twi_instance = p_params->p_twi_instance;
    motion_params_mpu9250.p_twi_cfg      = &twi_config_mpu9250;

    motion_params_lis2dh12.p_twi_instance = p_params->p_twi_instance;
    motion_params_lis2dh12.p_twi_cfg      = &twi_config_lis2dh12;

    err_code = drv_motion_init(drv_motion_evt_handler, &motion_params_mpu9250, &motion_params_lis2dh12);
    RETURN_IF_ERROR(err_code);

    return NRF_SUCCESS;
}
//#####################################################
static void drink_clock_handler(void * p_context){
    UNUSED_PARAMETER(p_context);
    m_ui_led_constant_set(255,165,100);
    //drv_speaker_tone_start(440,1000,50);
    drv_speaker_sample_play(6);
    /*
    //drv_speaker_sample_play(6);
    //drv_speaker_tone_start(261,100,100);//C
    //drv_speaker_tone_start(261,100,100);//C
    //drv_speaker_tone_start(329,1000,50);//E

    //drv_speaker_tone_start(220,1000,70);//A

    //drv_speaker_tone_start(184,1000,50);//F

    //drv_speaker_tone_start(220,1000,50);//A

    //drv_speaker_tone_start(195,1000,50);//G

    //drv_speaker_tone_start(246,1000,50);//B
    //app_timer_start(drink_it,APP_TIMER_TICKS(100),NULL);
    */
}
//##############################################

uint32_t raw_gyro_enable(void)
{
  uint32_t err_code;
  err_code = drv_motion_enable(DRV_MOTION_FEATURE_MASK_RAW_GYRO);

  //##############Drinking CountDown###############
  /*
  */
    err_code = app_timer_create(&drink_it,APP_TIMER_MODE_REPEATED,drink_clock_handler);
    APP_ERROR_CHECK(err_code);
    
    err_code = app_timer_start(drink_it,APP_TIMER_TICKS(drinkTimeOut),NULL);
    APP_ERROR_CHECK(err_code);
  //###########################################
  return err_code;
}

uint32_t raw_gyro_disable(void)
{
  uint32_t err_code;
  err_code = drv_motion_disable(DRV_MOTION_FEATURE_MASK_RAW_GYRO);
  return err_code;
}

uint32_t raw_motion_enable(void)
{
    uint32_t err_code;
    err_code = drv_motion_enable(DRV_MOTION_FEATURE_MASK_RAW_ACCEL);
    return err_code;
}

uint32_t raw_motion_disable(void)
{
    uint32_t err_code;
    err_code = drv_motion_disable(DRV_MOTION_FEATURE_MASK_RAW_ACCEL);
    return err_code;
}

uint32_t pedometer_enable(void)
{
  uint32_t err_code;
  err_code = drv_motion_enable(DRV_MOTION_FEATURE_MASK_PEDOMETER);
                APP_ERROR_CHECK(err_code);
  return err_code;
}

uint32_t pedometer_disable(void)
{
  uint32_t err_code;
  err_code = drv_motion_disable(DRV_MOTION_FEATURE_MASK_PEDOMETER);
                APP_ERROR_CHECK(err_code);
                return err_code;
}


uint32_t process_accel_data(double x, double y, double z)
{
    double mean_acc = 0;
    double normalized_accel = 0;

    char buffer[24];

    //NRF_LOG_INFO(" sample_index = %i \r\n", acc_index);

    //do the signal processing work to compute the steps

    //remove axes
    mag_buffer[acc_index] = sqrt((x*x) + (y*y) + (z*z));

    //compute the mean acceleration across the window
    for(int i = 0; i < ACCEL_BUF_SIZE; i++)
      mean_acc = mean_acc + mag_buffer[i];

    mean_acc = mean_acc / ACCEL_BUF_SIZE;

    //subtract mean from index zero -- this removes "DC" offset
    normalized_accel = mag_buffer[acc_index] - mean_acc;

    

    //print out this normalized acceleration for debug
    sprintf(buffer, "NORM_DATA %.2f", normalized_accel);
    NRF_LOG_INFO(" %s\r\n", buffer);

    //Filter noise with EWMA filter
    normalized_accel = normalized_accel*ewma_alpha + prev_ewma*(1 - ewma_alpha);
    sprintf(buffer, "EWMA_DATA %.2f", normalized_accel);
    NRF_LOG_INFO(" %s\r\n", buffer);

    prev_ewma = normalized_accel;

    //now, check for a zero crossing (negative to positive) and update history

    if(normalized_accel > ACCEL_HYST)
    {
      //if previous state was a negative sign below hysteresis window, step detected
      if(sign_state_pos == false)
      {
        sprintf(buffer, "STEP_DETECTED %i\r\n", step_count);
        NRF_LOG_INFO(" %s\r\n", buffer);
        step_count++;
        new_step = true;

        //even, LED is on. Odd, turn it off. Useful for debugging when not tethered to the debugger.
        if((step_count % 2) == 0)
           m_ui_led_constant_set(100,0,0);
        else
           m_ui_led_constant_set(0,0,0);
      	//_______________________________________________________
      }
        sign_state_pos = true;
    }
    else if(normalized_accel < (0-ACCEL_HYST))
    {
      sign_state_pos = false;
    }


    //update undex of circular buffer
    acc_index++;
    if(acc_index == 99)
        acc_index = 0;


    return 0;
}


