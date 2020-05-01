
// This is example of clunet_config.h file. Rename it, edit and use in your project.

/* Copyright (c) 2018-2020  Sergey V. DUDANOV
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
   POSSIBILITY OF SUCH DAMAGE.	*/

#ifndef _CLUNET_CONFIG_H_
#define _CLUNET_CONFIG_H_

/********************************
 *  EN: DEVICE CONFIGURATION    *
 *  RU: КОНФИГУРАЦИЯ УСТРОЙСТВА *
 ********************************/

// EN: Device short name for network discovery.
// RU: Короткое имя устройства для сетевого обнаружения.
#define CLUNET_CONFIG_DEVICE_NAME               "Light #1"

// EN: Network address (unique in network, between 1-255).
// RU: Сетевой адрес (уникальный в сети, от 1 до 255).
#define CLUNET_CONFIG_DEVICE_ADDRESS            10

// EN: Maximum TX payload data size in bytes (250 max). Transfer attempt of more data will be truncated them.
// RU: Максимальный размер передаваемых полезных данных в байтах (250 макс). Попытка передачи данных большего размера приведет к их обрезанию.
#define CLUNET_CONFIG_TX_PAYLOAD_SIZE           250

// EN: Maximum RX payload data size in bytes (250 max). Packets with more size of data will be ignored.
// RU: Максимальный размер принимаемых полезных данных в байтах (250 макс). Пакеты с бOльшим размером данных игнорируются.
#define CLUNET_CONFIG_RX_PAYLOAD_SIZE           250

/***************************************
 *  EN: MCU CONFIGURATION              *
 *  RU: КОНФИГУРАЦИЯ МИКРОКОНТРОЛЛЕРА  *
 ***************************************/

// EN: Timer number to use by driver. Needs any 8-bit timer with overflow interrupt.
// RU: Номер таймера для использования в драйвере. Необходим любой 8-битный таймер с прерыванием по переполнению.
#define CLUNET_CONFIG_TIM                       2

// EN: Number of external interrupt (EXTI). Reading pin has been auto-selected.
// RU: Номер внешнего прерывания. Соответствующая нога для чтения будет выбрана автоматически.
#define CLUNET_CONFIG_INT                       0

// EN: Define write pin, else has been used one-pin configuration.
// RU: Определите ногу записи, иначе будет использоваться конфигурация с одной ногой.
#define CLUNET_CONFIG_GPIO_WRITE_PORT           B
#define CLUNET_CONFIG_GPIO_WRITE_PIN            5

/***************************
 *  EN: API CONFIGURATION  *
 *  RU: КОНФИГУРАЦИЯ API   *
 ***************************/

// EN: Enable unsafe API! Only for experts!
// RU: Включить небезопасный API! Только для опытных!
#define CLUNET_CONFIG_API_UNSAFE                1

// EN: Use callback function for user packets. Must be implemented in user code!
//     Prototype in clunet.h: [ void clunet_receive_cb ( clunet_packet_t *packet ); ]
// RU: Использовать пользовательскую функцию обратного вызова.
//     Должен быть реализован в пользовательском коде. Прототип в заголовочном файле clunet.h.
#define CLUNET_CONFIG_API_RECEIVE_CALLBACK      1

// EN: Use sniffer callback function. Execute on every received packet. Must be implemented in user code!
//     Prototype in clunet.h: [ void clunet_sniffer_cb ( clunet_packet_t *packet ); ]
// RU: Использовать функцию обратного вызова сниффера. Выполняется при приеме всех пакетов.
//     Должен быть реализован в пользовательском коде. Прототип в заголовочном файле clunet.h.
#define CLUNET_CONFIG_API_SNIFFER_CALLBACK      0

#define CLUNET_CONFIG_SNIFFER_OWN_MESSAGES      1

/*********************************
 *  EN: BOOTLOADER CONFIGURATION *
 *  RU: КОНФИГУРАЦИЯ ЗАГРУЗЧИКА  *
 *********************************/

// EN: Reading attempt timeout in miliseconds.
// RU: Таймаут попытки чтения сообщения в милисекундах.
#define CLUNET_CONFIG_BOOTLOADER_READ_TIMEOUT           200

