/*
* @Author: Tuan PM
* @Date:   2016-09-10 09:33:06
* @Last Modified by:   Tuan PM
* @Last Modified time: 2017-02-15 13:11:53
*/
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "ringbuf.h"
#include "mqtt.h"

#include <user_config.sample.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "driver/adc.h"

#include "nvs.h"
#include "freertos/event_groups.h"
#include <sys/socket.h>
#include <netdb.h>

#include "soc/rtc_cntl_reg.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"

#include "cJSON.h"




static TaskHandle_t xMqttTask = NULL;
static TaskHandle_t xMqttSendingTask = NULL;
static TaskHandle_t xAdc2Task = NULL;
static TaskHandle_t xMqttStartReceiveSchedule = NULL;

static bool terminate_mqtt = false;

////ota variable
static const char *TAG = "ota";
/*an ota data write buffer ready to write to the flash*/
static char ota_write_data[BUFFSIZE + 1] = { 0 };
/*an packet receive buffer*/
static char text[BUFFSIZE + 1] = { 0 };
/* an image total length*/
static int binary_file_length = 0;
/*socket id*/
static int socket_id = -1;
static char http_request[64] = {0};


static int resolve_dns(const char *host, struct sockaddr_in *ip) {
    struct hostent *he;
    struct in_addr **addr_list;
    he = gethostbyname(host);
    if (he == NULL) return 0;
    addr_list = (struct in_addr **)he->h_addr_list;
    if (addr_list[0] == NULL) return 0;
    ip->sin_family = AF_INET;
    memcpy(&ip->sin_addr, addr_list[0], sizeof(ip->sin_addr));
    return 1;
}
static void mqtt_queue(mqtt_client *client)
{
    int msg_len;
    while (rb_available(&client->send_rb) < client->mqtt_state.outbound_message->length) {
        xQueueReceive(client->xSendingQueue, &msg_len, 1000 / portTICK_RATE_MS);
        rb_read(&client->send_rb, client->mqtt_state.out_buffer, msg_len);
    }
    rb_write(&client->send_rb,
             client->mqtt_state.outbound_message->data,
             client->mqtt_state.outbound_message->length);
    xQueueSend(client->xSendingQueue, &client->mqtt_state.outbound_message->length, 0);
}

static bool client_connect(mqtt_client *client)
{
    int ret;
    struct sockaddr_in remote_ip;

    while (1) {

        bzero(&remote_ip, sizeof(struct sockaddr_in));
        remote_ip.sin_family = AF_INET;
        remote_ip.sin_port = htons(client->settings->port);


        //if host is not ip address, resolve it
        if (inet_aton( client->settings->host, &(remote_ip.sin_addr)) == 0) {
            mqtt_info("Resolve dns for domain: %s", client->settings->host);

            if (!resolve_dns(client->settings->host, &remote_ip)) {
                vTaskDelay(1000 / portTICK_RATE_MS);
                continue;
            }
        }


#if defined(CONFIG_MQTT_SECURITY_ON)  // ENABLE MQTT OVER SSL
        client->ctx = NULL;
        client->ssl = NULL;

        client->ctx = SSL_CTX_new(TLSv1_2_client_method());
        if (!client->ctx) {
            mqtt_error("Failed to create SSL CTX");
            goto failed1;
        }
#endif

        client->socket = socket(PF_INET, SOCK_STREAM, 0);
        if (client->socket == -1) {
            mqtt_error("Failed to create socket");
            goto failed2;
        }



        mqtt_info("Connecting to server %s:%d,%d",
                  inet_ntoa((remote_ip.sin_addr)),
                  client->settings->port,
                  remote_ip.sin_port);


        if (connect(client->socket, (struct sockaddr *)(&remote_ip), sizeof(struct sockaddr)) != 00) {
            mqtt_error("Connect failed");
            goto failed3;
        }

#if defined(CONFIG_MQTT_SECURITY_ON)  // ENABLE MQTT OVER SSL
        mqtt_info("Creating SSL object...");
        client->ssl = SSL_new(client->ctx);
        if (!client->ssl) {
            mqtt_error("Unable to creat new SSL");
            goto failed3;
        }

        if (!SSL_set_fd(client->ssl, client->socket)) {
            mqtt_error("SSL set_fd failed");
            goto failed3;
        }

        mqtt_info("Start SSL connect..");
        ret = SSL_connect(client->ssl);
        if (!ret) {
            mqtt_error("SSL Connect FAILED");
            goto failed4;
        }
#endif
        mqtt_info("Connected!");

        return true;

        //failed5:
        //   SSL_shutdown(client->ssl);

#if defined(CONFIG_MQTT_SECURITY_ON)
        failed4:
          SSL_free(client->ssl);
          client->ssl = NULL;
#endif

        failed3:
          close(client->socket);
          client->socket = -1;

        failed2:
#if defined(CONFIG_MQTT_SECURITY_ON)
          SSL_CTX_free(client->ctx);

        failed1:
          client->ctx = NULL;
#endif
         vTaskDelay(1000 / portTICK_RATE_MS);

     }
}


