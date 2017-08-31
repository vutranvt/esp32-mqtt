#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>

#include <sys/socket.h>
#include <netdb.h>

#include "soc/rtc_cntl_reg.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "user_config.sample.h"
#include "debug.h"

#include "mqtt.h"

#include "nvs.h"
#include <stdlib.h>
#include "driver/gpio.h"
#include "driver/adc.h"
// json 
#include "cJSON.h"


// wifi variable 
char macID[48];
uint8_t sta_mac[6];

// config command to control esp32
char rxCmd[7] = "";
int icmd = 7;
char cmdUpdate[7] = "CMD_UPD";  // update new firmware
char cmdGetMac[7] = "CMD_MAC";  // get mac address
char cmdVersion[7] = "CMD_VER"; // get current firmware version


// adc variable 
volatile unsigned int counter1 = 0;
volatile unsigned int counter2 = 0;
volatile unsigned int counter3 = 0;
volatile double ampAdc1 = 0;
volatile double ampAdc2 = 0;
volatile double ampAdc3 = 0;
volatile double maxAmpAdc1 = 0;
volatile double maxAmpAdc2 = 0;
volatile double maxAmpAdc3 = 0;
char msgPublishAdc1[12];
char msgPublishAdc2[6];
char msgPublishAdc3[6];

// config json for mqtt message




void connected_cb(void *self, void *params)
{
    mqtt_client *client = (mqtt_client *)self;
    mqtt_subscribe(client, TOPIC_SUBSCRIBE, 0);
    // mqtt_subscribe(client, TOPIC_PUBLISH, 0);

}
void disconnected_cb(void *self, void *params)
{

}
void reconnect_cb(void *self, void *params)
{

}
void publish_data(void *self, void *params)
{
	mqtt_client *client = (mqtt_client *)self;
    char topicPublish[48] = "";
    char msgPublish[48] = "";

    strcat(topicPublish, TOPIC_PUBLISH);
    strcat(topicPublish, TOPIC_FIRMWARE_VERSION);
    strcat(msgPublish, "FIRMWARE VERSION: ");
    strcat(msgPublish, FIRMWARE_VERSION);

    INFO("[APP] Version: %s\n", topicPublish);


    mqtt_subscribe(client, TOPIC_PUBLISH, 0);
    // mqtt_subscribe(client, TOPIC_PUBLISH, 0);
    vTaskDelay(2000/portTICK_PERIOD_MS);
	mqtt_publish(client, TOPIC_SUBSCRIBE, msgPublish, strlen(msgPublish), 0, 0);
	vTaskDelay(2000/portTICK_PERIOD_MS);
}
void subscribe_cb(void *self, void *params)
{
    INFO("[APP] Subscribe ok, test publish msg\n");
    mqtt_client *client = (mqtt_client *)self;
    mqtt_publish(client, TOPIC_SUBSCRIBE, "reconnected", strlen("reconnected"), 0, 0);
    vTaskDelay(2000/portTICK_PERIOD_MS);
}
/*
my json string = {
    "id" : "30:AE:A4:08:6D:38",
    "current" : {
        "value1" : "112",
        "value2" : "15",
        "value3" : "0"     
    }
}
*/

void publish_cb(void *self, void *params)
{
    INFO("[JSON] cJSON AND PUBLISH .............. ");
	mqtt_client *client = (mqtt_client *)self;
	
	double data1, data2, data3;
	
	data1 = ampAdc1;
	ampAdc1 = 0;
	counter1 = 0;
	data2 = ampAdc2;
	ampAdc2 = 0;
	counter2 = 0;
	data3 = ampAdc3;
	ampAdc3 = 0;
	counter3 = 0;

    // char *my_string = "{\"value1\" : \"123\"}";
    cJSON *root;
    cJSON *curt; 
    root = cJSON_CreateObject();  
    // cJSON_AddItemToObject(root, "id", cJSON_CreateString(CLIENT_ID));
    cJSON_AddItemToObject(root, "id", cJSON_CreateString(CLIENT_ID));
    cJSON_AddItemToObject(root, "current", curt = cJSON_CreateObject());
    // cJSON_AddStringToObject(curt,"type",     "rect");
    cJSON_AddNumberToObject(curt, "value1", data1);
    cJSON_AddNumberToObject(curt, "value2", data2);
    cJSON_AddNumberToObject(curt, "value3", data3);
    // cJSON_AddFalseToObject (curt,"interlace");
    // cJSON_Print(root);
    //cJSON *string = cJSON_Parse(my_string);
    
    //cJSON_Print(string);
    //INFO("[JSON] root = %s\n", string);
    // test_demo git on sublime in "develop" branch
    char *rendered = cJSON_Print(root);
    cJSON_Delete(root);
    INFO("[JSON] root = %s\n", rendered);


	sprintf(msgPublishAdc1, "%.0f", data1);
	sprintf(msgPublishAdc2, "%.0f", data2);
	sprintf(msgPublishAdc3, "%.0f", data3);
	strcat(msgPublishAdc1, "-");
	strcat(msgPublishAdc1, msgPublishAdc2);
	strcat(msgPublishAdc1, "-");
	strcat(msgPublishAdc1, msgPublishAdc3);
	
    
    
    // mqtt_publish(client, TOPIC_PUBLISH, msgPublishAdc1, strlen(msgPublishAdc1), 0, 0);
	mqtt_publish(client, TOPIC_PUBLISH, rendered, strlen(rendered), 0, 0);
	
	vTaskDelay(5000/portTICK_PERIOD_MS);
}

