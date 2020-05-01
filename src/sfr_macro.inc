/* Copyright (c) 2018-2020 Sergey V. DUDANOV
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

#ifndef __SFR_MACRO_INC__
#define __SFR_MACRO_INC__

#if !defined(__ASSEMBLER__) || !defined(__AVR__)
 #error "This file for use with AVR GCC assembler only."
#endif

#include <avr/sfr_defs.h>

#ifndef DEFAULT_TMP_REGISTER
 #error "Define 'DEFAULT_TMP_REGISTER' first. Must be high (16..31)."
#elif DEFAULT_TMP_REGISTER < 16 || DEFAULT_TMP_REGISTER > 31
 #error "'DEFAULT_TMP_REGISTER' must be high register (16..31)."
#endif

; AVR GCC ABI registers definitions
.equ arg0lo, 24
.equ arg0hi, 25
.equ arg1lo, 22
.equ arg1hi, 23
.equ arg2lo, 20
.equ arg2hi, 21
.equ arg3lo, 18
.equ arg3hi, 19
.equ arg4lo, 16
.equ arg4hi, 17
.equ arg5lo, 14
.equ arg5hi, 15
.equ ret0lo, 24
.equ ret0hi, 25

.macro sfr_ld dst:req sfr:req
    .if _SFR_IO_REG_P(\sfr)
        in      \dst, _SFR_IO_ADDR(\sfr)    ; [1]
    .else
        lds     \dst, _SFR_MEM_ADDR(\sfr)   ; [2]
    .endif
.endm

.macro sfr_st sfr:req src:req
    .if _SFR_IO_REG_P(\sfr)
        out     _SFR_IO_ADDR(\sfr), \src    ; [1]
    .else
        sts     _SFR_MEM_ADDR(\sfr), \src   ; [2]
    .endif
.endm

.macro sfr_ldi sfr:req value:req tmp=DEFAULT_TMP_REGISTER
    .if \tmp < 16 || \tmp > 31
        .error "'tmp' must be high (16..31)."
    .endif
    ldi     \tmp, \value                ; [1]
    sfr_st  \sfr, \tmp                  ; [1|2]
.endm

.macro sfr_sbi sfr:req bit:req tmp=DEFAULT_TMP_REGISTER
    .if ((\sfr) < 0x20 + __SFR_OFFSET)
        sbi     _SFR_IO_ADDR(\sfr), \bit    ; [2]
    .else
        .if \tmp < 16 || \tmp > 31
            .error "'tmp' must be high (16..31)."
        .endif
        sfr_ld  \tmp, \sfr                  ; [1|2]
        sbr     \tmp, _BV(\bit)             ; [1]
        sfr_st  \sfr, \tmp                  ; [1|2]
    .endif
.endm

.macro sfr_cbi sfr:req bit:req tmp=DEFAULT_TMP_REGISTER
    .if (\sfr) < 0x20 + __SFR_OFFSET
        cbi     _SFR_IO_ADDR(\sfr), \bit    ; [2]
    .else
        .if \tmp < 16 || \tmp > 31
            .error "'tmp' must be high (16..31)."
        .endif
        sfr_ld  \tmp, \sfr                  ; [1|2]
        cbr     \tmp, _BV(\bit)             ; [1]
        sfr_st  \sfr, \tmp                  ; [1|2]
    .endif
.endm

.macro sfr_sbis sfr:req bit:req tmp=DEFAULT_TMP_REGISTER
    .if (\sfr) < 0x20 + __SFR_OFFSET
        sbis    _SFR_IO_ADDR(\sfr), \bit    ; [2|3][1]
    .else
        sfr_ld  \tmp, \sfr                  ; [1|2]
        sbrs    \tmp, \bit                  ; [2|3][1]
    .endif
.endm

.macro sfr_sbic sfr:req bit:req tmp=DEFAULT_TMP_REGISTER
    .if (\sfr) < 0x20 + __SFR_OFFSET
        sbic    _SFR_IO_ADDR(\sfr), \bit    ; [2|3][1]
    .else
        sfr_ld  \tmp, \sfr                  ; [1|2]
        sbrc    \tmp, \bit                  ; [2|3][1]
    .endif
.endm

.macro sfr_sbr sfr:req sbr:req tmp=DEFAULT_TMP_REGISTER
    .if \tmp < 16 || \tmp > 31
        .error "'tmp' must be high (16..31)."
    .endif
    sfr_ld  \tmp, \sfr                  ; [1|2]
    sbr     \tmp, \sbr                  ; [1]
    sfr_st  \sfr, \tmp                  ; [1|2]
.endm

.macro sfr_cbr sfr:req cbr:req tmp=DEFAULT_TMP_REGISTER
    .if \tmp < 16 || \tmp > 31
        .error "'tmp' must be high (16..31)."
    .endif
    sfr_ld  \tmp, \sfr                  ; [1|2]
    cbr     \tmp, \cbr                  ; [1]
    sfr_st  \sfr, \tmp                  ; [1|2]
.endm

.macro sfr_cbr_sbr sfr:req cbr:req sbr:req tmp=DEFAULT_TMP_REGISTER
    .if \tmp < 16 || \tmp > 31
        .error "'tmp' must be high (16..31)."
    .endif
    sfr_ld  \tmp, \sfr                  ; [1|2]
    cbr     \tmp, \cbr                  ; [1]
    sbr     \tmp, \sbr                  ; [1]
    sfr_st  \sfr, \tmp                  ; [1|2]
.endm

; Load XH:XL registers
.macro xldi int16:req
    ldi     XL, lo8(\int16)             ; [1]
    ldi     XH, hi8(\int16)             ; [1]
.endm

; Load YH:YL registers
.macro yldi int16:req
    ldi     YL, lo8(\int16)             ; [1]
    ldi     YH, hi8(\int16)             ; [1]
.endm

; Load ZH:ZL registers
.macro zldi int16:req
    ldi     ZL, lo8(\int16)             ; [1]
    ldi     ZH, hi8(\int16)             ; [1]
.endm

.macro arg0ldi16 int16:req
    ldi     arg0lo, lo8(\int16)         ; [1]
    ldi     arg0hi, hi8(\int16)         ; [1]
.endm

.macro arg0ldi24 int24:req
    arg0ldi16   \int24                  ; [2]
    ldi         arg1lo, hlo8(\int24)    ; [1]
.endm

; Load RAMPZ:ZH:ZL registers
.macro zldi_P pm:req tmp=DEFAULT_TMP_REGISTER
    ldi     ZL, lo8(\pm)                ; [1]
    ldi     ZH, hi8(\pm)                ; [1]
#if defined(__AVR_HAVE_RAMPZ__) && FLASHEND > 0xFFFF
    sfr_ldi RAMPZ, hlo8(\pm), \tmp      ; [2]
#endif
.endm

; Load RAMPZ:ZH registers
.macro zhldi_P pm:req tmp=DEFAULT_TMP_REGISTER
    ldi     ZH, hi8(\pm)                ; [1]
#if defined(__AVR_HAVE_RAMPZ__) && FLASHEND > 0xFFFF
    sfr_ldi RAMPZ, hlo8(\pm), \tmp      ; [2]
#endif
.endm

.macro stack_init tmp=DEFAULT_TMP_REGISTER
    sfr_ldi SPL, lo8(RAMEND), \tmp      ; [2]
 #if defined(SPH) && RAMEND > 0xFF
    sfr_ldi SPH, hi8(RAMEND), \tmp      ; [2]
 #endif
.endm

#endif  // __SFR_MACRO_INC__
