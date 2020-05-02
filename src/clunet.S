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
   POSSIBILITY OF SUCH DAMAGE. */

#include <avr/io.h>
#include <avr/interrupt.h>

#include "clunet.h"
#include "clunet_internal.h"

#define DEFAULT_TMP_REGISTER 19

#include "sfr_macro.inc"
#include "clunet.inc"
#include "wdt.inc"

; ***** STATUS FLAGS DEFINITIONS *****
; RAF - "Receive Active"
#define RAF 0
; TWF - "Transmit Wait"
#define TWF 1
; TAF - "Transmit Active"
#define TAF 2
; WIF - "Wait Interframe"
#define WIF 3
; BFF - "Bit Force"
#define BFF 4
; UCF - "UniCast"
#define UCF 5
; CSF - "CheckSum"
#define CSF 6
; RCF - "Receive Callback"
#define RCF 7

; Offsets of main data context

#define CLUNET_VAR(var)         ((clunet_context) + (var))
#define CLUNET_CONTEXT_SIZE     (16 + (CLUNET_CONFIG_RX_PAYLOAD_SIZE) + (CLUNET_CONFIG_TX_PAYLOAD_SIZE))

; ***** CLUNET DRIVER CONTEXT OFFSETS DEFINITIONS *****
; status vars offsets
var_status      = 0
var_bit_mask    = 1
var_byte_value  = 2
var_byte_index  = 3
; RX message buffer offsets
var_rx_priority = 4
var_rx_dst      = 5
var_rx_src      = 6
var_rx_command  = 7
var_rx_size     = 8
var_rx_payload  = 9
; TX message buffer offsets
var_tx_priority = 10 + CLUNET_CONFIG_RX_PAYLOAD_SIZE
var_tx_dst      = var_tx_priority + 1
var_tx_src      = var_tx_priority + 2
var_tx_command  = var_tx_priority + 3
var_tx_size     = var_tx_priority + 4
var_tx_payload  = var_tx_priority + 5

; Registers definition
__zero_reg__    = 1
status          = 18
tmp             = DEFAULT_TMP_REGISTER
bit_mask        = 20
bit_counter     = 20
command         = 20
byte_value      = 21
byte_index      = 22
byte_counter    = 22
crc             = 23

.data
.lcomm clunet_context, CLUNET_CONTEXT_SIZE

#if 0
struct {
    uint8_t status ;            // status byte
    uint8_t bit_mask ;          // bit mask
    uint8_t byte_value ;        // current byte
    uint8_t byte_index ;        // byte reverse counter
    clunet_message_t rx ;
    uint8_t _rx_payload_reserved[CLUNET_CONFIG_RX_PAYLOAD_SIZE] ;
    uint8_t _rx_crc_reserved ;
    clunet_message_t tx ;
    uint8_t _tx_payload_reserved[CLUNET_CONFIG_TX_PAYLOAD_SIZE] ;
    uint8_t _tx_crc_reserved ;
} clunet_context __attribute__ ((section (".noinit"))) ;
#endif

.section .progmem,"a",@progbits

; Device description info message
CLUNET_ASM_START(pm_ddesc)
    .byte   ddesc_msize
0:  .byte   CLUNET_PRIORITY_INFO
    .byte   CLUNET_ADDRESS_BROADCAST
    .byte   CLUNET_CONFIG_DEVICE_ADDRESS
    .byte   CLUNET_SYSTEM_EVENT(CLUNET_SYSTEM_INFO)
CLUNET_ASM_START(pm_ddesc_payload)
    .byte   ddesc_psize
1:  .byte   CLUNET_CONFIG_RX_PAYLOAD_SIZE
    .byte   CLUNET_CONFIG_TX_PAYLOAD_SIZE
    .ascii  CLUNET_CONFIG_DEVICE_NAME
.equ ddesc_psize, . - 1b
.equ ddesc_msize, . - 0b
CLUNET_ASM_END(pm_ddesc_payload)
CLUNET_ASM_END(pm_ddesc)

