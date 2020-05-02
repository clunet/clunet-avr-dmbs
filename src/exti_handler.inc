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


; Delays: input latch: ~22 clocks, timer: ~37 clocks

CLUNET_ASM_EXPORT(CLUNET_EXTI_INT_VECTOR)
CLUNET_ASM_START(CLUNET_EXTI_INT_VECTOR)

    ; if we pull-down the line - exit immediatly.
    skip_bus_write_free                                         ; [2][1]
    reti                                                        ; [0][4][=5] -> return
    ; [+2], +4 bytes
    
    ; [2]
    push   status                                               ; [2]
    sfr_ld status, SREG                                         ; [1]
    push   status                                               ; [2]
    ; [+5], +6 bytes
    
    ; [7]
    lds   status, CLUNET_VAR(var_status)                        ; [2]
    sbrc  status, RCF                                           ; [2][1] -|
    sbr   status, _BV(WIF)                                      ; [0][1] -- if (RCF) WIF = 1
    sbrc  status, WIF                                           ; [2][1]
    rjmp  7f                                                    ; [0][2][=14] -> if (WIF) goto 7f:
    ; [+6], +12 bytes
    
    ; [13] WIF and RCF is cleared. Status not changed.
    bst   status, TAF                                           ; [1][1] TAF store in T bit
    skip_bus_read_free                                          ; [2][1][=15] input signal latch (minimal delay: 22 clocks)
    rjmp  exti_bus_is_busy                                      ; [0][2][=17] -> bus is busy
    ; [+3], +6 bytes
    
    ; ***** BUS IS FREE *****
    
    ; [16]
    push  tmp                                                   ; [2]
    push  bit_mask                                              ; [2]
    push  byte_value                                            ; [2]
    lds   bit_mask, CLUNET_VAR(var_bit_mask)                    ; [2]
    lds   byte_value, CLUNET_VAR(var_byte_value)                ; [2]
    ; [+10], +14 bytes

    ; [26]
    brts 3f                                                     ; [1][2] -> TX mode. Skip reading bit cost...
    ; [+1], +2 (44) bytes

    ; [27]
    ; ** READING BIT COST BLOCK **
    sbrc   status, BFF                                          ; [2][1]
    rjmp   0f                                                   ; [0][2] -> if (BFF) bit = 1
    sfr_ld tmp, CLUNET_TIM_CNT_REG                              ; [1|2][=31]
    cpi    tmp, CLUNET_TIM_CAL_VALUE((CLUNET_CONFIG_INTERFRAME_PERIODS) - 2, 37) ; [1]
    brsh 1f                                                     ; [1][2] -> bit is '1'.
    com    bit_mask                                             ; [1]
    and    byte_value, bit_mask                                 ; [1]
    com    bit_mask                                             ; [1]
    rjmp   3f                                                   ; [2] -> bitmask operation.
0:  cbr    status, _BV(BFF)                                     ; [1]
1:  or     byte_value, bit_mask                                 ; [1]
    ; +22 (66) bytes

    ; Transmit mode:  => 26 + 2  = [28] clocks
    ; Receiving mode: => 26 + 8  = [34] bit is TRUE,
    ;                    26 + 11 = [37] bit is FALSE
    ; [28..37]
    ; ** BITMASK OP **
3:  lsr  bit_mask                                               ; [1]
    brne 6f                                                     ; [1][2][+3] -> receiving byte not complete: update bit_mask, byte_value, status. return.
    ; [+2], +4 (74) bytes

    ; ***** ONE MORE BYTE IS RECEIVED *****

    ; [30..43]
    ; ** STORE SOME USED REGISTERS TO STACK **
    push byte_index                                             ; [2]
    push ZL                                                     ; [2]
    push ZH                                                     ; [2] stack size: 2|3 call + 8 vars = 10|11 bytes
    ; [+6], +6 (80) bytes

    ; [36..49]
    ; ** STORE RX BYTE TO BUFFER **
    ldi  ZL, lo8(CLUNET_VAR(var_rx_priority))                   ; [1]-|
    ldi  ZH, hi8(CLUNET_VAR(var_rx_priority))                   ; [1]-- Z = &rx_buffer
    lds  byte_index, CLUNET_VAR(var_byte_index)                 ; [2]
    add  ZL, byte_index                                         ; [1]-|
    adc  ZH, bit_mask                                           ; [1]-- Z += byte_index. bit_mask is zero here.
    st   Z+, byte_value                                         ; [2]-- *Z++ = byte_value
    ; [+8], +14 (94) bytes

    ; [44..57]
    ; ** SWITCH RECEIVING MESSAGE STAGE **
    cpi  byte_index, 4                                          ; [1]
    brlo 3f                                                     ; [1][2][+3] CASE: pre-header
    lds  byte_value, CLUNET_VAR(var_rx_size)                    ; [2] use 'byte_value' register as temporary for 'payload_size' value
    brne 2f                                                     ; [1][2][+4] CASE: post-header (extended data message)
    brts 1f                                                     ; [1][2] -> if TX is active - skip some checks
    ; [+2], +4 (98) bytes

    ; ***** HEADER AND MINIMAL POSSIBLE MESSAGE IS RECEIVED *****

    ; ** CHECK HEADER FIELDS **
    lds     tmp, CLUNET_VAR(var_rx_src)                         ; [2]
    cpi     tmp, CLUNET_ADDRESS_BROADCAST                       ; [1]
    breq    5f                                                  ; [1][2] -> PROTOCOL ERROR: SRC address coudn't be BROADCAST
    lds     tmp, CLUNET_VAR(var_rx_dst)                         ; [2]
    cpi     tmp, CLUNET_ADDRESS_BROADCAST                       ; [1]
    breq    0f                                                  ; [1][2] -> OK: BROADCAST message for us
    cpi     tmp, CLUNET_DEVICE_ADDRESS                          ; [1]