// Close client socket
// including SSL objects if CNFIG_MQTT_SECURITY_ON is enabled
void closeclient(mqtt_client *client)
{

#if defined(CONFIG_MQTT_SECURITY_ON)
        if (client->ssl != NULL)
        {
          SSL_shutdown(client->ssl);

          SSL_free(client->ssl);
          client->ssl = NULL;
        }
#endif
        if (client->socket != -1)
        {
          close(client->socket);
          client->socket = -1;
        }

#if defined(CONFIG_MQTT_SECURITY_ON)
        if (client->ctx != NULL)
        {
          SSL_CTX_free(client->ctx);
          client->ctx = NULL;
        }
#endif

}
/*
 * mqtt_connect
 * input - client
 * return 1: success, 0: fail
 */
static bool mqtt_connect(mqtt_client *client)
{
    int write_len, read_len, connect_rsp_code;
    struct timeval tv;

    tv.tv_sec = 10;  /* 30 Secs Timeout */
    tv.tv_usec = 0;  // Not init'ing this can cause strange errors

    setsockopt(client->socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));

    mqtt_msg_init(&client->mqtt_state.mqtt_connection,
                  client->mqtt_state.out_buffer,
                  client->mqtt_state.out_buffer_length);
    client->mqtt_state.outbound_message = mqtt_msg_connect(&client->mqtt_state.mqtt_connection,
                                          client->mqtt_state.connect_info);
    client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
    client->mqtt_state.pending_msg_id = mqtt_get_id(client->mqtt_state.outbound_message->data,
                                        client->mqtt_state.outbound_message->length);
    mqtt_info("Sending MQTT CONNECT message, type: %d, id: %04X",
              client->mqtt_state.pending_msg_type,
              client->mqtt_state.pending_msg_id);

    write_len = ClientWrite(
                      client->mqtt_state.outbound_message->data,
                      client->mqtt_state.outbound_message->length);

    mqtt_info("Reading MQTT CONNECT response message");

    read_len = ClientRead(client->mqtt_state.in_buffer, CONFIG_MQTT_BUFFER_SIZE_BYTE);

    tv.tv_sec = 0;  /* No timeout */
    setsockopt(client->socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));

    if (read_len < 0) {
        mqtt_error("Error network response");
        return false;
    }
    if (mqtt_get_type(client->mqtt_state.in_buffer) != MQTT_MSG_TYPE_CONNACK) {
        mqtt_error("Invalid MSG_TYPE response: %d, read_len: %d", mqtt_get_type(client->mqtt_state.in_buffer), read_len);
        return false;
    }
    connect_rsp_code = mqtt_get_connect_return_code(client->mqtt_state.in_buffer);
    switch (connect_rsp_code) {
        case CONNECTION_ACCEPTED:
            mqtt_info("Connected");
            return true;
        case CONNECTION_REFUSE_PROTOCOL:
        case CONNECTION_REFUSE_SERVER_UNAVAILABLE:
        case CONNECTION_REFUSE_BAD_USERNAME:
        case CONNECTION_REFUSE_NOT_AUTHORIZED:
            mqtt_warn("Connection refuse, reason code: %d", connect_rsp_code);
            return false;
        default:
            mqtt_warn("Connection refuse, Unknow reason");
            return false;
    }
    return false;
}