#if CLUNET_BOOTLOADER_CONFIG
.section .bootloader,"ax"
btldr_main:
 #ifdef CLUNET_RWW_WORKAROUND
  ; export global symbol '__init' instead weak defined at avr-libc startup file.
  ; now MCU reset vector unconditional jump to bootloader.
  ; this my easy workaround if MCU unsupported bootloader feature.
  .global __init
  __init:
  ; create new reset vector at last word in app section
  ; so rjmp instruction is enouth for about 1K jump
  .section .clunetreset,"ax"
  clunet_reset:
    rjmp    __ctors_start
 #endif
 .macro start_btldr
    cli                         ; [1]
  .if FLASHEND < 0x2000
    rjmp    btldr_main          ; [2] =>
  .else
    jmp     btldr_main          ; [2] =>
  .endif
 .endm
#endif

.text

CLUNET_ASM_EXPORT(clunet_task)
CLUNET_ASM_START(clunet_task)

    lds     status, CLUNET_VAR(var_status)          ; [2]
    sbrs    status, RAF                             ; [1][1][2]
    sbrc    status, TAF                             ; [2][1][0]
    ret                                             ; [0][4][4][=11] => OK: RX or TX is active. return.
    
    cli                                             ; [1]
    lds     status, CLUNET_VAR(var_status)          ; [2]
    sbrs    status, RCF                             ; [2][1]
    rjmp    7f                                      ; [0][2][=11] -> OK: no message in RX buffer.
    sei                                             ; [1]

    ; RCF && !RAF && !TAF
    ; ***** MESSAGE IN RX BUFFER *****

    sbrc    status, CSF                             ; [2][1]
    rjmp    3f                                      ; [0][2][=9] -> skip checksum validating of RX message

    ; checksum validate
    xldi    CLUNET_VAR(var_rx_dst)                  ; [2]
    rcall   message_checksum                        ; [53*size + 15]
    ld      byte_value, X                           ; [2]
    sub     byte_value, crc                         ; [1]
    brne    6f                                      ; [1][2] -> BAD: CHECKSUM IS WRONG
    st      X, byte_value                           ; [2] reset checksum field
    
    ; set checksum flag
    cli                                             ; [1]
    lds     status, CLUNET_VAR(var_status)          ; [2]
    sbr     status, _BV(CSF)                        ; [1]
    sei                                             ; [1]
    sts     CLUNET_VAR(var_status), status          ; [2]

    ; ***** CHECKSUM IS OK *****

#if CLUNET_CONFIG_API_SNIFFER_CALLBACK
3:  arg0ldi16 CLUNET_VAR(var_rx_priority)           ; [2]
    rcall   clunet_sniffer_cb                       ; [3]
    cpi     ret0lo, 1                               ; [1]
    brlo    9f                                      ; [1][2] -> callback return   0: just return
    brne    clunet_tx_start                         ; [1][2] -> callback return > 1: send response
    ; callback return 1: continue
    lds     status, CLUNET_VAR(var_status)          ; [2] Load status from RAM (register may be corrupted in clunet_sniffer_cb)
    sbrs    status, UCF                             ; [2][1]
    rjmp    6f                                      ; [0][2] -> RESET & CHECK TWF: message not for us.
#endif

3:  lds     command, CLUNET_VAR(var_rx_command)     ; [2]
    sbrs    command, CLUNET_MESSAGE_RESPONSE_BIT    ; [2][1][1]
    sbrs    command, CLUNET_MESSAGE_SYSTEM_BIT      ; [0][2][1]
    rjmp    user_cb                                 ; [2][0][2][+6] -> all responses (or events) and non-system requests goes to user's callback function
    ; [+5]
    
    ; ***** SYSTEM REQUEST MESSAGES ONLY *****
    
    cbr     command, CLUNET_MESSAGE_TYPE_MASK       ; [1] clear all type bits
    
    ; Here only command code

    ; first system command (zero code). rebooting always.
    brne    0f                                      ; [1][2] -> next command
