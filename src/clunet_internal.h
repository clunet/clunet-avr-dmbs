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
   POSSIBILITY OF SUCH DAMAGE. */

#ifndef _CLUNET_INTERNAL_H_
#define _CLUNET_INTERNAL_H_

#include "clunet_config.h"

#ifndef CLUNET_CONFIG_PERIOD_US
# define CLUNET_CONFIG_PERIOD_US          64
#endif

#ifndef CLUNET_CONFIG_INTERFRAME_PERIODS
# define CLUNET_CONFIG_INTERFRAME_PERIODS 5
#endif

#if defined(CLUNET_CONFIG_GPIO_WRITE_PORT) && defined(CLUNET_CONFIG_GPIO_WRITE_PIN)
# define CLUNET_GPIO_TWO_PINS
#endif

#ifndef RAMSIZE
# define RAMSIZE (RAMEND - RAMSTART + 1)
#endif

#define CLUNET_ASM_START(sym)       sym:
#define CLUNET_ASM_END(sym)         .size sym, . - sym
#define CLUNET_ASM_EXPORT(sym)      .global sym

#define CLUNET_CONCAT2(a, b)        a ## b
#define CLUNET_CONCAT2I(a, b)       CLUNET_CONCAT2(a, b)
#define CLUNET_CONCAT3(a, b, c)     a ## b ## c
#define CLUNET_CONCAT3I(a, b, c)    CLUNET_CONCAT3(a, b, c)
#define CLUNET_OUTPORT(name)        CLUNET_CONCAT2(PORT, name)
#define CLUNET_INPORT(name)         CLUNET_CONCAT2(PIN, name)
#define CLUNET_DDRPORT(name)        CLUNET_CONCAT2(DDR, name)

#if !defined(CLUNET_CONFIG_DEVICE_ADDRESS) || (CLUNET_CONFIG_DEVICE_ADDRESS <= 0) || (CLUNET_CONFIG_DEVICE_ADDRESS > 255)
# error "CLUNET_CONFIG_DEVICE_ADDRESS must be defined (1-255) and unique in your network."
#endif
#if CLUNET_CONFIG_TX_PAYLOAD_SIZE > 250
# error "CLUNET_CONFIG_TX_PAYLOAD_SIZE couldn't be more than 250 bytes."
#elif CLUNET_CONFIG_RX_PAYLOAD_SIZE > 250
# error "CLUNET_CONFIG_RX_PAYLOAD_SIZE couldn't be more than 250 bytes."
#elif CLUNET_CONFIG_RX_PAYLOAD_SIZE < CLUNET_CONFIG_TX_PAYLOAD_SIZE
# warning "CLUNET_CONFIG_RX_PAYLOAD_SIZE couldn't be less than CLUNET_CONFIG_TX_PAYLOAD_SIZE. CLUNET_CONFIG_RX_PAYLOAD_SIZE was automatically increased to CLUNET_CONFIG_TX_PAYLOAD_SIZE."
# undef CLUNET_CONFIG_RX_PAYLOAD_SIZE
# define CLUNET_CONFIG_RX_PAYLOAD_SIZE CLUNET_CONFIG_TX_PAYLOAD_SIZE
#endif