void mqtt_sending_task(void *pvParameters)
{
    mqtt_client *client = (mqtt_client *)pvParameters;
    uint32_t msg_len, send_len;
	bool connected = true;
    mqtt_info("mqtt_sending_task");

	// mqtt_publish(client, TOPIC_SUBSCRIBE, FIRMWARE_VERSION, strlen(FIRMWARE_VERSION), 0, 0);
	// vTaskDelay(2000/portTICK_PERIOD_MS);
	client->settings->publish_data(client, NULL);
	
    while (connected) {

        if (xQueueReceive(client->xSendingQueue, &msg_len, 1000 / portTICK_RATE_MS)) {
            //queue available
            while (msg_len > 0) {
                send_len = msg_len;
                if (send_len > CONFIG_MQTT_BUFFER_SIZE_BYTE)
                    send_len = CONFIG_MQTT_BUFFER_SIZE_BYTE;
                mqtt_info("Sending...%d bytes", send_len);

                rb_read(&client->send_rb, client->mqtt_state.out_buffer, send_len);
                client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.out_buffer);
                client->mqtt_state.pending_msg_id = mqtt_get_id(client->mqtt_state.out_buffer, send_len);
                ClientWrite(client->mqtt_state.out_buffer, send_len);

                //TODO: Check sending type, to callback publish message
                msg_len -= send_len;
            }
            //invalidate keepalive timer
            client->keepalive_tick = client->settings->keepalive / 2;
        }
        else {
            if (client->keepalive_tick > 0) client->keepalive_tick --;
            else {
                client->keepalive_tick = client->settings->keepalive / 2;
                client->mqtt_state.outbound_message = mqtt_msg_pingreq(&client->mqtt_state.mqtt_connection);
                client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
                client->mqtt_state.pending_msg_id = mqtt_get_id(client->mqtt_state.outbound_message->data,
                                                    client->mqtt_state.outbound_message->length);
                mqtt_info("Sending pingreq");
                ClientWrite(
                      client->mqtt_state.outbound_message->data,
                      client->mqtt_state.outbound_message->length);
				connected = false;
            }
			
        }
        mqtt_info("End of mqtt_sending_task............................................................");////
    }
	
	closeclient(client);
    xMqttSendingTask = NULL;
    vTaskDelete(NULL);

}