#if !CLUNET_BOOTLOADER_CONFIG || CLUNET_CONFIG_BOOTLOADER_WDT_DISABLE
    reboot_by_wdt                                   ; [x] => hard reset MCU by WDT
#else
    start_btldr                                     ; [x] => bootloader without WDT reset, so simple jump to it.
#endif
0:  sbrs    status, TWF                             ; [2][1]
    rjmp    0f                                      ; [0][2] -> no message in TX buffer
    
    ; tx buffer is busy. If same or equal - cancel transmit and response on new request.
    lds     tmp, CLUNET_VAR(var_rx_priority)        ; [2]
    lds     crc, CLUNET_VAR(var_tx_priority)        ; [2]
    cp      tmp, crc                                ; [1]
    brlo    6f                                      ; [1][2] -> TWF is SET. ZF is not set. Status register is updated. Start transmit.

    ; last system command
0:  cpi     command, CLUNET_SYSTEM_PING             ; [1]
    breq    0f                                      ; [1][2] -> CLUNET_SYSTEM_PING (2)
    brsh    6f                                      ; [1][2] -> RESET & CHECK TWF: ignore all above last command

    ; CLUNET_SYSTEM_INFO(1)
    rcall   clunet_tx_prepare_simple_response       ; [3]
    arg0ldi16 pm_ddesc_payload                      ; [2]
    rcall   clunet_tx_set_payload_template_P        ; [3]
    rjmp    clunet_tx_start                         ; [2]

    ; CLUNET_SYSTEM_PING(2)
0:  rcall   clunet_tx_prepare_simple_response       ; [3]
    arg0ldi16 CLUNET_VAR(var_rx_payload)            ; [2]
    lds     arg1lo, CLUNET_VAR(var_rx_size)         ; [2]
    rcall   clunet_tx_set_payload                   ; [3]
    rjmp    clunet_tx_start                         ; [2]

user_cb:
    arg0ldi16 CLUNET_VAR(var_rx_priority)           ; [2]
    rcall   clunet_receive_cb                       ; [3+]
    cpi     ret0lo, 1                               ; [1]
    brlo    9f                                      ; [1][2] -> callback return   0: repeat again
    brne    clunet_tx_start                         ; [1][2] -> callback return > 1: send response
    ; callback return 1: normal
6:  cli                                             ; [1]
    lds     status, CLUNET_VAR(var_status)          ; [2] Load status from RAM (register may be corrupted in clunet_sniffer_cb)
7:  andi    status, _BV(WIF) | _BV(TWF)             ; [1]
    sts     CLUNET_VAR(var_status), status          ; [2]
    brne    5f                                      ; [1][2] -> start transmit
    sei                                             ; [1]
9:  ret                                             ; [4]

CLUNET_ASM_EXPORT(clunet_tx_start)
CLUNET_ASM_START(clunet_tx_start)

    ldi     tmp, CLUNET_CONFIG_DEVICE_ADDRESS       ; [1]
    sts     CLUNET_VAR(var_tx_src), tmp             ; [2]
    xldi    CLUNET_VAR(var_tx_dst)                  ; [2]
    rcall   message_checksum                        ; [3]
    st      X, crc                                  ; [2]

CLUNET_ASM_START(clunet_tx_restart)

    cli                                             ; [1]
    lds     status, CLUNET_VAR(var_status)          ; [2]
    andi    status, _BV(WIF)                        ; [1]
    sbr     status, _BV(TWF)                        ; [1]
    sts     CLUNET_VAR(var_status), status          ; [2]
    ; start transmit
5:  sfr_ld  tmp, CLUNET_TIM_INT_REG                 ; [1]
    sbr     tmp, _BV(CLUNET_TIM_INT_OVF_BIT)        ; [1]
    skip_bus_read_busy                              ; [2][1]
    sfr_st  CLUNET_TIM_INT_REG, tmp                 ; [0][1]
    sei                                             ; [1]
    ret                                             ; [4]

CLUNET_ASM_END(clunet_tx_restart)
CLUNET_ASM_END(clunet_tx_start)
CLUNET_ASM_END(clunet_task)

