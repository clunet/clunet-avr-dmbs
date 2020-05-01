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

/* Based on original <boot.h> header file from avr-libc library */

#ifndef __BOOT_INC__
#define __BOOT_INC__

#if !defined(__ASSEMBLER__) || !defined(__AVR__)
 #error "This file must use with AVR GCC assembler only."
#endif

#include "sfr_macro.inc"

; Check for SPM Control Register in MCU
#if defined(SPMCSR)
 #define __SPM_REG SPMCSR
#elif defined(SPMCR)
 #define __SPM_REG SPMCR
#else
 #error "MCU does not provide self-programming support."
#endif

; Check for SPM Enable bit.
#if defined(SPMEN)
 #define __SPM_ENABLE SPMEN
#elif defined(SELFPRGEN)
 #define __SPM_ENABLE SELFPRGEN
#else
 #error "Cannot find SPM enable bit definition."
#endif

; Create common bits definitions.
#if defined(ASB)
 #define __COMMON_ASB ASB
#elif defined(RWWSB)
 #define __COMMON_ASB RWWSB
#endif

#if defined(ASRE)
 #define __COMMON_ASRE ASRE
#elif defined(RWWSRE)
 #define __COMMON_ASRE RWWSRE
#endif

#if defined(RFLB)
 #define __COMMON_RFLB RFLB
#elif defined(BLBSET)
 #define __COMMON_RFLB BLBSET
#endif

; Check if the RWW section is busy.
.macro boot_skip_rww_busy tmp=DEFAULT_TMP_REGISTER
    sfr_sbis __SPM_REG, __COMMON_ASB, \tmp
.endm
.macro boot_skip_rww_free tmp=DEFAULT_TMP_REGISTER
    sfr_sbic __SPM_REG, __COMMON_ASB, \tmp
.endm

; Check if the SPM instruction is busy.
.macro boot_skip_spm_busy tmp=DEFAULT_TMP_REGISTER
    sfr_sbis __SPM_REG, __SPM_ENABLE, \tmp
.endm
.macro boot_skip_spm_free tmp=DEFAULT_TMP_REGISTER
    sfr_sbic __SPM_REG, __SPM_ENABLE, \tmp
.endm

; Wait while the SPM instruction is busy.
.macro boot_spm_busy_wait tmp=DEFAULT_TMP_REGISTER
0:  boot_skip_spm_free \tmp
    rjmp    0b
.endm

#define __BOOT_PAGE_ERASE    (_BV(__SPM_ENABLE) | _BV(PGERS))
#define __BOOT_PAGE_WRITE    (_BV(__SPM_ENABLE) | _BV(PGWRT))
#define __BOOT_PAGE_FILL     (_BV(__SPM_ENABLE))
#define __BOOT_RWW_ENABLE    (_BV(__SPM_ENABLE) | _BV(__COMMON_ASRE))
#define __BOOT_LOCK_BITS_SET (_BV(__SPM_ENABLE) | _BV(__COMMON_RFLB))

; Filling page buffer within 16-bit data.
; Data is 16-bit: R1:R0, address: PCWORD in ZH:ZL.
.macro boot_page_fill
    ldi     tmphi, __BOOT_PAGE_FILL     ; [1]
0:  ld      r0, X+                      ; [2] <- next word
    ld      r1, X+                      ; [2]
    rcall   btldr_safe_spm              ; [3+]
    adiw    ZL, 2                       ; [2]
    subi    byte_counter, 2             ; [1]
    brne    0b                          ; [1][2] -> next word
.endm

; Erasing page operation.
; The page address must be written to PCPAGE in RAMPZ:ZH:ZL.
; Other bits in the Z-pointer must be written to zero.
.macro boot_page_erase
    ldi     tmphi, __BOOT_PAGE_ERASE
    rcall   btldr_safe_spm
.endm

; Page write from page buffer.
; The page address must be written to PCPAGE in RAMPZ:ZH:ZL.
; Other bits in the Z-pointer must be written to zero.
.macro boot_page_write
    ldi     tmphi, __BOOT_PAGE_WRITE
    rcall   btldr_safe_spm
.endm

; RWW section read enable
.macro boot_rww_enable
#ifdef __COMMON_ASRE
    ldi     tmphi, __BOOT_RWW_ENABLE
    rcall   btldr_safe_spm
#endif
.endm

; From the mega16/mega128 data sheets (maybe others):

;   Bits by SPM To set the Boot Loader Lock bits, write the desired data to
;   R0, write "X0001001" to SPMCR and execute SPM within four clock cycles
;   after writing SPMCR. The only accessible Lock bits are the Boot Lock bits
;   that may prevent the Application and Boot Loader section from any
;   software update by the MCU.

;   If bits 5..2 in R0 are cleared (zero), the corresponding Boot Lock bit
;   will be programmed if an SPM instruction is executed within four cycles
;   after BLBSET and SPMEN (or SELFPRGEN) are set in SPMCR. The Z-pointer is 
;   don't care during this operation, but for future compatibility it is 
;   recommended to load the Z-pointer with $0001 (same as used for reading the 
;   Lock bits). For future compatibility It is also recommended to set bits 7, 
;   6, 1, and 0 in R0 to 1 when writing the Lock bits. When programming the 
;   Lock bits the entire Flash can be read during the operation.

.macro boot_lock_bits_set
    sbr     tmphi, 0b11000011
    mov     r0, tmphi
    zldi    1
    ldi     tmphi, __BOOT_LOCK_BITS_SET
    rcall   btldr_safe_spm
.endm

#endif // __BOOT_INC__
