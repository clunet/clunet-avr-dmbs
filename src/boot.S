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

#include <avr/io.h>
#include "clunet.h"
#include "clunet_internal.h"

; check for bootloader section word address in flash memory. depends on MCU and fuses.
#if !defined(CLUNET_BOOTLOADER_ADDRESS) || !CLUNET_BOOTLOADER_ADDRESS
 #error "Bootloader address must be defined."
#endif

; default duration of "silence" in CLUNET as retry
#ifndef CLUNET_CONFIG_BOOTLOADER_READ_TIMEOUT
 #define CLUNET_CONFIG_BOOTLOADER_READ_TIMEOUT  200
#endif

; default number of retries to receive BTLDR_ENTER packet
#ifndef CLUNET_CONFIG_BOOTLOADER_READ_RETRIES
 #define CLUNET_CONFIG_BOOTLOADER_READ_RETRIES  5
#endif

; support EEPROM globally
#define CLUNET_BTLDR_EEPROM_SUPPORT (CLUNET_CONFIG_BOOTLOADER_EEPROM_READ_SUPPORT || CLUNET_CONFIG_BOOTLOADER_EEPROM_WRITE_SUPPORT)

; support DATA globally
#define CLUNET_BTLDR_DATA_SUPPORT   (CLUNET_CONFIG_BOOTLOADER_DATA_READ_SUPPORT || CLUNET_CONFIG_BOOTLOADER_DATA_WRITE_SUPPORT)

; checking for EEPROM
#if !defined(E2END) && CLUNET_BTLDR_EEPROM_SUPPORT
 #warning "Device does not have EEPROM available. Disabling EEPROM support."
 #undef  CLUNET_BTLDR_EEPROM_SUPPORT
 #define CLUNET_BTLDR_EEPROM_SUPPORT                    0
 #undef  CLUNET_CONFIG_BOOTLOADER_EEPROM_READ_SUPPORT
 #define CLUNET_CONFIG_BOOTLOADER_EEPROM_READ_SUPPORT   0
 #undef  CLUNET_CONFIG_BOOTLOADER_EEPROM_WRITE_SUPPORT
 #define CLUNET_CONFIG_BOOTLOADER_EEPROM_WRITE_SUPPORT  0
#endif

; checking for bitlock write support
#if !defined(BLBSET) && CLUNET_CONFIG_BOOTLOADER_BITLOCK_WRITE_SUPPORT
 #warning "Device has read-only BITLOCK support. Disable write support."
 #undef  CLUNET_CONFIG_BOOTLOADER_BITLOCK_WRITE_SUPPORT
 #define CLUNET_CONFIG_BOOTLOADER_BITLOCK_WRITE_SUPPORT     0
#endif

#if CLUNET_BTLDR_EEPROM_SUPPORT && CLUNET_BTLDR_DATA_SUPPORT
 #define CLUNET_BTLDR_DATA_IDX  2
 #if CLUNET_CONFIG_BOOTLOADER_DATA_WRITE_SUPPORT
  .equ btldr_cmd_limit, CLUNET_BTLDR_MEMORY_EXECUTE(2)
 #else
  .equ btldr_cmd_limit, CLUNET_BTLDR_MEMORY_WRITE(2)
 #endif
#elif CLUNET_BTLDR_DATA_SUPPORT
 #define CLUNET_BTLDR_DATA_IDX  1
 #if CLUNET_CONFIG_BOOTLOADER_DATA_WRITE_SUPPORT
  .equ btldr_cmd_limit, CLUNET_BTLDR_MEMORY_EXECUTE(1)
 #else
  .equ btldr_cmd_limit, CLUNET_BTLDR_MEMORY_WRITE(1)
 #endif
#elif CLUNET_BTLDR_EEPROM_SUPPORT
 #if CLUNET_CONFIG_BOOTLOADER_EEPROM_WRITE_SUPPORT
  .equ btldr_cmd_limit, CLUNET_BTLDR_MEMORY_EXECUTE(1)
 #else
  .equ btldr_cmd_limit, CLUNET_BTLDR_MEMORY_WRITE(1)
 #endif
#else
 .equ btldr_cmd_limit, CLUNET_BTLDR_MEMORY_READ(1)
#endif

; **** REGISTERS DEFINITION ****
; may be low registers (except r1:r0, it using for SPM)
.equ __zero_reg__,  2
.equ tmplo,         3
.equ crc,           4
; must be high registers
#define DEFAULT_TMP_REGISTER 16
.equ tmphi,         DEFAULT_TMP_REGISTER
.equ client,        17
.equ retries_reg,   18
.equ bit_counter,   19
.equ byte_value,    20
.equ byte_index,    21
.equ byte_counter,  byte_index

; **** INCLUDES ****

#include "sfr_macro.inc"
#include "boot.inc"
#if CLUNET_BTLDR_EEPROM_SUPPORT
 #include "eeprom.inc"
#endif
#include "wdt.inc"
#include "clunet.inc"

#ifndef lo8
 #define lo8(val)   ((val) & 255)
#endif
#ifndef hi8
 #define hi8(val)   (((val) >> 8) & 255)
#endif
#ifndef hlo8
 #define hlo8(val)  (((val) >> 16) & 255)
#endif
#ifndef hhi8
 #define hhi8(val)  (((val) >> 24) & 255)
#endif

