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

#ifndef __EEPROM_INC__
#define __EEPROM_INC__

#if !defined(__ASSEMBLER__) || !defined(__AVR__)
 #error "This file must use with AVR GCC assembler only."
#endif

#include "sfr_macro.inc"

#ifndef __BOOT_INC__
 #warning "If you using SPM write operations, include 'boot.inc' first."
#endif

#if !defined(EECR) && defined(DEECR)    /* AT86RF401 */
 #define EECR  DEECR
 #define EEARL DEEAR
 #define EEDR  DEEDR
#endif

#if !defined(EERE) && defined(EER)      /* AT86RF401 */
 #define EERE EER
#endif

#if !defined(EEWE) && defined(EEPE)     /* A part of Mega and Tiny */
 #define EEWE EEPE
#endif

#if !defined(EEWE) && defined(EEL)      /* AT86RF401 */
 #define EEWE EEL
#endif

#if !defined(EEMWE) && defined(EEMPE)   /* A part of Mega and Tiny */
 #define EEMWE EEMPE
#endif

#if !defined(EEMWE) && defined(EEU)     /* AT86RF401 */
 #define EEMWE EEU
#endif

#if defined (EEAR) && !defined (EEARL) && !defined (EEARH)
 #define EEARL EEAR
#endif

.macro eeprom_busy_wait tmp=DEFAULT_TMP_REGISTER
#if E2END
ebw\@:
 #if defined(__AVR_XMEGA__) && __AVR_XMEGA__
    sfr_sbic NVM_STATUS, NVM_NVMBUSY_bp, \tmp
 #elif defined (DEECR)
    sfr_sbic DEECR, BSY, \tmp
 #elif defined (EEPE)
    sfr_sbic EECR, EEPE, \tmp
 #else
    sfr_sbic EECR, EEWE, \tmp
 #endif
    rjmp     ebw\@
#endif
.endm

; Macro: 'eeprom_write_memblock'
; 'X': pointer to source in RAM
; 'Z': pointer to destination in EEPROM
; 'byte_counter': size of memory block
.macro eeprom_write_memblock tmp=DEFAULT_TMP_REGISTER
#ifdef __COMMON_ASRE
    boot_spm_busy_wait \tmp         ; [X]
#endif
ewm\@:
    eeprom_busy_wait \tmp           ; [X]<- next byte
    sfr_st  EEARL, ZL               ; [1]
#ifdef EEARH
 #if E2END > 0xFF
    sfr_st  EEARH, ZH               ; [1]
 #else
    sfr_st  EEARH, __zero_reg__     ; [1]
 #endif
#endif
    ld      \tmp, X+                ; [2]
    sfr_st  EEDR, \tmp              ; [1]
    sfr_sbi EECR, EEMWE, \tmp       ; [2]
    sfr_sbi EECR, EEWE, \tmp        ; [2+2]
#if E2END > 0xFF
    adiw    ZL, 1                   ; [2]
#else
    inc     ZL                      ; [1]
#endif
    dec     byte_counter            ; [1]
    brne    ewm\@                   ; [1][2]-> next byte
.endm

; Macro: 'eeprom_read_memblock'
; 'Z': pointer to source in EEPROM
; 'X': pointer to destination in RAM
; 'byte_counter': size of memory block
.macro eeprom_read_memblock tmp=DEFAULT_TMP_REGISTER
    eeprom_busy_wait \tmp           ; [X]
#if defined(EEARH) && E2END < 0x100
    sfr_st  EEARH, __zero_reg__     ; [1]
#endif
erm\@:
    sfr_st  EEARL, ZL               ; [1] <- next byte
#if E2END > 0xFF
    sfr_st  EEARH, ZH               ; [1]
#endif
	sfr_sbi EECR, EERE, \tmp        ; [2+4]
	sfr_ld  \tmp, EEDR              ; [1]
    st      X+, \tmp                ; [2]
#if E2END > 0xFF
    adiw    ZL, 1                   ; [2]
#else
    inc     ZL                      ; [1]
#endif
    dec     byte_counter            ; [1]
    brne    erm\@                   ; [1][2] -> next byte
.endm

#endif  // __EEPROM_INC__