void deliver_publish(mqtt_client *client, uint8_t *message, int length)
{
    mqtt_event_data_t event_data;
    int len_read, total_mqtt_len = 0, mqtt_len = 0, mqtt_offset = 0;

    do
    {
        event_data.topic_length = length;
        event_data.topic = mqtt_get_publish_topic(message, &event_data.topic_length);
        event_data.data_length = length;
        event_data.data = mqtt_get_publish_data(message, &event_data.data_length);

        if(total_mqtt_len == 0){
            total_mqtt_len = client->mqtt_state.message_length - client->mqtt_state.message_length_read + event_data.data_length;
            mqtt_len = event_data.data_length;
        } else {
            mqtt_len = len_read;
        }

        event_data.data_total_length = total_mqtt_len;
        event_data.data_offset = mqtt_offset;
        event_data.data_length = mqtt_len;

        mqtt_info("Data received: %d/%d bytes ", mqtt_len, total_mqtt_len);
        // config restart if the server broker happenned stopped  v
        if(total_mqtt_len < 0)  esp_restart();

        if(client->settings->data_cb) {
            client->settings->data_cb(client, &event_data);
        }
        mqtt_offset += mqtt_len;
        if (client->mqtt_state.message_length_read >= client->mqtt_state.message_length)
            break;

        len_read = ClientRead(client->mqtt_state.in_buffer, CONFIG_MQTT_BUFFER_SIZE_BYTE);
        client->mqtt_state.message_length_read += len_read;
    } while (1);

}
void mqtt_start_receive_schedule(mqtt_client* client)
{
    int read_len;
    uint8_t msg_type;
    uint8_t msg_qos;
    uint16_t msg_id;

    while (1) {
    	mqtt_info("Start mqtt_start_receive_schedule............................................................");////
		
		if (terminate_mqtt) break;
    	if (xMqttSendingTask == NULL) break;
		
        read_len = ClientRead(client->mqtt_state.in_buffer, CONFIG_MQTT_BUFFER_SIZE_BYTE);

        mqtt_info("Read len %d", read_len);
        if (read_len == 0)
            break;

        msg_type = mqtt_get_type(client->mqtt_state.in_buffer);
        msg_qos = mqtt_get_qos(client->mqtt_state.in_buffer);
        msg_id = mqtt_get_id(client->mqtt_state.in_buffer, client->mqtt_state.in_buffer_length);
        // mqtt_info("msg_type %d, msg_id: %d, pending_id: %d", msg_type, msg_id, client->mqtt_state.pending_msg_type);
        switch (msg_type)
        {
            case MQTT_MSG_TYPE_SUBACK:
                if (client->mqtt_state.pending_msg_type == MQTT_MSG_TYPE_SUBSCRIBE && client->mqtt_state.pending_msg_id == msg_id) {
                    mqtt_info("Subscribe successful");
                    if (client->settings->subscribe_cb) {
                        client->settings->subscribe_cb(client, NULL);
                    }
                }
                break;
            case MQTT_MSG_TYPE_UNSUBACK:
                if (client->mqtt_state.pending_msg_type == MQTT_MSG_TYPE_UNSUBSCRIBE && client->mqtt_state.pending_msg_id == msg_id)
                    mqtt_info("UnSubscribe successful");
                break;
            case MQTT_MSG_TYPE_PUBLISH:
				client->settings->publish_cb(client, NULL);
				
                if (msg_qos == 1)
                    client->mqtt_state.outbound_message = mqtt_msg_puback(&client->mqtt_state.mqtt_connection, msg_id);
                else if (msg_qos == 2)
                    client->mqtt_state.outbound_message = mqtt_msg_pubrec(&client->mqtt_state.mqtt_connection, msg_id);

                if (msg_qos == 1 || msg_qos == 2) {
                    mqtt_info("Queue response QoS: %d", msg_qos);
                    mqtt_queue(client);
                    // if (QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1) {
                    //     mqtt_info("MQTT: Queue full");
                    // }
                }
				client->mqtt_state.message_length_read = read_len;
                client->mqtt_state.message_length = mqtt_get_total_length(client->mqtt_state.in_buffer, client->mqtt_state.message_length_read);
                mqtt_info("deliver_publish");

                deliver_publish(client, client->mqtt_state.in_buffer, client->mqtt_state.message_length_read);
                break;
            case MQTT_MSG_TYPE_PUBACK:
                if (client->mqtt_state.pending_msg_type == MQTT_MSG_TYPE_PUBLISH && client->mqtt_state.pending_msg_id == msg_id) {
                    mqtt_info("received MQTT_MSG_TYPE_PUBACK, finish QoS1 publish");
                }

                break;
            case MQTT_MSG_TYPE_PUBREC:
                client->mqtt_state.outbound_message = mqtt_msg_pubrel(&client->mqtt_state.mqtt_connection, msg_id);
                mqtt_queue(client);
                break;
            case MQTT_MSG_TYPE_PUBREL:
                client->mqtt_state.outbound_message = mqtt_msg_pubcomp(&client->mqtt_state.mqtt_connection, msg_id);
                mqtt_queue(client);

                break;
            case MQTT_MSG_TYPE_PUBCOMP:
                if (client->mqtt_state.pending_msg_type == MQTT_MSG_TYPE_PUBLISH && client->mqtt_state.pending_msg_id == msg_id) {
                    mqtt_info("Receive MQTT_MSG_TYPE_PUBCOMP, finish QoS2 publish");
                }
                break;
            case MQTT_MSG_TYPE_PINGREQ:
                client->mqtt_state.outbound_message = mqtt_msg_pingresp(&client->mqtt_state.mqtt_connection);
                mqtt_queue(client);
                break;
            case MQTT_MSG_TYPE_PINGRESP:
                mqtt_info("MQTT_MSG_TYPE_PINGRESP");
                // Ignore
                //client->settings->publish_cb(client, NULL);////
                break;
        }
        mqtt_info("End of mqtt_start_receive_schedule............................................................");////
    }
    mqtt_info("network disconnected");
    esp_restart();  // config restart -v
}