; bootloader delay conversion to timer's overflows
.equ _timer_overflows, (((((F_CPU) + (CLUNET_TIM_PSC) / 2) / (CLUNET_TIM_PSC) + 128) / 256 * (CLUNET_CONFIG_BOOTLOADER_READ_TIMEOUT) + 1500) / 1000)
.if _timer_overflows < 2
 .warning "bootloader delay too short and increased to minimum possible value."
 .equ _timer_overflows, 2
.elseif _timer_overflows > 0x10000
 .warning "bootloader delay too long and decreased to maximum possible value."
 .equ _timer_overflows, 0x10000
.endif

; flash memory parameters
.equ _flash_size, FLASHEND + 1
.equ _flash_page_size, SPM_PAGESIZE
.equ _btldr_address, CLUNET_BOOTLOADER_ADDRESS
.equ _btldr_size, _flash_size - _btldr_address

; memory sections addresses
.equ _flash_start, 0
.equ _flash_end, FLASHEND
.equ _flash_write_start, 0
.equ _flash_write_end, _btldr_address - 1
.equ _ram_start, RAMSTART
.equ _ram_end, RAMEND
.equ _ram_size, _ram_end - _ram_start + 1
#if CLUNET_BTLDR_EEPROM_SUPPORT
 .equ _eeprom_start, 0
 .equ _eeprom_end, E2END
#endif
#if CLUNET_BTLDR_DATA_SUPPORT
 .equ _data_start, 0
 .equ _data_end, RAMEND
#endif

; PC WORD MASK
.equ _pcword_mask, SPM_PAGESIZE - 1
.equ _pcword_mask_zl, lo8(_pcword_mask)
.equ _pcword_mask_zh, hi8(_pcword_mask)

; required stack size
.equ _nested_calls, 2
.if _flash_size > 0x20000
 .equ _stack_size, _nested_calls * 3
.else
 .equ _stack_size, _nested_calls * 2
.endif

.if _ram_size - _stack_size >= 256
 .equ _buffer_size, 256
.else
 .equ _buffer_size, _ram_size - _stack_size
.endif

.equ _max_data_size, _buffer_size - 10   ; maximum data size in packet (6 bytes for header, 4 for address)

; jump to reset vector macro
.macro jmp_to_app
#if CLUNET_CONFIG_BOOTLOADER_CHECK_RESET_VECTOR
 #ifdef CLUNET_RWW_WORKAROUND
    zldi    btldr_main - 2      ; [2]
 #else
    zldi    0                   ; [2]
 #endif
    lpm     XL, Z+              ; [3]
    lpm     XH, Z               ; [3]
    adiw    XL, 1               ; [2]
    breq    btldr_restart       ; [1][2] => BAD: NO USER PROGRAM (FLASH ERASED). RESTART BOOTLOADER.
#endif
#ifdef CLUNET_RWW_WORKAROUND
    rjmp    btldr_main - 2      ; [2] => OK: JUMP TO TO WORKAROUND RESET VECTOR (LAST WORD IN APP SECTION)
#else
    rjmp    _btldr_size         ; [2] => OK: JUMP TO RESET VECTOR THROUTH PC OVERFLOW
#endif
.endm

; PGM read macro
.macro lpgm reg:req
#if CLUNET_RAMPZ
    elpm    \reg, Z+    ; [3]
#else
    lpm     \reg, Z+    ; [3]
#endif
.endm

.section .bootloader,"ax",@progbits

; ****** START OF MAIN PROGRAM ******

CLUNET_ASM_EXPORT(btldr_main)
CLUNET_ASM_START(btldr_main)

    ; init block
    clr     __zero_reg__    ; [1]
#if CLUNET_CONFIG_BOOTLOADER_WDT_DISABLE
    wdt_disable             ; [4|7]
#endif
    ;sfr_ldi CLKPR, _BV(CLKPCE)
    ;sfr_ldi CLKPR, _BV(CLKPS1)
    gpio_init               ; [0|4]
    stack_init              ; [2|4]
    tim_start               ; [2]
    yldi    RAMSTART        ; [2] set Y pointer to start of RAM

    ; transmit broadcast event about bootloader start and wait for request from client.
    ; on success - enter to bootloader mode, else - start user application.

btldr_restart:
    std     Y+4, __zero_reg__                                   ; [2]
    ldi     client, CLUNET_ADDRESS_BROADCAST                    ; [1] initially set client address to broadcast
    ldi     retries_reg, CLUNET_CONFIG_BOOTLOADER_READ_RETRIES  ; [1]
    ldi     arg0lo, CLUNET_SYSTEM_REBOOT                        ; [1]
    rcall   btldr_send                                          ; [x]
0:  rcall   btldr_recv                                          ; [x] <-
    brtc    1f                                                  ; [1][2] ->
    cpi     ret0lo, CLUNET_BTLDR_ENTER                          ; [1]
    brne    1f                                                  ; [1][2] ->
    ldd     client, Y+2                                         ; [2] store client address
    rjmp    3f                                                  ; [2] -> enter to bootloader mode and answer to 'CLUNET_BTLDR_ENTER' command
1:  dec     retries_reg                                         ; [1]
    brne    0b                                                  ; [1][2] -> try to receive new message again.
7:  jmp_to_app                                                  ; [2] ==> start user app.

    ; BOOTLOADER MODE
    ; an endless loop of receiving and replying to messages until the 'CLUNET_BTLDR_EXIT' message is received.

btldr_send_error:
    std     Y+4, __zero_reg__                   ; [2]
btldr_send_response:
    rcall   btldr_send                          ; [3]
