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
#define WIFI_SSID "STARLIGHT"
#define WIFI_PASS "starlight123"

//config mqtt user

#define CLIENT_ID "ID_24:0A:C4:0F:17:2C"	// config for each device
#define MQTT_USERNAME "esp32"
#define MQTT_PASSWORD "mtt@23377"

//config mqtt topic
#define TOPIC_PUBLISH "bmt/current/tudien_nong"								// config fot each device
#define TOPIC_SUBSCRIBE "bmt/current/tudien_nong"							// config fot each device
#define TOPIC_MAC_ADDRESS "bmt/current/tudien_nong/macaddress"				// config fot each device
#define TOPIC_FIRMWARE_VERSION "bmt/current/tudien_nong/firmwareversion"	// config fot each device
#define TOPIC_LWT "bmt/current/tudien_nong/lwt"								// config fot each device
#define MESSAGE_LWT "offline"

//config function of device 
#define CENTER_NAME "bmt"				// config for each device
#define DEVICE_FUNCTION "current"		// config for each device
#define DEVICE_POSITION "tudien_nong"	// config for each device

// config ota update firmware
#define FIRMWARE_VERSION "1.0"
#define SERVER_IP   "113.161.21.15"
#define SERVER_PORT "8267"
#define FILENAME "/current-v1.0.bin"
#define BUFFSIZE 1024
#define TEXT_BUFFSIZE 1024

// config adc
#define PIN_ADC1 (ADC1_CHANNEL_0)		////GPIO 34
#define PIN_ADC2 (ADC1_CHANNEL_3)		////GPIO 35
#define PIN_ADC3 (ADC1_CHANNEL_6)		////GPIO 32

#define adcZero1 2291
#define adcZero2 2217
#define adcZero3 2291

#define CALIBRATION_RATIO 22
#define TI_RATIO 30	//50/5A:10	//150/5A:30

// change to branch develop on sublime


#endif

