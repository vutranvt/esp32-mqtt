/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __USER_CONFIG_LOCAL_H__
#define __USER_CONFIG_LOCAL_H__

//#define CPU_FREQ_160MHZ

// config wifi
#define WIFI_SSID "Phong Ky Thuat"
#define WIFI_PASS "123456789"

// config mqtt
#define CLIENT_ID "30:AE:A4:08:6D:38"
#define TOPIC_PUBLISH "hue/30:AE:A4:08:6D:38"
#define TOPIC_SUBSCRIBE "hue/30:AE:A4:08:6D:38/control"
#define TOPIC_LWT "hue/30:AE:A4:08:6D:38/lwt"
#define MESSAGE_LWT "offline"

// config mqtt for sensor device
#define TOPIC_MAC_ADDRESS "/macaddress"
#define TOPIC_FIRMWARE_VERSION "/firmwareversion"

// config ota update firmware
#define FIRMWARE_VERSION "1.0"
#define SERVER_IP   "113.161.21.15"
#define SERVER_PORT "8267"
#define FILENAME "/version1.0.bin"
#define BUFFSIZE 1024
#define TEXT_BUFFSIZE 1024

// config adc
#define PIN_ADC1 (ADC1_CHANNEL_0)		////GPIO 34
#define PIN_ADC2 (ADC1_CHANNEL_3)		////GPIO 35
#define PIN_ADC3 (ADC1_CHANNEL_6)		////GPIO 32

#define adcZero1 2291
#define adcZero2 2217
#define adcZero3 2291

// change to branch develop on sublime


#endif

