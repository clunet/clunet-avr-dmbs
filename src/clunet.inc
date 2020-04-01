/* Copyright (c) 2018-2019 Sergey V. DUDANOV
   All rights reserved.

   This file is part of clunet-avr-dmbs project.
   https://github.com/clunet/clunet-avr-dmbs

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
   * Neither the name of the copyright holders nor the names of
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.	*/

#ifndef __CLUNET_INC__
#define __CLUNET_INC__

#include "clunet_config.h"
#include "sfr_macro.inc"

.macro tim_start tmp=DEFAULT_TMP_REGISTER
    sfr_ldi CLUNET_TIM_CTRL_REG, CLUNET_TIM_CTRL_VALUE, \tmp
.endm

.macro tim_stop
    sfr_st  CLUNET_TIM_CTRL_REG, __zero_reg__
.endm

.macro tim_ld dst:req
    sfr_ld  \dst, CLUNET_TIM_CNT_REG
.endm

.macro tim_st src:req
    sfr_st  CLUNET_TIM_CNT_REG, \src
.endm

.macro tim_arm periods:req clocks:req tmp=DEFAULT_TMP_REGISTER
    sfr_ldi CLUNET_TIM_CNT_REG, CLUNET_TIM_CAL_VALUE(\periods, \clocks), \tmp
.endm

.macro tim_rst_flg tmp=DEFAULT_TMP_REGISTER
    sfr_ldi CLUNET_TIM_FLAGS_REG, _BV(CLUNET_TIM_FLAGS_OVF_BIT), \tmp
.endm

.macro tim_eint tmp=DEFAULT_TMP_REGISTER
	sfr_sbi CLUNET_TIM_INT_REG, CLUNET_TIM_INT_OVF_BIT, \tmp
.endm

.macro tim_dint tmp=DEFAULT_TMP_REGISTER
    sfr_cbi CLUNET_TIM_INT_REG, CLUNET_TIM_INT_OVF_BIT, \tmp
.endm

.macro gpio_init tmp=DEFAULT_TMP_REGISTER
 #if defined(CLUNET_GPIO_TWO_PINS)
    ; setting write pin to output mode
    sfr_sbi CLUNET_DDRPORT(CLUNET_CONFIG_GPIO_WRITE_PORT), CLUNET_CONFIG_GPIO_WRITE_PIN, \tmp
    ; enabling internal pull-up resistor on the reading pin
    sfr_sbi CLUNET_OUTPORT(CLUNET_GPIO_READ_PORT), CLUNET_GPIO_READ_PIN, \tmp
 #endif
.endm

.macro gpio_deinit tmp=DEFAULT_TMP_REGISTER
 #if defined(CLUNET_GPIO_TWO_PINS)
    ; setting write pin to output mode
    sfr_cbi CLUNET_DDRPORT(CLUNET_CONFIG_GPIO_WRITE_PORT), CLUNET_CONFIG_GPIO_WRITE_PIN, \tmp
    ; enabling internal pull-up resistor on the reading pin
    sfr_cbi CLUNET_OUTPORT(CLUNET_GPIO_READ_PORT), CLUNET_GPIO_READ_PIN, \tmp
 #endif
.endm

.macro skip_bus_read_free tmp=DEFAULT_TMP_REGISTER
    sfr_sbis CLUNET_INPORT(CLUNET_GPIO_READ_PORT), CLUNET_GPIO_READ_PIN, \tmp
.endm

.macro skip_bus_read_busy tmp=DEFAULT_TMP_REGISTER
    sfr_sbic CLUNET_INPORT(CLUNET_GPIO_READ_PORT), CLUNET_GPIO_READ_PIN, \tmp
.endm

.macro skip_bus_write_free tmp=DEFAULT_TMP_REGISTER
 #if defined(CLUNET_GPIO_TWO_PINS)
    sfr_sbic CLUNET_OUTPORT(CLUNET_CONFIG_GPIO_WRITE_PORT), CLUNET_CONFIG_GPIO_WRITE_PIN, \tmp
 #else
    sfr_sbic CLUNET_DDRPORT(CLUNET_GPIO_READ_PORT), CLUNET_GPIO_READ_PIN, \tmp
 #endif
.endm

.macro skip_bus_write_busy tmp=DEFAULT_TMP_REGISTER
 #if defined(CLUNET_GPIO_TWO_PINS)
    sfr_sbis CLUNET_OUTPORT(CLUNET_CONFIG_GPIO_WRITE_PORT), CLUNET_CONFIG_GPIO_WRITE_PIN, \tmp
 #else
    sfr_sbis CLUNET_DDRPORT(CLUNET_GPIO_READ_PORT), CLUNET_GPIO_READ_PIN, \tmp
 #endif
.endm

.macro bus_write_busy tmp=DEFAULT_TMP_REGISTER
 #if defined(CLUNET_GPIO_TWO_PINS)
    sfr_sbi CLUNET_OUTPORT(CLUNET_CONFIG_GPIO_WRITE_PORT), CLUNET_CONFIG_GPIO_WRITE_PIN, \tmp
 #else
    sfr_sbi CLUNET_DDRPORT(CLUNET_GPIO_READ_PORT), CLUNET_GPIO_READ_PIN, \tmp
 #endif
.endm

.macro bus_write_free tmp=DEFAULT_TMP_REGISTER
 #if defined(CLUNET_GPIO_TWO_PINS)
    sfr_cbi CLUNET_OUTPORT(CLUNET_CONFIG_GPIO_WRITE_PORT), CLUNET_CONFIG_GPIO_WRITE_PIN, \tmp
 #else
    sfr_cbi CLUNET_DDRPORT(CLUNET_GPIO_READ_PORT), CLUNET_GPIO_READ_PIN, \tmp
 #endif
.endm

.macro exti_init tmp=DEFAULT_TMP_REGISTER
    sfr_sbr CLUNET_EXTI_CTRL_REG, CLUNET_EXTI_CTRL_MASK, \tmp
    sfr_sbi CLUNET_EXTI_INT_REG, CLUNET_EXTI_INT_BIT, \tmp
.endm

#endif  // __CLUNET_INC__