0:  rcall   btldr_recv                          ; [3]
    brtc    0b                                  ; [1][2] -> ERROR: wrong packet.

    cpi     ret0lo, CLUNET_BTLDR_EXIT           ; [1]
    brlo    btldr_send_error                    ; [1][2] -> ERROR: command not supported.
    breq    7b                                  ; [1][2] ==> start user app...

; EN: ENTERING TO BOOTLOADER MODE AND SENDING BASE INFORMATION
; RU: ВХОД В РЕЖИМ ЗАГРУЗЧИКА И ОТПРАВКА ОСНОВНОЙ ИНФОРМАЦИИ

    cpi     ret0lo, CLUNET_BTLDR_ENTER          ; [1]
3:  xldi    RAMSTART + 5                        ; [2] set X pointer to payload data in packet
    brne    2f                                  ; [1][2] -> next command

    zldi_P  pm_hwinfo                           ; [2]
btldr_send_payload:
    lpgm    byte_counter                        ; [3]
    std     Y+4, byte_counter                   ; [2]
    tst     byte_counter                        ; [1]
    breq    btldr_send_response                 ; [1][2] -> OK: it's error answer. send it.
btldr_send_pgm_block:
    lpgm    tmplo                               ; [3]
    st      X+, tmplo                           ; [2]
    dec     byte_counter                        ; [1]
    brne    btldr_send_pgm_block                ; [1][2] -> next byte
    rjmp    btldr_send_response                 ; [2] -> start TX

#if CLUNET_CONFIG_BOOTLOADER_FUSES_READ_SUPPORT

; EN: SUPPORT FOR READ FUSES AND BITLOCK
; RU: ПОДДЕРЖКА ЧТЕНИЯ FUSE И BITLOCK

2:  cpi     ret0lo, CLUNET_BTLDR_READ_FUSES     ; [1] <- next command
    brne    2f                                  ; [1][2] -> next command
    zldi    0                                   ; [2]
    ldi     byte_counter, 4                     ; [1]
    ldi     tmphi, __BOOT_LOCK_BITS_SET         ; [1]
 #if CLUNET_CONFIG_BOOTLOADER_EEPROM_WRITE_SUPPORT
    eeprom_busy_wait   tmplo                    ; [X]
 #endif
 #ifndef CLUNET_RWW_WORKAROUND
    boot_spm_busy_wait tmplo                    ; [X]
 #endif
0:  sfr_st  __SPM_REG, tmphi                    ; [1] <- next byte
    lpm     tmplo, Z+                           ; [3]
    st      X+, tmplo                           ; [2]
    dec     byte_counter                        ; [1]
    brne    0b                                  ; [1][2] -> next byte
    rjmp    btldr_send_address                  ; [2] -> GOOD: send answer.

#endif

2:  ldd     byte_counter, Y+4                   ; [2] take payload size

#if CLUNET_CONFIG_BOOTLOADER_BITLOCK_WRITE_SUPPORT

; EN: WRITING BITLOCK SUPPORT (USE WITH CARE!)
; RU: ПОДДЕРЖКА ЗАПИСИ BITLOCK (ПРИМЕНЯТЬ С ОСТОРОЖНОСТЬЮ!)

    cpi     ret0lo, CLUNET_BTLDR_BITLOCK_WRITE  ; [1]
    brne    2f                                  ; [1][2] -> next command
    cpi     byte_counter, 1                     ; [1]
    brne    btldr_send_error                    ; [1][2] -> ERROR: wrong request. must be 1 byte of data.
    ld      tmphi, X                            ; [2]
    boot_lock_bits_set                          ; [x]
    rjmp    btldr_send_response                 ; [2] -> GOOD: send pong answer.

#endif

; EN: IS COMMAND FOR MEMORY OPERATIONS?
; RU: КОМАНДА ОПЕРАЦИИ С ПАМЯТЬЮ?

2:  ; low bound command check
    cpi     ret0lo, CLUNET_BTLDR_MEMORY_READ(0)             ; [1]
    brlo    btldr_send_error                                ; [1][2] -> no supported command. send error.
    ; high bound command check
    cpi     ret0lo, btldr_cmd_limit                         ; [1]
    brsh    btldr_send_error                                ; [1][2] -> no supported command. send error.
    ; checking for minimum payload size (at least 4 bytes)
    subi    byte_counter, 4                                 ; [1]
    brsh    2f                                              ; [1][2] -> work command. payload size >= 4

; EN: COMMAND FOR MEMORY RANGE REQUEST (ANSWER FROM TABLE)
; RU: КОМАНДА ЗАПРОСА ДИАПАЗОНА ПАМЯТИ (ОТВЕТ ИЗ ТАБЛИЦЫ)

    zldi_P  pm_answer_table - CLUNET_BTLDR_MEMORY_READ(0)   ; [1]
    ; load pointer increment for command from PGM table
    add     ZL, ret0lo                                      ; [1]
    adc     ZH, __zero_reg__                                ; [1]
    lpgm    tmplo                                           ; [3]
    ; get PGM pointer to answer template
    add     ZL, tmplo                                       ; [1]
    adc     ZH, __zero_reg__                                ; [1]
    rjmp    btldr_send_payload                              ; [2] -> jump to send PGM payload routine

; EN: COMMAND FOR MEMORY OPERATION
; RU: КОМАНДА ОПЕРАЦИИ С ПАМЯТЬЮ

    ; Take 24-bit address to RAMPZ:ZH:ZL
2:  ldd     ZL, Y+5                                 ; [2]
    ldd     ZH, Y+6                                 ; [2]