void mqtt_destroy(mqtt_client *client)
{
    free(client->mqtt_state.in_buffer);
    free(client->mqtt_state.out_buffer);
    free(client);
    vTaskDelete(xMqttSendingTask);
	vTaskDelete(xMqttStartReceiveSchedule);
	
    esp_restart();
}

void mqtt_task(void *pvParameters)
{
    mqtt_client *client = (mqtt_client *)pvParameters;
	mqtt_info("Start MQTT _TASK........................");
	
	
    while (1) {
        if (terminate_mqtt) break;
		
		client_connect(client);
		
        mqtt_info("Connected to server %s:%d", client->settings->host, client->settings->port);
        if (!mqtt_connect(client)) {
			closeclient(client);
			continue;
            //return;
        }
        mqtt_info("Connected to MQTT broker, create sending thread before call connected callback");
		

		
        xTaskCreate(&mqtt_sending_task, "mqtt_sending_task", 2048, client, CONFIG_MQTT_PRIORITY + 1, &xMqttSendingTask);
		
        if (client->settings->connected_cb) {
        	mqtt_info("CONNECT CB............");
            client->settings->connected_cb(client, NULL);
        }

        mqtt_info("mqtt_start_receive_schedule");
		//xTaskCreate(&mqtt_start_receive_schedule, "mqtt_start_receive_schedule", 2048, client, CONFIG_MQTT_PRIORITY + 2, &xMqttStartReceiveSchedule);////
        mqtt_start_receive_schedule(client);
		
		/* 
		while(xMqttSendingTask!=NULL){	
		} */
		
		if (xMqttSendingTask != NULL) {
        	vTaskDelete(xMqttSendingTask);
        	vTaskDelete(xMqttStartReceiveSchedule);
        }
		
        //closeclient(client);
        //mqtt_destroy(client);
		//vTaskDelete(xMqttSendingTask);
		vTaskDelay(1000 / portTICK_RATE_MS);

    }
    mqtt_info("End of Mqtt-Task:::::::::::::::::");
    mqtt_destroy(client);

}

mqtt_client *mqtt_start(mqtt_settings *settings)
{
    int stackSize = 4096;

    uint8_t *rb_buf;
	terminate_mqtt = false;////
    if (xMqttTask != NULL)
        return NULL;
    mqtt_client *client = malloc(sizeof(mqtt_client));

    if (client == NULL) {
        mqtt_error("Memory not enough");
        return NULL;
    }
    memset(client, 0, sizeof(mqtt_client));

    client->settings = settings;
    client->connect_info.client_id = settings->client_id;
    client->connect_info.username = settings->username;
    client->connect_info.password = settings->password;
    client->connect_info.will_topic = settings->lwt_topic;
    client->connect_info.will_message = settings->lwt_msg;
    client->connect_info.will_qos = settings->lwt_qos;
    client->connect_info.will_retain = settings->lwt_retain;


    client->keepalive_tick = settings->keepalive / 2;

    client->connect_info.keepalive = settings->keepalive;
    client->connect_info.clean_session = settings->clean_session;

    client->mqtt_state.in_buffer = (uint8_t *)malloc(CONFIG_MQTT_BUFFER_SIZE_BYTE);
    client->mqtt_state.in_buffer_length = CONFIG_MQTT_BUFFER_SIZE_BYTE;
    client->mqtt_state.out_buffer =  (uint8_t *)malloc(CONFIG_MQTT_BUFFER_SIZE_BYTE);
    client->mqtt_state.out_buffer_length = CONFIG_MQTT_BUFFER_SIZE_BYTE;
    client->mqtt_state.connect_info = &client->connect_info;

    client->socket = -1;

#if defined(CONFIG_MQTT_SECURITY_ON)  // ENABLE MQTT OVER SSL
    client->ctx = NULL;
    client->ssl = NULL;
    stackSize = 10240; // Need more stack to handle SSL handshake
#endif

    /* Create a queue capable of containing 64 unsigned long values. */
    client->xSendingQueue = xQueueCreate(64, sizeof( uint32_t ));
    rb_buf = (uint8_t*) malloc(CONFIG_MQTT_QUEUE_BUFFER_SIZE_WORD * 4);

    if (rb_buf == NULL) {
        mqtt_error("Memory not enough");
        return NULL;
    }

    rb_init(&client->send_rb, rb_buf, CONFIG_MQTT_QUEUE_BUFFER_SIZE_WORD * 4, 1);

    mqtt_msg_init(&client->mqtt_state.mqtt_connection,
                  client->mqtt_state.out_buffer,
                  client->mqtt_state.out_buffer_length);

    xTaskCreate(&mqtt_task, "mqtt_task", stackSize, client, CONFIG_MQTT_PRIORITY, &xMqttTask);

    mqtt_info("End of XTaskCreate-mqtt_task....................................................................\r\n");
    return client;
}


