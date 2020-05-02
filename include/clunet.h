/* Copyright (c) 2018-2020 Sergey V. DUDANOV
   All rights reserved.

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

#ifndef _CLUNET_H_
#define _CLUNET_H_

// EN: Defining 4 levels of packets priority.
// RU: Определение 4 уровней приоритета сообщений.
#define CLUNET_PRIORITY_NOTICE          0
#define CLUNET_PRIORITY_INFO            1
#define CLUNET_PRIORITY_MESSAGE         2
#define CLUNET_PRIORITY_COMMAND         3

// EN: Broadcast address.
// RU: Широковещательный адрес.
#define CLUNET_ADDRESS_BROADCAST        255

// EN: Return it from callback function if you want return to it again. WARNING: receiving buffer is not freeing.
// RU: Верните из функции обратного вызова, чтобы вернуться в нее позже. ПРЕДУПРЕЖДЕНИЕ: приемный буфер не освобождается.
#define CLUNET_CALLBACK_RESULT_BACK     0

// EN: Return it from callback function for free receive buffer and receiving new messages.
// RU: Верните из функции обратного вызова для освобождения приемного буфера и приема новых сообщений.
#define CLUNET_CALLBACK_RESULT_NORMAL   1

// EN: Return it from callback function if you want free receive buffer and send message in transmit buffer.
// RU: Верните из функции обратного вызова для освобождения приемного буфера и отправки сообщения в буфере передачи.
#define CLUNET_CALLBACK_RESULT_SEND     2

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

// EN: Predefined bits in command/event field.
// RU: Предопределенные биты в поле команды/события.
#define CLUNET_MESSAGE_SYSTEM_BIT           6
#define CLUNET_MESSAGE_RESPONSE_BIT         7
#define CLUNET_MESSAGE_EVENT_BIT            CLUNET_MESSAGE_RESPONSE_BIT
#define CLUNET_MESSAGE_RESPONSE             _BV(CLUNET_MESSAGE_RESPONSE_BIT)
#define CLUNET_MESSAGE_EVENT                _BV(CLUNET_MESSAGE_EVENT_BIT)
#define CLUNET_MESSAGE_SYSTEM               _BV(CLUNET_MESSAGE_SYSTEM_BIT)
#define CLUNET_MESSAGE_TYPE_MASK            (CLUNET_MESSAGE_RESPONSE | CLUNET_MESSAGE_SYSTEM)

// EN: Request.
// RU: Запрос.
#define CLUNET_REQUEST(cmd)                 ((cmd))
// EN: Response.
// RU: Ответ.
#define CLUNET_RESPONSE(cmd)                ((cmd) | CLUNET_MESSAGE_RESPONSE)
// EN: Event (response alias).
// RU: Событие (синоним ответа).
#define CLUNET_EVENT(cmd)                   ((cmd) | CLUNET_MESSAGE_EVENT)
// EN: System request.
// RU: Системный запрос.
#define CLUNET_SYSTEM_REQUEST(cmd)          ((cmd) | CLUNET_MESSAGE_SYSTEM)
// EN: System response.
// RU: Системный ответ.
#define CLUNET_SYSTEM_RESPONSE(cmd)         ((cmd) | CLUNET_MESSAGE_SYSTEM | CLUNET_MESSAGE_RESPONSE)
// EN: System event (alias to system response).
// RU: Системное событие (синоним системного ответа).
#define CLUNET_SYSTEM_EVENT(cmd)            ((cmd) | CLUNET_MESSAGE_SYSTEM | CLUNET_MESSAGE_EVENT)

/* EN: DRIVER SYSTEM INTERNAL COMMANDS */
/* RU: ВНУТРЕННИЕ СИСТЕМНЫЕ КОМАНДЫ ДРАЙВЕРА */

// EN: MCU reset. If starting bootloader answer by broadcast event, waiting for CLUNET_BTLDR_ENTER.
// RU: Сброс МК. В случае запуска загрузчика - отвечает широковещательным событием, ожидая CLUNET_BTLDR_ENTER.
#define CLUNET_SYSTEM_REBOOT            0

// EN: Device info. The response contains information about the device (rx and tx buffers sizes and name of device).
// RU: Информация об устройстве. Данные содержат следующее: размеры буферов приема-передачи и имя устройства.
#define CLUNET_SYSTEM_INFO              1

