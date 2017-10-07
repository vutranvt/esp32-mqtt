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

// #define RAW_ADC		// send raw_adc  to broker

/* config device's LOCATION*/
// #define NTH	
#define BUONMATHUOT	
// #define DANANG	
// #define HUE	
// #define QUYNHON	
// #define BAOLOC	

/* config mqtt user*/
#define MQTT_USERNAME "esp32"
#define MQTT_PASSWORD "mtt@23377"

/* config wifi*/
#if defined(NTH)
    #define WIFI_SSID "Phong Ky Thuat"
    #define WIFI_PASS "123456789"
#else
	#define WIFI_SSID "STARLIGHT"
    #define WIFI_PASS "starlight123"	
#endif

/* config mqtt client id */
#if defined(NTH)
    #define CLIENT_ID "esp_24:0A:C4:0F:17:2C"	
#endif
#if defined(BUONMATHUOT)
	#define CLIENT_ID "esp_24:0A:C4:12:1F:04"	// bmt/current/tudien_tong
#endif
// #define CLIENT_ID "esp_24:0A:C4:10:F9:C8"	



/* config device in NTH */
#if defined(NTH)
    //config mqtt topic
    #define TOPIC_PUBLISH "nth/current/tudien_tong"								
    #define TOPIC_SUBSCRIBE "nth/current/tudien_tong"							
    #define TOPIC_MAC_ADDRESS "nth/current/tudien_tong/mac_address"				
    #define TOPIC_FIRMWARE_VERSION "nth/current/tudien_tong/firmware_version"	
    #define TOPIC_LWT "nth/current/tudien_tong/lwt"								
    #define MESSAGE_LWT "offline"

    //config function of device 
    #define CENTER_NAME "nth"		// trung tam
    #define TYPE "current"			// loai
    #define LOCATION "tudien_tong"	// vi tri
#endif

/* config device in BMT */
#if defined(BUONMATHUOT)	
	#define TOPIC_PUBLISH "bmt/current/tudien_tong"								
	#define TOPIC_SUBSCRIBE "bmt/current/tudien_tong"							
	#define TOPIC_MAC_ADDRESS "bmt/current/tudien_tong/mac_address"				
	#define TOPIC_FIRMWARE_VERSION "bmt/current/tudien_tong/firmware_version"	
	#define TOPIC_LWT "bmt/current/tudien_tong/lwt"								
	#define MESSAGE_LWT "offline"

	//config function of device 
	#define CENTER_NAME "bmt"				
	#define TYPE "current"		
	#define LOCATION "tudien_tong"	
#endif

/* config device in DA NANG */
#if defined(DANANG)	
	#define TOPIC_PUBLISH "dna/current/tudien_tong"								
	#define TOPIC_SUBSCRIBE "dna/current/tudien_tong"							
	#define TOPIC_MAC_ADDRESS "dna/current/tudien_tong/mac_address"				
	#define TOPIC_FIRMWARE_VERSION "dna/current/tudien_tong/firmware_version"	
	#define TOPIC_LWT "dna/current/tudien_tong/lwt"								
	#define MESSAGE_LWT "offline"

	//config function of device 
	#define CENTER_NAME "dna"		
	#define TYPE "current"		
	#define LOCATION "tudien_tong"
#endif

/* config device in HUE */
#if defined(HUE)	
	#define TOPIC_PUBLISH "hue/current/tudien_tong"								
	#define TOPIC_SUBSCRIBE "hue/current/tudien_tong"							
	#define TOPIC_MAC_ADDRESS "hue/current/tudien_tong/mac_address"				
	#define TOPIC_FIRMWARE_VERSION "hue/current/tudien_tong/firmware_version"	
	#define TOPIC_LWT "hue/current/tudien_tong/lwt"								
	#define MESSAGE_LWT "offline"

	//config function of device 
	#define CENTER_NAME "hue"				
	#define TYPE "current"		
	#define LOCATION "tudien_tong"	
#endif


/* config device in BAOLOC */
#if defined(BAOLOC)	
	#define TOPIC_PUBLISH "blo/current/tudien_tong"								
	#define TOPIC_SUBSCRIBE "blo/current/tudien_tong"							
	#define TOPIC_MAC_ADDRESS "blo/current/tudien_tong/mac_address"				
	#define TOPIC_FIRMWARE_VERSION "blo/current/tudien_tong/firmware_version"	
	#define TOPIC_LWT "blo/current/tudien_tong/lwt"								
	#define MESSAGE_LWT "offline"

	//config function of device 
	#define CENTER_NAME "blo"				
	#define TYPE "current"		
	#define LOCATION "tudien_tong"	
#endif

/* config device in QUYNHON */
#if defined(QUYNHON)
	#define TOPIC_PUBLISH "qn/current/tudien_tong"
	#define TOPIC_SUBSCRIBE "qn/current/tudien_tong"
	#define TOPIC_MAC_ADDRESS "qn/current/tudien_tong/mac_address"
	#define TOPIC_FIRMWARE_VERSION "qn/current/tudien_tong/firmware_version"
	#define TOPIC_LWT "qn/current/tudien_tong/lwt"							
	#define MESSAGE_LWT "offline"

	//config function of device 
	#define CENTER_NAME "qn"				
	#define TYPE "current"		
	#define LOCATION "tudien_tong"
#endif


/* config update firmware */
#define FIRMWARE_VERSION "1.0"
#define SERVER_IP   "113.161.21.15"
#define SERVER_PORT "8267"
#define FILENAME "/current-v1.0.bin"
#define BUFFSIZE 1024
#define TEXT_BUFFSIZE 1024

/* config adc */
// (ADC1_CHANNEL_0)		// GPIO 36 - VP
// (ADC1_CHANNEL_1)		// GPIO 37
// (ADC1_CHANNEL_2)		// GPIO 38
// (ADC1_CHANNEL_3)		// GPIO 39 - VN
// (ADC1_CHANNEL_4)		// GPIO 32
// (ADC1_CHANNEL_5)		// GPIO 33
// (ADC1_CHANNEL_6)		// GPIO 34
// (ADC1_CHANNEL_7)		// GPIO 35
#define PIN_ADC1 (ADC1_CHANNEL_0)		// GPIO 36 - VP
#define PIN_ADC2 (ADC1_CHANNEL_7)		// GPIO 35
#define PIN_ADC3 (ADC1_CHANNEL_6)		// GPIO 34

#define adcZero1 2450
#define adcZero2 2424
#define adcZero3 2445

#define CALIBRATION_RATIO 27
#define TI_RATIO 80	//50/5A:10	//150/5A:30


#endif

