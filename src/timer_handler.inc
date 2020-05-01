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

; TIMER OVERFLOW INTERRUPT ROUTINE
; Delays: FREE LINE: ~13 clocks, BUSY LINE: ~30 clocks

CLUNET_ASM_EXPORT(CLUNET_TIM_INT_OVF_VECTOR)
CLUNET_ASM_START(CLUNET_TIM_INT_OVF_VECTOR)

    push    status                                                      ; [2]

    skip_bus_write_busy status                                          ; [2][1]
    rjmp    0f                                                          ; [0][2][=5] ->
    bus_write_free      status                                          ; [2][=6]
    tim_arm 1, 15,      status                                          ; [2] ~15 clocks
    pop     status                                                      ; [2]
    reti                                                                ; [4]

    ; [5]
0:  sfr_ld  status, SREG                                                ; [1]
    push    status                                                      ; [2]
    push    tmp                                                         ; [2]

    ; [10]
    lds     status, CLUNET_VAR(var_status)                              ; [2]
    bst     status, TWF                                                 ; [1]
    mov     tmp, status                                                 ; [1]
    andi    tmp, _BV(WIF) | _BV(RAF) | _BV(RCF)                         ; [1]
    brne    1f                                                          ; [1][2][=17] -> (WIF || RAF || RCF)

    ; ( !WIF && !RAF && !RCF )
    brts    3f                                                          ; [1][2][+8] -> TWF is set. Start transmit.

    ; [18] ( !WIF && !RAF && !RCF && !TWF ) - transmit active. collision detection block.
    skip_bus_read_busy                                                  ; [2][1]
    rjmp    4f                                                          ; [0][2][=21] -> bus is free
    ldi     status, _BV(TWF) | _BV(BFF) | _BV(RAF)                      ; [1]
2:  tim_dint                                                            ; [3]
    rjmp    6f                                                          ; [2] -> exit

    ; [10] ; ( !WIF && !RAF && !RCF && TWF ) Transmit not active but needed. Preparing for start transmit.
3:  ldi     status, _BV(TAF)                                            ; [1]
4:  bus_write_busy                                                      ; [2][=23]
    lds     tmp, CLUNET_VAR(var_byte_value)                             ; [2]
    push    status                                                      ; [2]
    lds     status, CLUNET_VAR(var_bit_mask)                            ; [2]
    and     tmp, status                                                 ; [1]
    pop     status                                                      ; [2]
    ldi     tmp, CLUNET_TIM_CAL_VALUE(1, 43)                            ; [1]
    breq    .+2                                                         ; [1][2]
    ldi     tmp, CLUNET_TIM_CAL_VALUE(3, 43)                            ; [1][0]
5:  sfr_st  CLUNET_TIM_CNT_REG, tmp                                     ; [1]
6:  sts     CLUNET_VAR(var_status), status                              ; [2]
    pop     tmp                                                         ; [2]
    pop     status                                                      ; [2]
    sfr_st  SREG, status                                                ; [1]
    pop     status                                                      ; [2]
    reti                                                                ; [4]

    ; ( WIF || RAF || RCF ): Clear only WIF and RAF, prepare to the new frame.
1:  cbr     status, _BV(WIF) | _BV(RAF)                                 ; [1]
    ldi     tmp, 0b10                                                   ; [1]
    sts     CLUNET_VAR(var_bit_mask), tmp                               ; [2]
    clr     tmp                                                         ; [1]
    sts     CLUNET_VAR(var_byte_index), tmp                             ; [2]
    lds     tmp, CLUNET_VAR(var_tx_priority)                            ; [2]
    sts     CLUNET_VAR(var_byte_value), tmp                             ; [2]
    brtc    2b                                                          ; [1][2] -> !TWF - disable timer interrupts and exit.
    sbrc    status, RCF                                                 ; [2][1]
    rjmp    2b                                                          ; [0][2] ->  RCF - disable timer interrupts and exit.
    ; (TWF && !RCF): Planning start transmit through 5T.
    sbr     status, _BV(TAF)                                            ; [1]
    ldi     tmp, CLUNET_TIM_CAL_VALUE(CLUNET_CONFIG_INTERFRAME_PERIODS, 43)    ; [1]
    rjmp    5b                                                          ; [2] -> set timer and exit

CLUNET_ASM_END(CLUNET_TIM_INT_OVF_VECTOR)
