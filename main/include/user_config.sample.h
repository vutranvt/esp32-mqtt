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

//config mqtt user
// #define CLIENT_ID "24:0A:C4:10:F9:C8"	// asjust mqtt id client for each device
#define CLIENT_ID "30:AE:A4:08:6D:38"	
#define MQTT_USERNAME "esp32"
#define MQTT_PASSWORD "mtt@23377"

//config mqtt topic
#define TOPIC_PUBLISH "hue/current/tudien_2"				// config topic: "function/ position" for device
#define TOPIC_SUBSCRIBE "hue/current/tudien_2"
#define TOPIC_LWT "hue/current/tudien_2/lwt"
#define MESSAGE_LWT "offline"

//config function of device 
#define CENTER_NAME "hue"			// config center for each device
#define DEVICE_FUNCTION "current"	// config function for each device
#define DEVICE_POSITION "tudien_2"	// config position of device

// config mqtt for sensor device
#define TOPIC_MAC_ADDRESS "/macaddress"
#define TOPIC_FIRMWARE_VERSION "/firmwareversion"

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

// change to branch develop on sublime


#endif