// calculate timer prescaler and control value
#define CLUNET_TIM_FREQ (256 / (CLUNET_CONFIG_INTERFRAME_PERIODS) * 1000000 / (CLUNET_CONFIG_PERIOD_US))
#define CLUNET_TIM_PSC  (((F_CPU) + (CLUNET_TIM_FREQ) - 1) / (CLUNET_TIM_FREQ))
#if CLUNET_TIM_PSC > 1024
# error "MCU frequency or CLUNET period too high. Decrease it and try again."
#elif CLUNET_TIM_PSC > 256
# undef  CLUNET_TIM_PSC
# define CLUNET_TIM_PSC 1024
# define CLUNET_TIM_CTRL_VALUE      (7 << CLUNET_CONCAT3I(CS, CLUNET_CONFIG_TIM, 0))
#elif CLUNET_TIM_PSC > 128
# undef  CLUNET_TIM_PSC
# define CLUNET_TIM_PSC 256
# define CLUNET_TIM_CTRL_VALUE      (6 << CLUNET_CONCAT3I(CS, CLUNET_CONFIG_TIM, 0))
#elif CLUNET_TIM_PSC > 64
# undef  CLUNET_TIM_PSC
# define CLUNET_TIM_PSC 128
# define CLUNET_TIM_CTRL_VALUE      (5 << CLUNET_CONCAT3I(CS, CLUNET_CONFIG_TIM, 0))
#elif CLUNET_TIM_PSC > 32
# undef  CLUNET_TIM_PSC
# define CLUNET_TIM_PSC 64
# define CLUNET_TIM_CTRL_VALUE      (4 << CLUNET_CONCAT3I(CS, CLUNET_CONFIG_TIM, 0))
#elif CLUNET_TIM_PSC > 8
# undef  CLUNET_TIM_PSC
# define CLUNET_TIM_PSC 32
# define CLUNET_TIM_CTRL_VALUE      (3 << CLUNET_CONCAT3I(CS, CLUNET_CONFIG_TIM, 0))
#elif CLUNET_TIM_PSC > 1
# undef  CLUNET_TIM_PSC
# define CLUNET_TIM_PSC 8
# define CLUNET_TIM_CTRL_VALUE      (2 << CLUNET_CONCAT3I(CS, CLUNET_CONFIG_TIM, 0))
#else
# define CLUNET_TIM_CTRL_VALUE      (1 << CLUNET_CONCAT3I(CS, CLUNET_CONFIG_TIM, 0))
# warning "MCU frequency too low. Suggested to increase it or CLUNET period."
#endif
#undef CLUNET_TIM_FREQ

/* MCU specific configurations */

#if defined (__AVR_ATmega8__) || defined (__AVR_ATmega8A__) || defined (__AVR_ATmega16__) || defined (__AVR_ATmega16A__) || \
    defined (__AVR_ATmega32__) || defined (__AVR_ATmega32A__)

# if CLUNET_CONFIG_TIM != 0 && CLUNET_CONFIG_TIM != 2
#  error "Only timers 0 and 2 is supported."
# endif

# if CLUNET_CONFIG_TIM != 2
#  define CLUNET_PSC_SHRINK
# endif

# define CLUNET_TIM_CTRL_REG        CLUNET_CONCAT2I(TCCR, CLUNET_CONFIG_TIM)
# define CLUNET_TIM_CNT_REG         CLUNET_CONCAT2I(TCNT, CLUNET_CONFIG_TIM)
# define CLUNET_TIM_INT_REG         TIMSK
# define CLUNET_TIM_INT_OVF_BIT     CLUNET_CONCAT2I(TOIE, CLUNET_CONFIG_TIM)
# define CLUNET_TIM_FLAGS_REG       TIFR
# define CLUNET_TIM_FLAGS_OVF_BIT   CLUNET_CONCAT2I(TOV, CLUNET_CONFIG_TIM)
# define CLUNET_TIM_INT_OVF_VECTOR  CLUNET_CONCAT3I(TIMER, CLUNET_CONFIG_TIM, _OVF_vect)
# define CLUNET_GPIO_READ_PORT      D
# define CLUNET_GPIO_READ_PIN       2 + CLUNET_CONFIG_INT

# if CLUNET_CONFIG_INT > 1
#  error "MCU support only INT[0:1] external interrupts."
# endif