#if defined(EIND) || CLUNET_RAMPZ
    ldd     tmplo, Y+7                              ; [2]
 #if CLUNET_RAMPZ
    sfr_st  RAMPZ, tmplo                            ; [1]
 #endif
#endif

    cpi     byte_counter, 1                         ; [2][1]
    brsh    3f                                      ; [0][2] -> address + some of payload data in message

; EN: COMMANDS FOR MEMORY OPERATIONS WITH 32-BIT ADDRESS ONLY
; RU: КОМАНДЫ ОПЕРАЦИЙ С ПАМЯТЬЮ, СОДЕРЖАЩИЕ ТОЛЬКО 32-БИТНЫЙ АДРЕС

#if CLUNET_CONFIG_BOOTLOADER_FLASH_EXEC_SUPPORT

; EN: SUPPORT FOR CODE EXECUTION FROM FLASH MEMORY
; RU: ПОДДЕРЖКА ВЫПОЛНЕНИЯ КОДА С FLASH ПАМЯТИ

    cpi     ret0lo, CLUNET_BTLDR_MEMORY_EXECUTE(0)  ; [1]
    brne    2f                                      ; [1][2] -> OK: next command
 #if defined(EIND)
    sfr_st  EIND, tmplo                             ; [1]
    eijmp                                           ; [2] => jump to custom address
 #else
    ijmp                                            ; [2] => jump to custom address
 #endif

#endif // CLUNET_CONFIG_BOOTLOADER_FLASH_EXEC_SUPPORT

; EN: SUPPORT FOR WRITE PAGES OF FLASH MEMORY
; RU: ПОДДЕРЖКА ЗАПИСИ СТРАНИЦ FLASH ПАМЯТИ

2:  cpi     ret0lo, CLUNET_BTLDR_MEMORY_WRITE(0)    ; [1]
    brne    7f                                      ; [1][2] -> OK: no more commands with 4 bytes of payload
    
    ; check for high bounds of page address
    cpi     ZL, lo8(_btldr_address)                 ; [1]
    ldi     tmphi, hi8(_btldr_address)              ; [1]
    cpc     ZH, tmphi                               ; [1]
#if CLUNET_RAMPZ
    ldi     tmphi, hlo8(_btldr_address)             ; [1]
    cpc     tmplo, tmphi                            ; [1]
#endif
    brsh    7f                                      ; [1][2] -> BAD: write address is out of bounds

    ; clear word address bits (datasheet require)
    cbr     ZL, _pcword_mask_zl                     ; [1]
.if _pcword_mask_zh != 0
    cbr     ZH, _pcword_mask_zh                     ; [1]
.endif
    
    boot_page_erase                                 ; [X] erase page
    boot_page_write                                 ; [X] write page buffer to page
#ifndef CLUNET_RWW_WORKAROUND
    boot_rww_enable                                 ; [X] enable rww section
#endif
    rjmp    btldr_send_response                     ; [2] -> OK: send answer

; EN: COMMANDS FOR MEMORY OPERATIONS WITH 32-BIT ADDRESS + EXTRA DATA
; RU: КОМАНДЫ ОПЕРАЦИЙ С ПАМЯТЬЮ, СОДЕРЖАЩИЕ 32-БИТНЫЙ АДРЕС И ПРОЧИЕ ДАННЫЕ

3:  adiw    XL, 4                                   ; [2]

; 'RAMPZ:ZH:ZL' points to memory address
; 'X' points to data in message
; 'byte_counter' is size of data

; EN: SUPPORT FOR FILL PAGE BUFFER OF FLASH MEMORY
; RU: ПОДДЕРЖКА ЗАПОЛНЕНИЯ БУФЕРА СТРАНИЦЫ FLASH ПАМЯТИ

    cpi     ret0lo, CLUNET_BTLDR_MEMORY_FILL(0)     ; [1]
#if CLUNET_CONFIG_BOOTLOADER_FLASH_READ_SUPPORT || CLUNET_BTLDR_EEPROM_SUPPORT || CLUNET_BTLDR_DATA_SUPPORT
    brne    2f                                      ; [1][2] -> skip command
#else
    brne    7f                                      ; [1][2] -> skip command
#endif
    sbrc    byte_counter, 0                         ; [2][1]
7:  rjmp    btldr_send_error                        ; [0][2] -> BAD: odd data size
    boot_page_fill                                  ; [X]
btldr_send_address:
    ldi     tmphi, 4                                ; [1]
    std     Y+4, tmphi                              ; [2]
8:  rjmp    btldr_send_response                     ; [2]

#if CLUNET_CONFIG_BOOTLOADER_EEPROM_WRITE_SUPPORT

; EN: SUPPORT FOR WRITE EEPROM MEMORY
; RU: ПОДДЕРЖКА ЗАПИСИ EEPROM ПАМЯТИ

2:  cpi     ret0lo, CLUNET_BTLDR_MEMORY_WRITE(1)    ; [1] <- next command
 #if CLUNET_CONFIG_BOOTLOADER_EEPROM_READ_SUPPORT || CLUNET_CONFIG_BOOTLOADER_FLASH_READ_SUPPORT || CLUNET_BTLDR_DATA_SUPPORT
    brne    2f                                      ; [1][2] -> next command
 #else
    brne    7b                                      ; [1][2] -> no more similar commands: send error
 #endif
    eeprom_write_memblock                           ; [X]
    rjmp    btldr_send_address                      ; [2] -> OK: send answer

#endif // CLUNET_CONFIG_BOOTLOADER_EEPROM_WRITE_SUPPORT