void data_cb(void *self, void *params)
{
    mqtt_client *client = (mqtt_client *)self;
    mqtt_event_data_t *event_data = (mqtt_event_data_t *)params;

    if (event_data->data_offset == 0) {

        char *topic = malloc(event_data->topic_length + 1);
        memcpy(topic, event_data->topic, event_data->topic_length);
        topic[event_data->topic_length] = 0;
        INFO("[APP] Publish topic: %s\n", topic);
        free(topic);
    }

    // char *data = malloc(event_data->data_length + 1);
    // memcpy(data, event_data->data, event_data->data_length);
    // data[event_data->data_length] = 0;
    INFO("[APP] Publish data[%d/%d bytes]\n",
         event_data->data_length + event_data->data_offset,
         event_data->data_total_length);
         // data);

    // free(data);

    ////
    INFO("[APP] Publish data[%s]\n",event_data->data);////

    // get receive command from server
    int i = 0;
    while(i<icmd){
    	rxCmd[i] = event_data->data[i];
    	i++;
    }

    // update new firmware
    if(strcmp(rxCmd, cmdUpdate)==0){
    	INFO("[APP] Publish data[%s]\n",event_data->data);
    	INFO("[APP] update firmware ............\n");

		ota_example_task(client);
    }

    // publish mac address to broker
	if(strcmp(rxCmd, cmdGetMac)==0){
        INFO("[APP] get mac id ............\n");

        char topicPub[48] = "";
        strcat(topicPub, TOPIC_PUBLISH);
        strcat(topicPub, TOPIC_MAC_ADDRESS);

        mqtt_publish(client, topicPub, macID, strlen(macID), 0, 0);
		vTaskDelay(2000/portTICK_PERIOD_MS);
	}

    // publish current firmware version to broker
	if(strcmp(rxCmd, cmdVersion)==0){
        INFO("[APP] get app version ............\n");   

        char topicPub[48] = "";
        strcat(topicPub, TOPIC_PUBLISH);
        strcat(topicPub, TOPIC_FIRMWARE_VERSION);

		mqtt_publish(client, topicPub, FIRMWARE_VERSION, strlen(FIRMWARE_VERSION), 0, 0);
		vTaskDelay(2000/portTICK_PERIOD_MS);
	}
}

mqtt_settings settings = {
    .host = "113.161.21.15",
#if defined(CONFIG_MQTT_SECURITY_ON)         
    .port = 8883, // encrypted
#else
    .port = 1883, // unencrypted
#endif    
    .client_id = CLIENT_ID,
    .username = "esp32",
    .password = "mtt@23377",
    .clean_session = 0,
    .keepalive = 50,
    .lwt_topic = TOPIC_LWT,
    .lwt_msg = MESSAGE_LWT,
    .lwt_qos = 0,
    .lwt_retain = 0,
    .connected_cb = connected_cb,
    .disconnected_cb = disconnected_cb,
    .reconnect_cb = reconnect_cb,
    .subscribe_cb = subscribe_cb,
    .publish_cb = publish_cb,
    .data_cb = data_cb,
	.publish_data = publish_data
};
/*
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
    	mqtt_start(&settings);
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:

        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}*/
/*
static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_SSID,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}*/



static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{

    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        ESP_ERROR_CHECK(esp_wifi_connect());
        break;

    case SYSTEM_EVENT_STA_GOT_IP:

    	//xTaskCreate(&ota_example_task, "ota_example_task", 8192, NULL, 5, NULL);////
    	mqtt_start(&settings);
        // Notice that, all callback will called in mqtt_task
        // All function publish, subscribe
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        
        mqtt_stop();
        ESP_ERROR_CHECK(esp_wifi_connect());
        break;
    default:
        break;
    }
    return ESP_OK;


}


void wifi_conn_init(void)
{
    INFO("[APP] Start, connect to Wifi network: %s ..\n", WIFI_SSID);

    tcpip_adapter_init();

    ESP_ERROR_CHECK( esp_event_loop_init(wifi_event_handler, NULL) );

    wifi_init_config_t icfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&icfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS
        },
    };

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK( esp_wifi_start());
	
	ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac));
	sprintf(macID, "%02X:%02X:%02X:%02X:%02X:%02X", sta_mac[0], sta_mac[1], sta_mac[2], sta_mac[3], sta_mac[4], sta_mac[5]);

}