# define CLUNET_EXTI_CTRL_REG       MCUCR
# define CLUNET_EXTI_CTRL_MASK      (1 << CLUNET_CONCAT3I(ISC, CLUNET_CONFIG_INT, 0))
# define CLUNET_EXTI_INT_REG        GICR
# define CLUNET_EXTI_INT_BIT        CLUNET_CONCAT2I(INT, CLUNET_CONFIG_INT)
# define CLUNET_EXTI_INT_VECTOR     CLUNET_CONCAT3I(INT, CLUNET_CONFIG_INT, _vect)

#elif defined (__AVR_ATmega64__) || defined (__AVR_ATmega64A__) || defined (__AVR_ATmega128__) || defined (__AVR_ATmega128A__)

# if CLUNET_CONFIG_TIM != 0 && CLUNET_CONFIG_TIM != 2
#  error "Only timers 0 and 2 is supported."
# endif

# if CLUNET_CONFIG_TIM != 0
#  define CLUNET_PSC_SHRINK
# endif

# define CLUNET_TIM_CTRL_REG        CLUNET_CONCAT2I(TCCR, CLUNET_CONFIG_TIM)
# define CLUNET_TIM_CNT_REG         CLUNET_CONCAT2I(TCNT, CLUNET_CONFIG_TIM)
# define CLUNET_TIM_INT_REG         TIMSK
# define CLUNET_TIM_INT_OVF_BIT     CLUNET_CONCAT2I(TOIE, CLUNET_CONFIG_TIM)
# define CLUNET_TIM_FLAGS_REG       TIFR
# define CLUNET_TIM_FLAGS_OVF_BIT   CLUNET_CONCAT2I(TOV, CLUNET_CONFIG_TIM)
# define CLUNET_TIM_INT_OVF_VECTOR  CLUNET_CONCAT3I(TIMER, CLUNET_CONFIG_TIM, _OVF_vect)

# if CLUNET_CONFIG_INT < 4
#  define CLUNET_EXTI_CTRL_REG      EICRA
#  define CLUNET_GPIO_READ_PORT     D
# elif CLUNET_CONFIG_INT < 8
#  define CLUNET_EXTI_CTRL_REG      EICRB
#  define CLUNET_GPIO_READ_PORT     E
# else
#  error "MCU support only INT[0:7] external interrupts."
# endif

# define CLUNET_GPIO_READ_PIN       CLUNET_CONFIG_INT
# define CLUNET_EXTI_CTRL_MASK      (1 << CLUNET_CONCAT3I(ISC, CLUNET_CONFIG_INT, 0))
# define CLUNET_EXTI_INT_REG        EIMSK
# define CLUNET_EXTI_INT_BIT        CLUNET_CONCAT2I(INT, CLUNET_CONFIG_INT)
# define CLUNET_EXTI_INT_VECTOR     CLUNET_CONCAT3I(INT, CLUNET_CONFIG_INT, _vect)

#elif defined (__AVR_ATmega48__) || defined (__AVR_ATmega48A__) || defined (__AVR_ATmega48PA__) || defined (__AVR_ATmega48PB__) ||      \
      defined (__AVR_ATmega48P__) || defined (__AVR_ATmega88__) || defined (__AVR_ATmega88A__) || defined (__AVR_ATmega88P__) ||        \
      defined (__AVR_ATmega88PA__) || defined (__AVR_ATmega88PB__) || defined (__AVR_ATmega168__) || defined (__AVR_ATmega168A__) ||    \
      defined (__AVR_ATmega168P__) || defined (__AVR_ATmega168PA__) || defined (__AVR_ATmega168PB__) || defined (__AVR_ATmega328P__) || \
      defined (__AVR_ATmega328__)

# if defined (__AVR_ATmega48__) || defined (__AVR_ATmega48A__) || defined (__AVR_ATmega48PA__) || \
     defined (__AVR_ATmega48PB__) || defined (__AVR_ATmega48P__)
#  define CLUNET_RWW_WORKAROUND
# endif

# if CLUNET_CONFIG_TIM != 0 && CLUNET_CONFIG_TIM != 2
#  error "Only timers 0 and 2 is supported."
# endif