void mqtt_subscribe(mqtt_client *client, char *topic, uint8_t qos)
{
    client->mqtt_state.outbound_message = mqtt_msg_subscribe(&client->mqtt_state.mqtt_connection,
                                          topic, qos,
                                          &client->mqtt_state.pending_msg_id);
    mqtt_info("Queue subscribe, topic\"%s\", id: %d", topic, client->mqtt_state.pending_msg_id);
    mqtt_queue(client);
}

void mqtt_publish(mqtt_client* client, char *topic, char *data, int len, int qos, int retain)
{

    client->mqtt_state.outbound_message = mqtt_msg_publish(&client->mqtt_state.mqtt_connection,
                                          topic, data, len,
                                          qos, retain,
                                          &client->mqtt_state.pending_msg_id);
    mqtt_queue(client);
    mqtt_info("Queuing publish, length: %d, queue size(%d/%d)\r\n",
              client->mqtt_state.outbound_message->length,
              client->send_rb.fill_cnt,
              client->send_rb.size);
}

void mqtt_stop()
{
	terminate_mqtt = true;
}


/*read buffer by byte still delim ,return read bytes counts*/
static int read_until(char *buffer, char delim, int len)
{
//  /*TODO: delim check,buffer check,further: do an buffer length limited*/
    int i = 0;
    while (buffer[i] != delim && i < len) {
        ++i;
    }
    return i + 1;
}

/* resolve a packet from http socket
 * return true if packet including \r\n\r\n that means http packet header finished,start to receive packet body
 * otherwise return false
 * */
static bool read_past_http_header(char text[], int total_len, esp_ota_handle_t update_handle)
{
    /* i means current position */
    int i = 0, i_read_len = 0;
    while (text[i] != 0 && i < total_len) {
        i_read_len = read_until(&text[i], '\n', total_len);
        // if we resolve \r\n line,we think packet header is finished
        if (i_read_len == 2) {
            int i_write_len = total_len - (i + 2);
            memset(ota_write_data, 0, BUFFSIZE);
            /*copy first http packet body to write buffer*/
            memcpy(ota_write_data, &(text[i + 2]), i_write_len);

            esp_err_t err = esp_ota_write( update_handle, (const void *)ota_write_data, i_write_len);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error: esp_ota_write failed! err=0x%x", err);
                return false;
            } else {
                ESP_LOGI(TAG, "esp_ota_write header OK");
                binary_file_length += i_write_len;
            }
            return true;
        }
        i += i_read_len;
    }
    return false;
}

static bool connect_to_http_server()
{
    ESP_LOGI(TAG, "Server IP: %s Server Port:%s", SERVER_IP, SERVER_PORT);
    sprintf(http_request, "GET %s HTTP/1.1\r\nHost: %s:%s \r\n\r\n", FILENAME, SERVER_IP, SERVER_PORT);

    int  http_connect_flag = -1;
    struct sockaddr_in sock_info;

    socket_id = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_id == -1) {
        ESP_LOGE(TAG, "Create socket failed!");
        return false;
    }

    // set connect info
    memset(&sock_info, 0, sizeof(struct sockaddr_in));
    sock_info.sin_family = AF_INET;
    sock_info.sin_addr.s_addr = inet_addr(SERVER_IP);
    sock_info.sin_port = htons(atoi(SERVER_PORT));

    // connect to http server
    http_connect_flag = connect(socket_id, (struct sockaddr *)&sock_info, sizeof(sock_info));
    if (http_connect_flag == -1) {
        ESP_LOGE(TAG, "Connect to server failed! errno=%d", errno);
        close(socket_id);
        return false;
    } else {
        ESP_LOGI(TAG, "Connected to server");
        return true;
    }
    return false;
}