#if CLUNET_CONFIG_BOOTLOADER_DATA_WRITE_SUPPORT

; EN: SUPPORT FOR WRITE DATA MEMORY (ALL REGISTERS AND SRAM, USE WITH CARE!)
; RU: ПОДДЕРЖКА ЗАПИСИ DATA ПАМЯТИ (ВСЕ РЕГИСТРЫ И ОЗУ, ПРИМЕНЯТЬ С ОСТОРОЖНОСТЬЮ!)

2:  cpi     ret0lo, CLUNET_BTLDR_MEMORY_WRITE(CLUNET_BTLDR_DATA_IDX)    ; [1] <- next command
 #if CLUNET_CONFIG_BOOTLOADER_EEPROM_READ_SUPPORT || CLUNET_CONFIG_BOOTLOADER_FLASH_READ_SUPPORT || CLUNET_CONFIG_BOOTLOADER_DATA_READ_SUPPORT
    brne    2f                                                          ; [1][2] -> next command
 #else
    brne    7b                                                          ; [1][2] -> no more similar commands: send error
 #endif
0:  ld      tmplo, X+                                                   ; [2] <- next byte
    st      Z+, tmplo                                                   ; [2]
    dec     byte_counter                                                ; [1]
    brne    0b                                                          ; [1][2] -> next byte
    rjmp    btldr_send_address                                          ; [2] -> OK: send answer

#endif // CLUNET_CONFIG_BOOTLOADER_DATA_WRITE_SUPPORT

#if CLUNET_CONFIG_BOOTLOADER_FLASH_READ_SUPPORT || CLUNET_CONFIG_BOOTLOADER_EEPROM_READ_SUPPORT || CLUNET_CONFIG_BOOTLOADER_DATA_READ_SUPPORT

; EN: COMMANDS FOR MEMORY READ OPERATIONS
; RU: КОМАНДЫ ОПЕРАЦИЙ ЧТЕНИЯ ПАМЯТИ

2:  ld      byte_counter, X                         ; [2] take read size
    cpi     byte_counter, _max_data_size + 1        ; [1]
    brsh    7b                                      ; [1][2] -> BAD: too more data requested...
    subi    byte_counter, -4                        ; [1]
    std     Y+4, byte_counter                       ; [2] store payload size of answer
    subi    byte_counter, 4                         ; [1]
    breq    8b                                      ; [1][2] -> zero size request. answer with address only...

 #if CLUNET_CONFIG_BOOTLOADER_FLASH_READ_SUPPORT

; EN: SUPPORT FOR READ OF FLASH MEMORY
; RU: ПОДДЕРЖКА ЧТЕНИЯ FLASH ПАМЯТИ

    cpi     ret0lo, CLUNET_BTLDR_MEMORY_READ(0)     ; [1] <- next command
  #if CLUNET_CONFIG_BOOTLOADER_EEPROM_READ_SUPPORT || CLUNET_CONFIG_BOOTLOADER_DATA_READ_SUPPORT
    brne    2f                                      ; [1][2] -> next command
  #else
    brne    7b                                      ; [1][2] -> no more reading commands: send error
  #endif
    rjmp    btldr_send_pgm_block                    ; [2] -> jump to sending flash memblock routine

 #endif // CLUNET_CONFIG_BOOTLOADER_FLASH_READ_SUPPORT

 #if CLUNET_CONFIG_BOOTLOADER_EEPROM_READ_SUPPORT

; EN: SUPPORT FOR READ OF EEPROM MEMORY
; RU: ПОДДЕРЖКА ЧТЕНИЯ EEPROM ПАМЯТИ

 2: cpi     ret0lo, CLUNET_BTLDR_MEMORY_READ(1)     ; [1] <- next command
  #if CLUNET_CONFIG_BOOTLOADER_DATA_READ_SUPPORT
    brne    2f                                      ; [1][2] -> next command
  #else
    brne    7b                                      ; [1][2] -> no more reading commands: send error
  #endif
    eeprom_read_memblock                            ; [X]
    rjmp    btldr_send_response                     ; [2] -> reading complete: send answer
 
 #endif // CLUNET_CONFIG_BOOTLOADER_EEPROM_READ_SUPPORT

 #if CLUNET_CONFIG_BOOTLOADER_DATA_READ_SUPPORT

; EN: SUPPORT FOR READ DATA MEMORY (ALL REGISTERS AND SRAM)
; RU: ПОДДЕРЖКА ЧТЕНИЯ DATA ПАМЯТИ (ВСЕ РЕГИСТРЫ И ОЗУ)

2:  cpi     ret0lo, CLUNET_BTLDR_MEMORY_READ(CLUNET_BTLDR_DATA_IDX) ; [1] <- next command
    brne    7b                                                      ; [1][2] -> no more reading commands: send error
0:  ld      tmplo, Z+                                               ; [2] <- next byte
    st      X+, tmplo                                               ; [2]
    dec     byte_counter                                            ; [1]
    brne    0b                                                      ; [1][2] -> next byte
    rjmp    btldr_send_response                                     ; [2] -> reading complete: send answer

 #endif // CLUNET_CONFIG_BOOTLOADER_DATA_READ_SUPPORT

#endif // CLUNET_CONFIG_BOOTLOADER_FLASH_READ_SUPPORT || CLUNET_CONFIG_BOOTLOADER_EEPROM_READ_SUPPORT || CLUNET_CONFIG_BOOTLOADER_DATA_READ_SUPPORT

CLUNET_ASM_END(btldr_main)