; message_checksum - local function for calculate CRC-8 MAXIM
; IN:  X - address of data, byte_counter - size of data
; OUT: crc = CRC-8,
; X - points to next byte after data,
; byte_counter, bit_counter = 0, tmp = 0x8C
; 53 * size + 11(12) clocks

CLUNET_ASM_START(message_checksum)

    movw  ZL, XL                        ; [2]
    ldd   byte_counter, Z+3             ; [2] size
    subi  byte_counter, -4              ; [1]
    clr   crc                           ; [1]
#if CLUNET_BOOTLOADER_SUPPORT && CLUNET_CONFIG_BOOTLOADER_USE_CRC_TABLE
    zhldi_P FLASHEND                    ; [2]
#else
    ldi   tmp, 0b10001100               ; [1]
#endif
0:  ld    ZL, X+                        ; [2] <- next byte
#if CLUNET_BOOTLOADER_SUPPORT && CLUNET_CONFIG_BOOTLOADER_USE_CRC_TABLE
    eor   ZL, crc                       ; [1]
 #if CLUNET_RAMPZ
    elpm  crc, Z                        ; [3]
 #else
    lpm   crc, Z                        ; [3]
 #endif
#else
    eor   crc, ZL                       ; [1]
    ldi   bit_counter, 8                ; [1]
1:  lsr   crc                           ; [1] <- next bit
    brcc  2f                            ; [1][2]
    eor   crc, tmp                      ; [1][0]
2:  dec   bit_counter                   ; [1]
    brne  1b                            ; [1][2] -> next bit
#endif
    dec   byte_counter                  ; [1]
    brne  0b                            ; [1][2] -> next byte
    ret                                 ; [4]

CLUNET_ASM_END(message_checksum)

CLUNET_ASM_EXPORT(clunet_init)
CLUNET_ASM_START(clunet_init)

    ; ALMOST ALL REGISTERS INITIALLY RESET TO ZERO
    ldi     tmp, _BV(WIF)                           ; [1]
    sts     CLUNET_VAR(var_status), tmp             ; [2]
    gpio_init                                       ; [x]
    exti_init                                       ; [x]
    tim_arm CLUNET_CONFIG_INTERFRAME_PERIODS, 0     ; [x]
    tim_rst_flg                                     ; [x]
    tim_start                                       ; [x]

CLUNET_ASM_EXPORT(clunet_tx_send_discovery)
CLUNET_ASM_START(clunet_tx_send_discovery)

    arg0ldi16 pm_ddesc                              ; [2]
    rcall   clunet_tx_set_message_template_P        ; [3+]
    rjmp    clunet_tx_start                         ; [2] ->

CLUNET_ASM_END(clunet_tx_send_discovery)
CLUNET_ASM_END(clunet_init)

CLUNET_ASM_EXPORT(clunet_tx_is_busy)
CLUNET_ASM_START(clunet_tx_is_busy)

    lds   ret0lo, CLUNET_VAR(var_status)    ; [2]
    andi  ret0lo, _BV(TAF) | _BV(TWF)       ; [1]
    ret                                     ; [4]

CLUNET_ASM_END(clunet_tx_is_busy)


CLUNET_ASM_EXPORT(clunet_tx_wait)
CLUNET_ASM_START(clunet_tx_wait)

0:  lds     ret0lo, CLUNET_VAR(var_status)  ; [2]
    andi    ret0lo, _BV(TAF) | _BV(TWF)     ; [1]
    breq    1f                              ; [1][2]
    rcall   clunet_task                     ; [3++]
    rjmp    0b                              ; [2]
1:  ret                                     ; [4]

CLUNET_ASM_END(clunet_tx_wait)

; Get pointer to TX message
CLUNET_ASM_EXPORT(clunet_tx_get_message)
CLUNET_ASM_START(clunet_tx_get_message)

    arg0ldi16 CLUNET_VAR(var_tx_priority)   ; [2]
    ret                                     ; [4]

CLUNET_ASM_END(clunet_tx_get_message)

