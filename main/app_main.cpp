#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <iostream>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "config.h"
#include "wifi.h"
//socket
#define HOST_IP_ADDR "192.168.1.67"
#define PORT 8000
//gpio 
#define GPIO_OUTPUT_LED1    LED1
#define GPIO_OUTPUT_LED2    LED2
// #define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_LED1) | (1ULL<<GPIO_OUTPUT_LED2))

#define GPIO_INPUT_BUTTON1     BUTTON1
#define GPIO_INPUT_BUTTON2     BUTTON2
// #define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_BUTTON1) | (1ULL<<GPIO_INPUT_BUTTON2))

static const char *TAG = "MAIN";
static const char *payload = "Hello Server";



void btn_init(void){
    gpio_config_t btn ;
    memset(&btn, 0, sizeof(gpio_config_t));

	btn.mode = GPIO_MODE_INPUT;
    btn.pin_bit_mask = (1ULL << BUTTON1);
    btn.pull_up_en = GPIO_PULLUP_ENABLE;
    btn.intr_type = GPIO_INTR_ANYEDGE;
    gpio_config(&btn);

    btn.mode = GPIO_MODE_INPUT;
    btn.pin_bit_mask = (1ULL << BUTTON2);
    btn.pull_up_en = GPIO_PULLUP_ENABLE;
    btn.intr_type = GPIO_INTR_ANYEDGE;
    gpio_config(&btn);

    gpio_pad_select_gpio(LED1);  
    gpio_pad_select_gpio(LED2);  
    gpio_pad_select_gpio(DOOR);  
    gpio_pad_select_gpio(BUZZER);  
    gpio_set_direction(LED1, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED2, GPIO_MODE_OUTPUT);
    gpio_set_direction(DOOR, GPIO_MODE_OUTPUT);
    gpio_set_direction(BUZZER, GPIO_MODE_OUTPUT);
}

void open_door(){
    ESP_LOGI(TAG, "Open door function was called!");
    if (device.door_state){
        gpio_set_level(LED1, OFF);
        gpio_set_level(LED2, ON);
        gpio_set_level(DOOR, ON);
        ESP_LOGI(TAG, "door_state = TRUE");
    }
    else{
        gpio_set_level(LED1, ON);
        gpio_set_level(LED2, OFF);
        gpio_set_level(DOOR, OFF);
        ESP_LOGI(TAG, "door_state = FALSE");
    }
}

void state_init(){
    device.door_state = false;
    device.emergency_state = false;
}

void open_task(void *arg){
    btn_init();
    while (1){
        while (device.emergency_state){
            // ESP_LOGI(TAG, "emergency state: %s", device.emergency_state ? "true" : "false");
            device.door_state = true;
            open_door();
        }
        if (!gpio_get_level(BUTTON1)){
            device.door_state = true;
            open_door();
            vTaskDelay(3000/portTICK_PERIOD_MS);
        }
        else{
            device.door_state = false;
            open_door();
        }
        vTaskDelay(50/portTICK_PERIOD_MS);
    }
}

void emercency_task(void *arg){
    btn_init();
    while(1){
        if (!gpio_get_level(BUTTON2)){
            device.emergency_state = true;
            ESP_LOGI(TAG, "emer: %s", device.emergency_state ? "T" : "F");

            gpio_set_level(BUZZER, ON);
        }
        else{
            gpio_set_level(BUZZER, OFF);
        }
        vTaskDelay(500/portTICK_PERIOD_MS);
    }
}



void socket_task(void *arg){
    char rx_buffer[128];
    char host_ip[] = HOST_IP_ADDR;
    int addr_family = 0;
    int ip_protocol = 0;

    while (1){
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(host_ip);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0){
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, PORT);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Successfully connected");

        while (1){
            int err = send(sock, payload, strlen(payload), 0);
            if (err < 0){
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                break;
            }

            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error occurred during receiving
            if (len < 0){
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            }
            // Data received
            else{
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
                ESP_LOGI(TAG, "%s", rx_buffer);
                if (strcmp(std::to_string(rx_buffer), "1"))
                {
                    device.door_state = true;
                    open_door();
                }
                else
                {
                    device.door_state = false;
                    open_door();
                }        
            }
            vTaskDelay(1000/portTICK_PERIOD_MS);
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}
extern "C"  void app_main(){
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    /* Initialize NVS partition */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    param.WN="1B KMT";
    param.WP="a1234567";
    state_init();
    ESP_LOGI(TAG, "emer: %s", device.emergency_state ? "T" : "F");

    //wifi
    if(wifi_init_sta()){
        //socket_task task
        xTaskCreate(&socket_task, "socket_task", 9216, NULL, 9, NULL);
        //open door task
        xTaskCreate(&open_task, "open_task", 1024*2, NULL, 7, NULL);
        //emergency task
        xTaskCreate(&emercency_task, "emergency_task", 1024*2, NULL, 7, NULL);
    }
    else{

    }
}