# if CLUNET_CONFIG_TIM != 2
#  define CLUNET_PSC_SHRINK
# endif

# define CLUNET_TIM_CTRL_REG        CLUNET_CONCAT3I(TCCR, CLUNET_CONFIG_TIM, B)
# define CLUNET_TIM_CNT_REG         CLUNET_CONCAT2I(TCNT, CLUNET_CONFIG_TIM)
# define CLUNET_TIM_INT_REG         CLUNET_CONCAT2I(TIMSK, CLUNET_CONFIG_TIM)
# define CLUNET_TIM_INT_OVF_BIT     CLUNET_CONCAT2I(TOIE, CLUNET_CONFIG_TIM)
# define CLUNET_TIM_FLAGS_REG       CLUNET_CONCAT2I(TIFR, CLUNET_CONFIG_TIM)
# define CLUNET_TIM_FLAGS_OVF_BIT   CLUNET_CONCAT2I(TOV, CLUNET_CONFIG_TIM)
# define CLUNET_TIM_INT_OVF_VECTOR  CLUNET_CONCAT3I(TIMER, CLUNET_CONFIG_TIM, _OVF_vect)
# define CLUNET_GPIO_READ_PORT      D
# define CLUNET_GPIO_READ_PIN       2 + CLUNET_CONFIG_INT

# if CLUNET_CONFIG_INT > 1
#  error "MCU support only INT[0:1] external interrupts."
# endif

# define CLUNET_EXTI_CTRL_REG       EICRA
# define CLUNET_EXTI_CTRL_MASK      (1 << CLUNET_CONCAT3I(ISC, CLUNET_CONFIG_INT, 0))
# define CLUNET_EXTI_INT_REG        EIMSK
# define CLUNET_EXTI_INT_BIT        CLUNET_CONCAT2I(INT, CLUNET_CONFIG_INT)
# define CLUNET_EXTI_INT_VECTOR     CLUNET_CONCAT3I(INT, CLUNET_CONFIG_INT, _vect)

#else
# error "MCU is not supported."
#endif

#ifdef CLUNET_PSC_SHRINK
# if CLUNET_TIM_PSC == 1024
#  undef  CLUNET_TIM_CTRL_VALUE
#  define CLUNET_TIM_CTRL_VALUE   (5 << CLUNET_CONCAT3I(CS, CLUNET_CONFIG_TIM, 0))
# elif CLUNET_TIM_PSC == 256 || CLUNET_TIM_PSC == 128
#  undef  CLUNET_TIM_PSC
#  define CLUNET_TIM_PSC 256
#  undef  CLUNET_TIM_CTRL_VALUE
#  define CLUNET_TIM_CTRL_VALUE   (4 << CLUNET_CONCAT3I(CS, CLUNET_CONFIG_TIM, 0))
# elif CLUNET_TIM_PSC == 64 || CLUNET_TIM_PSC == 32
#  undef  CLUNET_TIM_PSC
#  define CLUNET_TIM_PSC 64
#  undef  CLUNET_TIM_CTRL_VALUE
#  define CLUNET_TIM_CTRL_VALUE   (3 << CLUNET_CONCAT3I(CS, CLUNET_CONFIG_TIM, 0))
# endif
#endif

#define CLUNET_T(periods) ((((F_CPU) + (CLUNET_TIM_PSC) / 2) / (CLUNET_TIM_PSC) * (CLUNET_CONFIG_PERIOD_US) * (periods) / 100000 + 5) / 10)
#define CLUNET_TIM_CAL_VALUE(periods, cycles) (256 - CLUNET_T(periods) + ((cycles) + (CLUNET_TIM_PSC) / 2) / (CLUNET_TIM_PSC))
#define CLUNET_RAMPZ (defined(__AVR_HAVE_RAMPZ__) && FLASHEND > 0xFFFF)

#endif // _CLUNET_INTERNAL_H_