; EN: SUBROUTINES
; RU: ПОДПРОГРАММЫ

; btldr_safe_spm: safely run spm instruction, in [tmphi] must be correct operation value
; RETURN: none

CLUNET_ASM_START(btldr_safe_spm)

#if CLUNET_CONFIG_BOOTLOADER_EEPROM_WRITE_SUPPORT
    eeprom_busy_wait   tmplo    ; [X]
#endif
#ifndef CLUNET_RWW_WORKAROUND
    boot_spm_busy_wait tmplo    ; [X]
#endif
    sfr_st  __SPM_REG, tmphi    ; [1]
    spm                         ; [X]
    ret                         ; [4]

CLUNET_ASM_END(btldr_safe_spm)

; btldr_send: send message until success
; RETURN: none

CLUNET_ASM_START(btldr_send)

    ; PREPARE MESSAGE (HEADER, CHECKSUM, SIZE)
    
    std     Y+1, client                                         ; [2] set 'dst' to current client
    ldi     tmphi, CLUNET_CONFIG_DEVICE_ADDRESS                 ; [1]
    std     Y+2, tmphi                                          ; [2] set 'src' to device address
    sbr     arg0lo, CLUNET_MESSAGE_TYPE_MASK                    ; [1] set 'cmd' attributes 'system response'
    std     Y+0, arg0lo                                         ; [2] set 'prio' to max (2 high bits is set by previous instruction)
    std     Y+3, arg0lo                                         ; [2] set 'cmd'
    rcall   btldr_checksum                                      ; [3]
    st      Z+, crc                                             ; [2] set 'crc'
#if lo8(RAMSTART)
    subi    ZL, lo8(RAMSTART)                                   ; [1]
#endif
    mov     tmplo, ZL                                           ; [1] in 'tmplo' store size of packet

    ; INIT TRANSMIT

3:  mov     byte_counter, tmplo                                 ; [1]
    ldi     bit_counter, 2                                      ; [1]
    movw    ZL, YL                                              ; [2]

    ; WAITING BUS

    rcall   btldr_wait_interframe                               ; [x]

    ;  start transmit through 5T or with another device

0:  tim_ld  tmphi                                               ; [1] <-
    cpi     tmphi, CLUNET_T(CLUNET_CONFIG_INTERFRAME_PERIODS)   ; [1]
    skip_bus_read_busy                                          ; [2][1][1]
    brlo    0b                                                  ; [0][1][2] ->

    ; MAIN TRANSMIT LOOP

2:  ld      byte_value, Z+                                      ; [2] <- next byte
    ; 1T('0') or 3T('1') dominant signal block
1:  bus_write_busy                                              ; [2] <- next bit
    tim_st  __zero_reg__                                        ; [1]
0:  tim_ld  tmphi                                               ; [1] <-
    cpi     tmphi, CLUNET_T(1)                                  ; [1]
    sbrc    byte_value, 7                                       ; [2][1]
    cpi     tmphi, CLUNET_T(3)                                  ; [0][1]
    brlo    0b                                                  ; [1][2] ->
    ; 1T recessive signal block with collision detection
    bus_write_free                                              ; [2]
    tim_st  __zero_reg__                                        ; [1]
    clt                                                         ; [1]
0:  skip_bus_read_busy                                          ; [2][1] <-
    set                                                         ; [0][1]
    tim_ld  tmphi                                               ; [1]
    cpi     tmphi, CLUNET_T(1)                                  ; [1]
    brlo    0b                                                  ; [1][2] ->
    brtc    3b                                                  ; [1][2] -> ERROR: collision. transmit again.
    
    lsl     byte_value                                          ; [1]
    dec     bit_counter                                         ; [1]
    brne    1b                                                  ; [1][2] -> next bit
    ldi     bit_counter, 8                                      ; [1]
    dec     byte_counter                                        ; [1]
    brne    2b                                                  ; [1][2] -> next byte

    ret                                                         ; [4]
    
CLUNET_ASM_END(btldr_send)

; btldr_recv: received only system requests
; RETURN: [T flag] - result of operation (TRUE if success)
;         [ret0lo] - command number (without type bits)

CLUNET_ASM_START(btldr_recv)
    
    clt                                                         ; [1]
7:  rcall   btldr_wait_interframe                               ; [x]
    
    ; waiting for dominant signal

.if _timer_overflows > 256
9:  xldi    _timer_overflows                                    ; [2]
.else
9:  ldi     XL, lo8(_timer_overflows)                           ; [1]
.endif
0:  sfr_ldi CLUNET_TIM_FLAGS_REG, _BV(CLUNET_TIM_FLAGS_OVF_BIT) ; [1] <- clear overflow and decrement counter
.if _timer_overflows > 256
    sbiw    XL, 1                                               ; [2]
.else
    dec     XL                                                  ; [1]
.endif
    breq    7f                                                  ; [1][2] -> ERROR: too long time no any message
1:  sfr_sbic CLUNET_TIM_FLAGS_REG, CLUNET_TIM_FLAGS_OVF_BIT     ; [3][2] <- bus free
    rjmp    0b                                                  ; [0][2] -> clear overflow and decrement counter
    skip_bus_read_busy                                          ; [2][1]
    rjmp    1b                                                  ; [0][2] -> bus free
    
    ldi     bit_counter, 2                                      ; [1]
    clr     byte_index                                          ; [1]
    movw    ZL, YL                                              ; [2]
    
    ; reading bit
