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
char cmdUpdate[20] = "update";  // update new firmware
char cmdGetMac[20] = "get_mac_address";  // get mac address
char cmdGetVersion[20] = "get_version"; // get current firmware version


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


/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;


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
    // strcat(topicPublish, DEVICE_FUCTION);
    // strcat(topicPublish, "/");
    // strcat(topicPublish, DEVICE_POSITION);
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

/*json_command from server to esp32
json_cmd = {
    "id" : "30:AE:A4:08:6D:38", // sensor mac id 
    "cmd" : "update"            // "get_version" "adc_mode_max" "adc_mode_average"
}
*/

char *jsonCmdDecode(char *string) {
    cJSON *jsonCmd;
    jsonCmd = cJSON_Parse(string);
    //char *jsonID = cJSON_GetObjectItem(jsonCmd, "id")->valuestring;
    char *jsonMsg = cJSON_Print(jsonCmd);
    char *jsonID = "";
    cJSON_Delete(jsonCmd);
    INFO("[JSON] root = %s\n", jsonMsg);
    return jsonID;
}

/* json_data send to server
json_data = {
    "mac" : "30:AE:A4:08:6D:38", // sensor mac id 
    "center" : "bmt",
    "position" : "tudien_nong",
    "current" : {
        "value1" : "112",   // phase1 value
        "value2" : "15",    // phase2 value
        "value3" : "0"      // phase3 value
    }
}
*/
cJSON *jsonDataEncode (double data1, double data2, double data3) {
    cJSON *jsonString;  // json string
    cJSON *current;     // json child
    
    jsonString = cJSON_CreateObject();  

    cJSON_AddItemToObject(jsonString, "mac", cJSON_CreateString(CLIENT_ID));
    cJSON_AddItemToObject(jsonString, "center", cJSON_CreateString(CENTER_NAME));
    cJSON_AddItemToObject(jsonString, "position", cJSON_CreateString(DEVICE_POSITION));
    cJSON_AddItemToObject(jsonString, "current", current = cJSON_CreateObject());
    cJSON_AddNumberToObject(current, "value1", data1);
    cJSON_AddNumberToObject(current, "value2", data2);
    cJSON_AddNumberToObject(current, "value3", data3);

    return jsonString;
}


 
void publish_cb(void *self, void *params)
{
    INFO("[JSON] cJSON AND PUBLISH .............. ");
	mqtt_client *client = (mqtt_client *)self;
	
	double data1, data2, data3;
	
	data1 = ceilf(ampAdc1);
	ampAdc1 = 0;
	counter1 = 0;
	data2 = ceilf(ampAdc2);
	ampAdc2 = 0;
	counter2 = 0;
	data3 = ceilf(ampAdc3);
	ampAdc3 = 0;
	counter3 = 0;

    cJSON *root;
    root = jsonDataEncode(data1, data2, data3);
    char *jsonMsg = cJSON_Print(root);
    cJSON_Delete(root);
    // INFO("[JSON] root = %s\n", jsonMsg);
    
	mqtt_publish(client, TOPIC_PUBLISH, jsonMsg, strlen(jsonMsg), 0, 0);
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

    INFO("[APP] Publish data[%d/%d bytes]\n", event_data->data_length + event_data->data_offset, event_data->data_total_length);

    uint16_t i = 0;
    uint16_t j = event_data->data_total_length;
    //char c= "";
    int count = 0;
    int m = 0;
    int n = 0;
    char mac[30] = "";
    char cmd[30] = "";

    while(i<j){
        char c = event_data->data[i];
        if(c=='\"') 
            count++;
        if(count==3){
            if(m!=0)
                mac[m-1] = c;
            m++;
        }
        if(count==7){
            if(n!=0)
                cmd[n-1] = c;
            n++;
        }
        i++;
    }

    if(!strcmp(mac, macID)){
        // update firmware
        INFO("[APP] update firmware ............\n");
        if(strcmp(cmd, cmdUpdate)==0){
            INFO("[APP] Publish data[%s]\n",event_data->data);
            INFO("[APP] update firmware ............\n");

            ota_example_task(client);
        }

        // publish mac address to broker
        if(strcmp(cmd, cmdGetMac)==0){
            INFO("[APP] get mac id ............\n");

            char topicPub[48] = "";
            strcat(topicPub, TOPIC_PUBLISH);
            strcat(topicPub, TOPIC_MAC_ADDRESS);

            mqtt_publish(client, topicPub, macID, strlen(macID), 0, 0);
            vTaskDelay(2000/portTICK_PERIOD_MS);
        }

        // publish current firmware version to broker
        if(strcmp(cmd, cmdGetVersion)==0){
            INFO("[APP] get app version ............\n");   

            char topicPub[48] = "";
            strcat(topicPub, TOPIC_PUBLISH);
            strcat(topicPub, TOPIC_FIRMWARE_VERSION);

            mqtt_publish(client, topicPub, FIRMWARE_VERSION, strlen(FIRMWARE_VERSION), 0, 0);
            vTaskDelay(2000/portTICK_PERIOD_MS);
        }
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
    .username = MQTT_USERNAME,
    .password = MQTT_PASSWORD,
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
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        mqtt_start(&settings);
        // Notice that, all callback will called in mqtt_task
        // All function publish, subscribe
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        mqtt_stop();
        //ESP_ERROR_CHECK(esp_wifi_connect());
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);    
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
    wifi_event_group = xEventGroupCreate();
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