void adc1Task(void* arg)
{

	unsigned long adcValue = 0;
	unsigned long sampleTime = 0;
    unsigned long maxValue = 0;
    double avr = 0;
    double result = 0;
    double temp;
    //unsigned long startTime = millis();

	// initialize ADC
    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(PIN_ADC1, ADC_ATTEN_6db);

    while(1)
    {
    	sampleTime = 0;
    	avr = 0;
        maxValue = 0;
        while(sampleTime < 2000)
        {
        	adcValue = adc1_get_voltage(PIN_ADC1) - adcZero1;
    		avr = avr + (double)(adcValue * adcValue);
    		/*if(adcValue > maxValue){
                maxValue = adcValue;
            }*/
            sampleTime++;
    		//printf("%d\n",adcValue);
        }

        // temp = sqrt(avr/(double)sampleTime);
        // printf("adc1 = %.2f\n",temp);

    	result = sqrt(avr/(double)sampleTime) * (22/4096.0); // Lay gia tri trung binh
		
    	result = result - 0.10;

    	if(result<0.20)    
            result = 0;
    	else
    		result = result + 0.10;
		
    	result = result * 10;
		// printf("adc1 = %.2f\n",result);
		result = result;
		
    	ampAdc1 = (ampAdc1 + result) / (counter1 + 1);

    	counter1 = 1;

    	vTaskDelay(50/portTICK_PERIOD_MS);
    }
}

void adc2Task(void* arg)
{

	unsigned long adcValue = 0;
	unsigned long sampleTime = 0;
    double avr = 0;
    double result = 0;
    double temp;

	// initialize ADC
    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(PIN_ADC2, ADC_ATTEN_6db);

    while(1)
    {
    	sampleTime = 0;
    	avr = 0;
        while(sampleTime < 2000)
        {
        	adcValue = adc1_get_voltage(PIN_ADC2) - adcZero2;
    		avr = avr + (double)(adcValue * adcValue);
    		sampleTime++;

        }

        temp = sqrt(avr/(double)sampleTime);
        // printf("adc2 = %.2f\n",temp);

    	result = sqrt(avr/(double)sampleTime) * (22/4096.0); // Lay gia tri trung binh
		
		
    	result = result - 0.10;

    	if(result<0.20){
    		result = 0;
    	}else{
    		result = result + 0.10;
    	}
		
		
    	result = result * 10;	//// He so TI
		// printf("adc 2 = %.2f\n",result);
		
    	ampAdc2 = (ampAdc2 + result) / (counter2 + 1);

    	
    	counter2 = 1;

    	vTaskDelay(50/portTICK_PERIOD_MS);
    }
}

void adc3Task(void* arg)
{

	unsigned long adcValue = 0;
	unsigned long sampleTime = 0;
    double avr = 0;
    double result = 0;
    double temp = 0;


	// initialize ADC
    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(PIN_ADC3, ADC_ATTEN_6db);

    while(1)
    {
    	sampleTime = 0;
    	avr = 0;
        while(sampleTime < 2000)
        {
        	adcValue = adc1_get_voltage(PIN_ADC3) - adcZero3;
    		avr = avr + (double)(adcValue * adcValue);
    		sampleTime++;
    		//printf("%d\n",adcValue);
        }

        temp = sqrt(avr/(double)sampleTime);
        // printf("adc3 = %.2f\n",temp);

    	result = sqrt(avr/(double)sampleTime) * (22/4096.0); // Lay gia tri trung binh
		
		
    	result = result - 0.10;

    	if(result<0.20)
    		result = 0;
    	else
    		result = result + 0.10;
    	
    	result = result * 10;
		// printf("adc3 = %.2f\n",result);

    	ampAdc3 = (ampAdc3 + result) / (counter3 + 1);
    	counter3 = 1;

    	vTaskDelay(50/portTICK_PERIOD_MS);
    }
}


void app_main()
{
    INFO("[APP] Startup..\n");
	INFO("[APP] VERSION 1.0 ....................................................................\n");
    INFO("[APP] Free memory: %d bytes\n", system_get_free_heap_size());
    INFO("[APP] SDK version: %s, Build time: %s\n", system_get_sdk_version(), BUID_TIME);

#ifdef CPU_FREQ_160MHZ
    INFO("[APP] Setup CPU run as 160MHz\n");
    SET_PERI_REG_BITS(RTC_CLK_CONF, RTC_CNTL_SOC_CLK_SEL, 0x1, RTC_CNTL_SOC_CLK_SEL_S);
    WRITE_PERI_REG(CPU_PER_CONF_REG, 0x01);
    INFO("[APP] Setup CPU run as 160MHz - Done\n");
#endif
 
    nvs_flash_init();
    wifi_conn_init();
		
    xTaskCreate(adc1Task, "adc1Task", 1024*3, NULL, 10, NULL);////
	xTaskCreate(adc2Task, "adc2Task", 1024*3, NULL, 10, NULL);////
    xTaskCreate(adc3Task, "adc3Task", 1024*3, NULL, 10, NULL);////

}