#if CLUNET_CONFIG_API_SNIFFER_CALLBACK
    brne    1f                                                  ; [1][2] -> OK in SNIFFER MODE
0:  sbr     status, _BV(UCF)                                    ; [1] use UniCast Flag in SNIFFER MODE
#else
    brne    5f                                                  ; [1][2] -> ERROR: message not for us
#endif
0:  ; Checking length of the message
1:  cpi     byte_value, CLUNET_CONFIG_RX_PAYLOAD_SIZE + 1       ; [1]
    brsh    5f                                                  ; [1][2] -> ERROR: too big message
    
    ; [48..61] Is message received?
2:  subi    byte_value, -5                                      ; [1]
    cp      byte_index, byte_value                              ; [1]
    brsh    4f                                                  ; [1][2][56..69] -> OK: extended message is received
    ; +10 (116) bytes

    ; [47..68]
3:  inc     byte_index                                          ; [1]
    sts     CLUNET_VAR(var_byte_index), byte_index              ; [2]
    ; +6 (128) bytes

    ; [49..72] TX mode. Load next byte from TX buffer.
.if CLUNET_CONFIG_RX_PAYLOAD_SIZE > 57
 .if CLUNET_CONFIG_RX_PAYLOAD_SIZE == 250
    inc     ZH                                                  ; [1]
 .else
    subi    ZL, lo8(-(CLUNET_CONFIG_RX_PAYLOAD_SIZE + 6))       ; [1]
    sbci    ZH, hi8(-(CLUNET_CONFIG_RX_PAYLOAD_SIZE + 6))       ; [1]
 .endif
    ld      byte_value, Z                                       ; [2]
    ; [+4], +6 (132) bytes
.else
    ldd     byte_value, Z+(CLUNET_CONFIG_RX_PAYLOAD_SIZE + 6)   ; [2]
    ; [+2], +2 (130) bytes
.endif

    ; [51..76]
    ldi     bit_mask, 0b10000000                                ; [1]
    ; +2 (136) bytes

    ; [52..81]
0:  pop     ZH                                                  ; [2]
    pop     ZL                                                  ; [2]
    pop     byte_index                                          ; [2]
6:  sts     CLUNET_VAR(var_byte_value), byte_value              ; [2]
    sts     CLUNET_VAR(var_bit_mask), bit_mask                  ; [2]
    pop     byte_value                                          ; [2]
    pop     bit_mask                                            ; [2]
    pop     tmp                                                 ; [2]
7:  sts     CLUNET_VAR(var_status), status                      ; [2][37..40]
    andi    status, _BV(WIF) | _BV(RAF) | _BV(RCF)              ; [1]
    breq    9f                                                  ; [1][2][+15]
    tim_arm CLUNET_CONFIG_INTERFRAME_PERIODS, 61, status        ; [2]
    tim_rst_flg status                                          ; [2]
    tim_eint status                                             ; [3]
9:  pop     status                                              ; [2]
    sfr_st  SREG, status                                        ; [1]
    pop     status                                              ; [2]
    reti                                                        ; [4]
    ; +10 (164) bytes

    ; TX/RX COMPLETE [49..67]
4:  andi  status, _BV(TWF) | _BV(UCF)                           ; [1]
    sbr   status, _BV(WIF) | _BV(RCF)                           ; [1]
    brtc  0b                                                    ; [1][2][51..69] -> RX mode only
#if CLUNET_CONFIG_API_SNIFFER_CALLBACK && CLUNET_CONFIG_SNIFF_OWN_MESSAGES
    st    -Z, bit_mask                                          ; [2] reset size (checksum) field in simple messages
    ldi   status, _BV(WIF) | _BV(RCF) | _BV(CSF)                ; [1]
#else
    ldi   status, _BV(WIF)                                      ; [1]
#endif
    rjmp  0b                                                    ; [2] ->

    ; ERROR MESSAGE
5:  andi  status, _BV(TWF)                                      ; [1]
    sbr   status, _BV(WIF)                                      ; [1]
    rjmp  0b                                                    ; [2] -> save context and return.

    ; ***** BUS IS BUSY *****

exti_bus_is_busy:
    brtc  1f                                                    ; [1][2] -> RX only mode
    sbrs  status, TWF                                           ; [2][1]
    rjmp  0f                                                    ; [0][2] -> collision. stop transmit. receive only.
    sfr_ldi CLUNET_TIM_CNT_REG, 255, status                     ; [2] start transmit as fast as possible.
    rjmp  9b                                                    ; [2] -> return
0:  ldi   status, _BV(TWF)                                      ; [1] wait retransmit
1:  sbr   status, _BV(RAF)                                      ; [1] receive mode
    rjmp  7b                                                    ; [2] ->

CLUNET_ASM_END(CLUNET_EXTI_INT_VECTOR)