; Preapare simple response from message in RX buffer
; After calling this function, you may add payload if needed and finally
; call 'clunet_tx_start' for send message
CLUNET_ASM_EXPORT(clunet_tx_prepare_simple_response)
CLUNET_ASM_START(clunet_tx_prepare_simple_response)

    xldi    CLUNET_VAR(var_tx_priority)     ; [2]
    zldi    CLUNET_VAR(var_rx_priority)     ; [2]
    ldd     tmp, Z+0                        ; [2]
    st      X+, tmp                         ; [2] prio
    ldd     tmp, Z+2                        ; [2]
    st      X+, tmp                         ; [2] dst
    adiw    XL, 1                           ; [2]
    ldd     tmp, Z+3                        ; [2]
    sbr     tmp, CLUNET_MESSAGE_RESPONSE    ; [1]
    st      X+, tmp                         ; [2] cmd
    st      X+, __zero_reg__                ; [2] size
    ret                                     ; [4]

CLUNET_ASM_END(clunet_tx_prepare_simple_response)

CLUNET_ASM_EXPORT(clunet_tx_set_message_template_P)
CLUNET_ASM_START(clunet_tx_set_message_template_P)

    movw    ZL, arg0lo                  ; [2]
    lpm     arg1lo, Z+                  ; [3]
    xldi    CLUNET_VAR(var_tx_priority) ; [2]
    rjmp    2f                          ; [2] ->

; Copy payload data from PROGMEM to TX buffer.
; First byte in PROGMEM must be size of payload.
CLUNET_ASM_EXPORT(clunet_tx_set_payload_P)
CLUNET_ASM_START(clunet_tx_set_payload_P)

    movw    ZL, arg0lo                  ; [2]
    rjmp    1f                          ; [2] ->

; Copy payload data from PROGMEM to TX buffer.
; First byte in PROGMEM must be size of payload.
CLUNET_ASM_EXPORT(clunet_tx_set_payload_template_P)
CLUNET_ASM_START(clunet_tx_set_payload_template_P)

    movw    ZL, arg0lo                  ; [2]
    lpm     arg1lo, Z+                  ; [3]
1:  xldi    CLUNET_VAR(var_tx_size)     ; [2]
    st      X+, arg1lo                  ; [2] size
2:  lpm     tmp, Z+                     ; [3] <-
    st      X+, tmp                     ; [2]
    dec     arg1lo                      ; [1]
    brne    2b                          ; [1][2] ->
    ret                                 ; [4]

CLUNET_ASM_END(clunet_tx_set_payload_template_P)
CLUNET_ASM_END(clunet_tx_set_payload_P)
CLUNET_ASM_END(clunet_tx_set_message_template_P)

; Copy payload data to TX message buffer
; void clunet_tx_set_payload(uint8_t *payload, uint8_t size);
; WARNING: payload size must not exceed the size of the TX buffer!
CLUNET_ASM_EXPORT(clunet_tx_set_payload)
CLUNET_ASM_START(clunet_tx_set_payload)

    movw    ZL, arg0lo                  ; [2]
    xldi    CLUNET_VAR(var_tx_size)     ; [2]
    st      X+, arg1lo                  ; [2] size
    rjmp    0f                          ; [2] ->

CLUNET_ASM_EXPORT(clunet_tx_set_message)
CLUNET_ASM_START(clunet_tx_set_message)

    movw    ZL, arg0lo                  ; [2]
    ldd     arg1lo, Z+4                 ; [2]
    subi    arg1lo, -5                  ; [1]
    xldi    CLUNET_VAR(var_tx_priority) ; [2]
0:  ld      tmp, Z+                     ; [2] <-
    st      X+, tmp                     ; [2]
    dec     arg1lo                      ; [1]
    brne    0b                          ; [1][2] ->
    ret                                 ; [4]

CLUNET_ASM_END(clunet_tx_set_message)
CLUNET_ASM_END(clunet_tx_set_payload)

#include "timer_handler.inc"
#include "exti_handler.inc"