8:  tim_st  __zero_reg__                                        ; [1|2]
0:  tim_ld  tmphi                                               ; [1|2]
    cpi     tmphi, CLUNET_T(CLUNET_CONFIG_INTERFRAME_PERIODS)   ; [1]
    brsh    7b                                                  ; [1][2] -> ERROR: too long dominant signal. try again.
    skip_bus_read_free                                          ; [2][1]
    rjmp    0b                                                  ; [0][2] ->
    
    tim_st  __zero_reg__                                        ; [1|2] reset timer
    cpi     tmphi, CLUNET_T(2)                                  ; [1]
    rol     byte_value                                          ; [1]
    
    dec     bit_counter                                         ; [1]
    brne    4f                                                  ; [1][2] -> next bit
    
    com     byte_value                                          ; [1]
    st      Z+, byte_value                                      ; [2]
    
    cpi     byte_index, 4                                       ; [1]
    brlo    3f                                                  ; [1][2] -> too short packet for analyze. continue.
    brne    2f                                                  ; [1][2] -> checking length of extended packet.
    
    ; checking for dst address
    ldd     tmphi, Y+1                                          ; [2] dst
    cpi     tmphi, CLUNET_CONFIG_DEVICE_ADDRESS                 ; [1]
    brne    7f                                                  ; [1][2] -> ERROR: wrong dst address.
    
    ; checking for src address
    ldd     tmphi, Y+2                                          ; [2] src
    cpi     tmphi, CLUNET_CONFIG_DEVICE_ADDRESS                 ; [1]
    breq    7f                                                  ; [1][2] -> ERROR: src address is our.
    cpi     tmphi, CLUNET_ADDRESS_BROADCAST                     ; [1]
    breq    7f                                                  ; [1][2] -> ERROR: src is broadcast.
    
    cpi     client, CLUNET_ADDRESS_BROADCAST                    ; [1]
    breq    0f                                                  ; [1][2] -> client not set yet
    
    cpse    tmphi, client                                       ; [2][1]
7:  ret                                                         ; [0][4] -> ERROR: message not from client.
    
    ; check for system request message
0:  ldd     ret0lo, Y+3                                         ; [2]
    sbrc    ret0lo, CLUNET_MESSAGE_SYSTEM_BIT                   ; [1][1][2]
    sbrc    ret0lo, CLUNET_MESSAGE_RESPONSE_BIT                 ; [2][1][0]
    ret                                                         ; [0][4][4] -> ERROR: non-system or response (event) message.
    
    ; checking for end of data message
2:  ldd     tmphi, Y+4                                          ; [2] size
    subi    tmphi, -5                                           ; [1]
    cp      byte_index, tmphi                                   ; [1]
    breq    5f                                                  ; [1][2] -> OK: data message received
    
3:  inc     byte_index                                          ; [1]
    ldi     bit_counter, 8                                      ; [1]

    ; reading short recessive
4:  tim_ld  tmphi                                               ; [1]
    cpi     tmphi, CLUNET_T(CLUNET_CONFIG_INTERFRAME_PERIODS)   ; [1]
    brsh    9b                                                  ; [1][2] -> ERROR: too long recessive signal. try again.
    skip_bus_read_busy                                          ; [2][1]
    rjmp    4b                                                  ; [0][2] ->
    rjmp    8b                                                  ; [2] -> next byte
    
5:  rcall   btldr_checksum                                      ; [3]
    cpse    byte_value, crc                                     ; [2][1]
    ret                                                         ; [0][4] -> ERROR: CRC wrong
    cbr     ret0lo, CLUNET_MESSAGE_TYPE_MASK                    ; [1]
    set                                                         ; [1]
    ret                                                         ; [4]
    
CLUNET_ASM_END(btldr_recv)

; btldr_wait_interframe: infinite wait for interframe space
; RETURN: none

CLUNET_ASM_START(btldr_wait)

    skip_bus_read_free                                          ; [2][1] <- reset timer if bus is busy
btldr_wait_interframe:
    tim_st  __zero_reg__                                        ; [0][1]
    tim_ld  tmphi                                               ; [1]
    cpi     tmphi, CLUNET_T(CLUNET_CONFIG_INTERFRAME_PERIODS)   ; [1]
    brlo    btldr_wait                                          ; [1][2] -> reset timer if bus is busy
    tim_st  __zero_reg__                                        ; [1|2]
    ret                                                         ; [4]

CLUNET_ASM_END(btldr_wait)

; btldr_checksum: calculate checksum of the message in beginning of SRAM.
; RETURN: [crc]: CRC-8 MAXIM, [ZH:ZL]: points to msg.crc

CLUNET_ASM_START(btldr_checksum)

    ldd     byte_counter, Y+4   ; [0][2] msg->size
    subi    byte_counter, -4    ; [1]
    clr     crc                 ; [1]
    ldi     tmphi, 0b10001100   ; [1]
    zldi    RAMSTART + 1        ; [2]
0:  ld      tmplo, Z+           ; [2] <- next byte
    eor     crc, tmplo          ; [1]
    ldi     bit_counter, 8      ; [1]
1:  lsr     crc                 ; [1] <- next bit
    brcc    2f                  ; [1][2]
    eor     crc, tmphi          ; [1][0]
2:  dec     bit_counter         ; [1]
    brne    1b                  ; [1][2] -> next bit
    dec     byte_counter        ; [1]
    brne    0b                  ; [1][2] -> next byte
    ret                         ; [4]

CLUNET_ASM_END(btldr_checksum)

; PROGMEM DATA

CLUNET_ASM_START(pm_hwinfo)

    .byte   pm_hwinfo_size                              ; payload size