static void __attribute__((noreturn)) task_fatal_error(mqtt_client* client)
{
    ESP_LOGE(TAG, "Exiting task due to fatal error...");
    close(socket_id);
	//updateFailed();

	esp_restart();

    (void)vTaskDelete(NULL);

    /*
	while (1) {
        ;
    }*/

}

//static void ota_example_task(void *pvParameter)
void ota_example_task(mqtt_client* client)
{
    esp_err_t err;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;

    ESP_LOGI(TAG, "Starting OTA example...");

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    assert(configured == running); /* fresh from reset, should be running from configured boot partition */
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             configured->type, configured->subtype, configured->address);

    /* Wait for the callback to set the CONNECTED_BIT in the
       event group.
    */
    ////xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,false, true, portMAX_DELAY);
	
    ESP_LOGI(TAG, "Connect to Wifi ! Start to Connect to Server....");

    /*connect to http server*/
    if (connect_to_http_server()) {
        ESP_LOGI(TAG, "Connected to http server");
    } else {
        ESP_LOGE(TAG, "Connect to http server failed!");
        task_fatal_error(client);
    }

    int res = -1;
    /*send GET request to http server*/
    res = send(socket_id, http_request, strlen(http_request), 0);
    if (res == -1) {
        ESP_LOGE(TAG, "Send GET request to server failed");
        task_fatal_error(client);
    } else {
        ESP_LOGI(TAG, "Send GET request to server succeeded");
    }

    update_partition = esp_ota_get_next_update_partition(NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);
    assert(update_partition != NULL);

    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed, error=%d", err);
        task_fatal_error(client);
    }
    ESP_LOGI(TAG, "esp_ota_begin succeeded");

    bool resp_body_start = false, flag = true;
    /*deal with all receive packet*/
    while (flag) {
        memset(text, 0, TEXT_BUFFSIZE);
        memset(ota_write_data, 0, BUFFSIZE);
        int buff_len = recv(socket_id, text, TEXT_BUFFSIZE, 0);
        if (buff_len < 0) { /*receive error*/
            ESP_LOGE(TAG, "Error: receive data error! errno=%d", errno);
            task_fatal_error(client);
        } else if (buff_len > 0 && !resp_body_start) { /*deal with response header*/
            memcpy(ota_write_data, text, buff_len);
            resp_body_start = read_past_http_header(text, buff_len, update_handle);
        } else if (buff_len > 0 && resp_body_start) { /*deal with response body*/
            memcpy(ota_write_data, text, buff_len);
            err = esp_ota_write( update_handle, (const void *)ota_write_data, buff_len);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error: esp_ota_write failed! err=0x%x", err);
                task_fatal_error(client);
            }
            binary_file_length += buff_len;
            ESP_LOGI(TAG, "Have written image length %d", binary_file_length);
        } else if (buff_len == 0) {  /*packet over*/
            flag = false;
            ESP_LOGI(TAG, "Connection closed, all packets received");
            close(socket_id);
        } else {
            ESP_LOGE(TAG, "Unexpected recv result");
        }
    }

    ESP_LOGI(TAG, "Total Write binary data length : %d", binary_file_length);

    if (esp_ota_end(update_handle) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed!");
		task_fatal_error(client);
    }
    err = esp_ota_set_boot_partition(update_partition);
	
	
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed! err=0x%x", err);
        task_fatal_error(client);
    }
		ESP_LOGI(TAG, "Prepare to restart system!");
		esp_restart();
			// mqtt_publish(client, "hue/device1", "update failed", strlen("update failed"), 0, 0);
		// vTaskDelay(4000/portTICK_PERIOD_MS);
    
    return ;
}

	
uint64_t get_macid(){
    // Get MAC ADDRESS
    uint64_t _chipmacid;
    esp_efuse_mac_get_default((uint8_t*) (&_chipmacid));
	// INFO("ESP32 Chip ID = %04X",(uint16_t)(_chipmacid>>32));//print High 2 bytes
	// INFO("%08X\n",(uint32_t)_chipmacid);//print Low 4bytes.	
	return _chipmacid;
}