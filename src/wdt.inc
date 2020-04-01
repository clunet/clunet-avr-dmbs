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

#ifndef _WDT_INC_
#define _WDT_INC_

#if !defined(__ASSEMBLER__) || !defined(__AVR__)
#error "This file for use with AVR GCC assembler only."
#endif

#include "sfr_macro.inc"

#if defined(WDTCSR)
#define _WD_CONTROL_REG WDTCSR
#elif defined(WDTCR)
#define _WD_CONTROL_REG WDTCR
#else
#define _WD_CONTROL_REG WDT
#endif

#if defined(WDTOE)
#define _WD_CHANGE_BIT  WDTOE
#else
#define _WD_CHANGE_BIT  WDCE
#endif

.macro reboot_by_wdt tmp=DEFAULT_TMP_REGISTER
    cli                                                           ; [1]
    sfr_ldi _WD_CONTROL_REG, _BV(WDE) | _BV(_WD_CHANGE_BIT), \tmp ; [2]
    sfr_ldi _WD_CONTROL_REG, _BV(WDE), \tmp                       ; [2]
    rjmp    .-2                                                   ; [x]
.endm

.macro wdt_disable tmp=DEFAULT_TMP_REGISTER
    sfr_st  MCUSR, __zero_reg__                                   ; [1]
    sfr_ldi _WD_CONTROL_REG, _BV(WDE) | _BV(_WD_CHANGE_BIT), \tmp ; [2|3]
    sfr_st  _WD_CONTROL_REG, __zero_reg__                         ; [1|2]
.endm

#endif /* _WDT_INC_ */