0:  .byte   _max_data_size                              ; maximum R/W data size
    .byte   SIGNATURE_2, SIGNATURE_1, SIGNATURE_0, 0    ; MCU 32-bit signature
    .ascii  "flash"                                     ; memory region name
#if CLUNET_BTLDR_EEPROM_SUPPORT
    .byte   0
    .ascii  "eeprom"                                    ; memory region name
#endif
#if CLUNET_BTLDR_DATA_SUPPORT
    .byte   0
    .ascii  "data"                                      ; memory region name
#endif
    .equ    pm_hwinfo_size, . - 0b

CLUNET_ASM_END(pm_hwinfo)

CLUNET_ASM_START(pm_answer_table)

; EN: ANSWER TABLE FOR MEMORY RANGE REQUESTS
; RU: ТАБЛИЦА ОТВЕТОВ НА ЗАПРОСЫ ДИАПАЗОНОВ ПАМЯТИ

.equ pm_not_support, pm_flash_f + 1
#define answer_offset(sym)  (sym - . - 1)
    
    ; Section 1: FLASH
#if CLUNET_CONFIG_BOOTLOADER_FLASH_READ_SUPPORT
    .byte   answer_offset(pm_flash_r)       ; read
#else
    .byte   answer_offset(pm_not_support)   ; read
#endif
    .byte   answer_offset(pm_flash_w)       ; erase page
#if CLUNET_CONFIG_BOOTLOADER_FLASH_EXEC_SUPPORT
    .byte   answer_offset(pm_flash_x)       ; execute
#else
    .byte   answer_offset(pm_not_support)   ; execute
#endif
    .byte   answer_offset(pm_flash_f)       ; fill page buffer
    
    ; Section 2: EEPROM
#if CLUNET_BTLDR_EEPROM_SUPPORT
 #if CLUNET_CONFIG_BOOTLOADER_EEPROM_READ_SUPPORT
    .byte   answer_offset(pm_eeprom_rw)     ; read
 #else
    .byte   answer_offset(pm_not_support)   ; read
 #endif
 #if CLUNET_CONFIG_BOOTLOADER_EEPROM_WRITE_SUPPORT
    .byte   answer_offset(pm_eeprom_rw)     ; write
 #elif CLUNET_BTLDR_DATA_SUPPORT
    .byte   answer_offset(pm_not_support)   ; write
 #endif
 #if CLUNET_BTLDR_DATA_SUPPORT
    .byte   answer_offset(pm_not_support)   ; execute
    .byte   answer_offset(pm_not_support)   ; fill page buffer
 #endif
#endif

    ; Section 3: DATA
#if CLUNET_BTLDR_DATA_SUPPORT
 #if CLUNET_CONFIG_BOOTLOADER_DATA_READ_SUPPORT
    .byte   answer_offset(pm_data_rw)       ; read
 #else
    .byte   answer_offset(pm_not_support)   ; read
 #endif
 #if CLUNET_CONFIG_BOOTLOADER_DATA_WRITE_SUPPORT
    .byte   answer_offset(pm_data_rw)       ; write
 #else
    .byte   answer_offset(pm_not_support)   ; write
 #endif
#endif

CLUNET_ASM_END(pm_answer_table)

; EN: ANSWER TEMPLATES FOR MEMORY RANGE REQUESTS
; RU: ШАБЛОНЫ ОТВЕТОВ НА ЗАПРОСЫ ДИАПАЗОНОВ ПАМЯТИ

.macro pm_range start:req end:req
 .if \end < 0x100
    .byte   2
    .byte   lo8(\start)
    .byte   lo8(\end)
 .elseif \end < 0x10000
    .byte   4
    .byte   lo8(\start), hi8(\start)
    .byte   lo8(\end), hi8(\end)
 .elseif \end < 0x1000000
    .byte   6
    .byte   lo8(\start), hi8(\start), hlo8(\start)
    .byte   lo8(\end), hi8(\end), hlo8(\end)
 .else
    .byte   8
    .byte   lo8(\start), hi8(\start), hlo8(\start), hhi8(\start)
    .byte   lo8(\end), hi8(\end), hlo8(\end), hhi8(\end)
 .endif
.endm

#if CLUNET_CONFIG_BOOTLOADER_FLASH_READ_SUPPORT
CLUNET_ASM_START(pm_flash_r)
    pm_range _flash_start, _flash_end
CLUNET_ASM_END(pm_flash_r)
#endif

CLUNET_ASM_START(pm_flash_w)
    pm_range _flash_write_start, _flash_write_end
CLUNET_ASM_END(pm_flash_w)

#if CLUNET_CONFIG_BOOTLOADER_FLASH_EXEC_SUPPORT
CLUNET_ASM_START(pm_flash_x)
    pm_range _flash_start >> 1, _flash_end >> 1
CLUNET_ASM_END(pm_flash_x)
#endif

CLUNET_ASM_START(pm_flash_f)
    pm_range 0, _pcword_mask
CLUNET_ASM_END(pm_flash_f)

#if CLUNET_BTLDR_EEPROM_SUPPORT
CLUNET_ASM_START(pm_eeprom_rw)
    pm_range _eeprom_start, _eeprom_end
CLUNET_ASM_END(pm_eeprom_rw)
#endif

#if CLUNET_BTLDR_DATA_SUPPORT
CLUNET_ASM_START(pm_data_rw)
    pm_range _data_start, _data_end
CLUNET_ASM_END(pm_data_rw)
#endif