// EN: Number of retries to receive BTLDR_ENTER packet.
// RU: Количество попыток чтения пакета входа в режим загрузчика.
#define CLUNET_CONFIG_BOOTLOADER_READ_RETRIES           5

// EN: FLASH read support (1 - ON, 0 - OFF)
// RU: Поддержка чтения FLASH (1 - ВКЛ, 0 - ВЫКЛ)
#define CLUNET_CONFIG_BOOTLOADER_FLASH_READ_SUPPORT     1

// EN: FLASH execute support (1 - ON, 0 - OFF)
// RU: Поддержка запуска кода с FLASH (1 - ВКЛ, 0 - ВЫКЛ)
#define CLUNET_CONFIG_BOOTLOADER_FLASH_EXEC_SUPPORT     1

// EN: EEPROM read support (1 - ON, 0 - OFF)
// RU: Поддержка чтения EEPROM (1 - ВКЛ, 0 - ВЫКЛ)
#define CLUNET_CONFIG_BOOTLOADER_EEPROM_READ_SUPPORT    1

// EN: EEPROM write support (1 - ON, 0 - OFF)
// RU: Поддержка записи EEPROM (1 - ВКЛ, 0 - ВЫКЛ)
#define CLUNET_CONFIG_BOOTLOADER_EEPROM_WRITE_SUPPORT   1

// EN: DATA read support (1 - ON, 0 - OFF)
// RU: Поддержка чтения DATA (1 - ВКЛ, 0 - ВЫКЛ)
#define CLUNET_CONFIG_BOOTLOADER_DATA_READ_SUPPORT      1

// EN: DATA write support (1 - ON, 0 - OFF)
// RU: Поддержка записи DATA (1 - ВКЛ, 0 - ВЫКЛ)
#define CLUNET_CONFIG_BOOTLOADER_DATA_WRITE_SUPPORT     1

// EN: Read fuses and bitlock support (1 - ON, 0 - OFF)
// RU: Поддержка чтения fuse и bitlock (1 - ВКЛ, 0 - ВЫКЛ)
#define CLUNET_CONFIG_BOOTLOADER_FUSES_READ_SUPPORT     1

// EN: Bitlock write support (1 - ON, 0 - OFF)
// RU: Поддержка записи bitlock (1 - ВКЛ, 0 - ВЫКЛ)
#define CLUNET_CONFIG_BOOTLOADER_BITLOCK_WRITE_SUPPORT  1

// EN: Disable Watch Dog Timer on bootloader start (1 - ON, 0 - OFF)
// RU: Отключать сторожевой таймер при запуске загрузчика (1 - ВКЛ, 0 - ВЫКЛ)
#define CLUNET_CONFIG_BOOTLOADER_WDT_DISABLE            1

// EN: Checking for correct reset vector before jump (1 - ON, 0 - OFF)
// RU: Проверка корректности вектора сброса перед переходом (1 - ВКЛ, 0 - ВЫКЛ)
#define CLUNET_CONFIG_BOOTLOADER_CHECK_RESET_VECTOR     1

// EN: Use CRC table for fast CRC calculation (1 - ON, 0 - OFF)
// RU: Использовать таблицу для быстрого вычисления контрольной суммы (1 - ВКЛ, 0 - ВЫКЛ)
#define CLUNET_CONFIG_BOOTLOADER_USE_CRC_TABLE          1

/***************************************
 *  EN: CLUNET LOW LEVEL CONFIGURATION *
 *  RU: КОНФИГУРАЦИЯ CLUNET            *
 ***************************************/

// EN: Period of CLUNET protocol in us. Must be same for all devices in network!
// RU: Период протокола CLUNET. Должен быть одинаковым для всех устройств в сети!
#define CLUNET_CONFIG_PERIOD_US                 64

// EN: Interframe period. For transmit used double.
// RU: Межкадровый интервал, периодов (сброс). Для передачи используется двойной (сброс + пауза).
#define CLUNET_CONFIG_INTERFRAME_PERIODS        5

#endif // _CLUNET_CONFIG_H_