// EN: Ping.
// RU: Пинг.
#define CLUNET_SYSTEM_PING              2

/* EN: BOOTLOADER PROTOCOL COMMANDS */
/* RU: КОМАНДЫ ПРОТОКОЛА ЗАГРУЗЧИКА */

// EN: Exit from bootloader and start execute default code.
// RU: Выход из загрузчика и запуск выполнения кода по-умолчанию.
#define CLUNET_BTLDR_EXIT               8

// EN: Enter to bootloader mode. Answer with basic HW description data.
// RU: Вход в режим загрузчика. Отвечает основной информацией об оборудовании.
#define CLUNET_BTLDR_ENTER              9

/* EN: COMMANDS 10-15 SPECIFIC FOR VARIOUS PLATFORMS */
/* RU: КОМАНДЫ 10-15 СПЕЦИФИЧНЫЕ ДЛЯ РАЗЛИЧНЫХ ПЛАТФОРМ */

// EN: Read fuses and bitlock for AVR platform. RESPONSE: LF,BL,EF,HF
// RU: Чтение fuse и bitlock для платформы AVR. ОТВЕТ: LF,BL,EF,HF
#define CLUNET_BTLDR_READ_FUSES         10

// EN: Write bitlock for AVR platform. REQUEST: BL, RESPONSE: BL
// RU: Запись bitlock для платформы AVR. ЗАПРОС: BL, ОТВЕТ: BL
#define CLUNET_BTLDR_BITLOCK_WRITE      11

/* EN: BOOTLOADER MEMORY COMMANDS (16..63, 12 sections x 4 commands) */
/* RU: КОМАНДЫ ЗАГРУЗЧИКА ДЛЯ РАБОТЫ С ПАМЯТЬЮ (16..63, 12 секций по 4 команды) */

// EN: Bootloader command for reading memory with index 'm'
//     REQUEST:  uint32_t address, uint8_t size
//     RESPONSE: uint32_t address, readed data...
// RU: Команда загрузчика для чтения памяти с индексом 'm'
//     ЗАПРОС:   uint32_t адрес, uint8_t размер
//     ОТВЕТ:    uint32_t адрес, прочитанные данные...
#define CLUNET_BTLDR_MEMORY_READ(m)     (16 + (m) * 4)

// EN: Bootloader command for writing memory with index 'm'
//     REQUEST:  uint32_t address (for paged memory - page address), [ data to write... (for non-paged memory) ]
//     RESPONSE: uint32_t address
// RU: Команда загрузчика для чтения памяти с индексом 'm'
//     ЗАПРОС:   uint32_t адрес (для страничной памяти - адрес страницы), [ данные для записи... (для нестраничной памяти) ]
//     ОТВЕТ:    uint32_t адрес
#define CLUNET_BTLDR_MEMORY_WRITE(m)    (17 + (m) * 4)

// EN: Bootloader command for execute code from memory with index 'm'
//     REQUEST:  uint32_t address
//     RESPONSE: without response if success, else error response
// RU: Команда загрузчика для запуска кода из памяти с индексом 'm'
//     ЗАПРОС:   uint32_t адрес
//     ОТВЕТ:    без ответа в случае успеха, иначе сообщение об ошибке
#define CLUNET_BTLDR_MEMORY_EXECUTE(m)  (18 + (m) * 4)

// EN: Bootloader command for filling page buffer of memory with index 'm' (only for paged memory)
//     REQUEST:  uint32_t address, data to write...
//     RESPONSE: uint32_t address
// RU: Команда загрузчика для заполнения буфера страницы памяти с индексом 'm' (только для страничной памяти)
//     ЗАПРОС:   uint32_t адрес, данные для записи...
//     ОТВЕТ:    uint32_t адрес
#define CLUNET_BTLDR_MEMORY_FILL(m)     (19 + (m) * 4)

#ifndef __ASSEMBLER__

#include <stdint.h>

typedef uint8_t clunet_priority_t;
typedef uint8_t clunet_address_t;
typedef uint8_t clunet_command_t;
typedef uint8_t clunet_result_t;

typedef struct {
    uint8_t tx_size;
    uint8_t rx_size;
    char    name[];
} clunet_device_description_t;

typedef struct {
    clunet_priority_t priority;     /* message priority */
    clunet_address_t  dst_address;  /* destination address */
    clunet_address_t  src_address;  /* source address */
    clunet_command_t  command;      /* command */
    uint8_t           size;         /* size of payload data */
    /* payload data */
} clunet_message_t;

typedef struct {
    uint8_t           size;         /* whole message size */
    clunet_message_t  message;      /* message */
    /* payload data */
} clunet_template_t;

#define CLUNET_HEADER_SIZE  sizeof(clunet_message_t)

// EN: Driver initialization. After call needs to globally enable interrupts with sei().
// RU: Инициализация драйвера. После вызова необходимо глобально активировать прерывания с помощью sei().
void clunet_init(void);

// EN: Main driver function. Must be periodically call by user. Callback functions calling from its context.
// RU: Главная функция драйвера. Должна периодически вызываться пользователем. Из ее контекста вызываются функции обратного вызова.
void clunet_task(void);

// EN: DON'T USE IN CALLBACKS! Block until transmit not completed. Transmit not started while receive buffer is full.
//     Don't forget free it in callback.
// RU: НЕ ИСПОЛЬЗУЙТЕ В ФУНКЦИЯХ ОБРАТНОГО ВЫЗОВА! Блокирует управление пока передача не будет завершена.
//     Передача не начнется, пока занят буфер приема. Не забывайте освобождать его в функциях обратного вызова.
void clunet_tx_wait(void);

// EN: Getting state of transmitter. Return: NULL if IDLE, else - BUSY.
// RU: Проверка состояния передатчика. Возврат: 0 - свободен, иначе занят.
uint8_t clunet_tx_is_busy(void);

// EN: Getting pointer to transmit packet buffer for direct access. Unsafe.
// RU: Получение указателя на буфер сообщения передатчика для прямого доступа. Небезопасно.
clunet_message_t *clunet_tx_get_message(void);

// EN: Prepare simple response message in TX buffer. So you need set payload only if needed and start transmit.
// RU: Подготовка простого ответа. После только необходимо добавить данные если это необходимо и можно отправлять.
void clunet_tx_prepare_simple_response(void);

// EN: Copy message in RAM to TX buffer.
// RU: Копирует сообщение в буфер отправки.
void clunet_tx_set_message(clunet_message_t *message);

// EN: Copy payload to TX buffer.
// RU: Копирует данные в буфер отправки.
void clunet_tx_set_payload(void *payload, uint8_t payload_size);

// EN: Copy payload to TX buffer from PROGMEM data.
// RU: Копирует данные в буфер отправки из PROGMEM области.
void clunet_tx_set_payload_P(const void *payload, uint8_t payload_size);

// EN: Copy payload data to TX buffer from PROGMEM. First byte must contain size.
// RU: Копирует данные в буфер отправки из шаблона в PROGMEM области. Первый байт должен содержать размер.
void clunet_tx_set_payload_template_P(const void *payload);

// EN: Copy message to TX buffer from PROGMEM template. First byte must contain size of whole message.
// RU: Копирует сообщение в буфер отправки из шаблона в PROGMEM области. Первый байт должен содержать размер всего сообщения.
void clunet_tx_set_message_template_P(const clunet_template_t *message);

// EN: Start transmit operation. Add checksum and source device address, so don't care about it.
// RU: Запускает передачу. Добавляет контрольную сумму и адрес устройства, не переживайте об этом.
void clunet_tx_start(void);

// EN: Transmit broadcast discovery struct.
// RU: Отправляет широковещательное сообщение об устройстве.
void clunet_tx_send_discovery(void);

// EN: Receive callback function prototype. Must be implemented in user code.
// RU: Прототип функции обратного вызова.  Может быть включен в файле clunet_config.h и должен быть реализован в пользовательском коде.
clunet_result_t clunet_receive_cb(clunet_message_t *message);

// EN: Receive sniffer callback function prototype. May be enabled in clunet_config.h and must implemented in user code.
// RU: Прототип функции обратного вызова сниффера. Может быть включен в файле clunet_config.h и должен быть реализован в пользовательском коде.
clunet_result_t clunet_sniffer_cb(clunet_message_t *message);

#endif // __ASSEMBLER__
#endif // _CLUNET_H_
