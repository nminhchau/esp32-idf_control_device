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
#include <limits>


#include "config.h"
#include "wifi.h"
//socket
#define HOST_IP_ADDR "192.168.1.67"
#define PORT 8080
//gpio 



static const char *TAG = "MAIN";
static const char *payload = "Hello Server";



void btn_init(void)
{
  gpio_set_direction(DOOR_BUTTON, GPIO_MODE_INPUT);
  gpio_set_pull_mode(DOOR_BUTTON, GPIO_PULLUP_ONLY);
  gpio_set_direction(EMERGENCY_BUTTON, GPIO_MODE_INPUT);
  gpio_set_pull_mode(EMERGENCY_BUTTON, GPIO_PULLUP_ONLY);


  gpio_pad_select_gpio(CLOSE_LED);  
  gpio_set_direction(CLOSE_LED, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(OPEN_LED);  
  gpio_set_direction(OPEN_LED, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(DOOR);
  gpio_set_direction(DOOR, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(BUZZER);
  gpio_set_direction(BUZZER, GPIO_MODE_OUTPUT);
}

void state_init()
{
  state.door = false;
  state.emergency = false;
}

void open_door()
{
  gpio_set_level(CLOSE_LED, OFF);
  gpio_set_level(OPEN_LED, ON);
  gpio_set_level(DOOR, ON);
  state.door = true;
  ESP_LOGI("DOOR is OPEN! Door_state","%d", state.door);
  ESP_LOGI("DOOR_BUTTON", "%d", gpio_get_level(DOOR_BUTTON));
  vTaskDelay(3000/portTICK_PERIOD_MS);
}

void close_door()
{
  gpio_set_level(CLOSE_LED, ON);
  gpio_set_level(OPEN_LED, OFF);
  gpio_set_level(DOOR, OFF);
  gpio_set_level(BUZZER, 0);
  ESP_LOGI("DOOR is CLOSE!", "DOOR_BUTTON: %d", gpio_get_level(DOOR_BUTTON));
  vTaskDelay(100/portTICK_PERIOD_MS);
}

void control(void *arg)
{
  while(1)
  {
    while(state.emergency == true)
    {
      gpio_set_level(CLOSE_LED, OFF);
      gpio_set_level(OPEN_LED, ON);
      gpio_set_level(DOOR, ON);
      gpio_set_level(BUZZER, 1);
      state.door = true;
      ESP_LOGI("DOOR is OPEN with emergency mode! Door_state","%d", state.door);
      ESP_LOGI("EMERGENCY_BUTTON", "%d", gpio_get_level(EMERGENCY_BUTTON));
      vTaskDelay(100/portTICK_PERIOD_MS);
    }

    if(state.door == true)
    {
      open_door();
    }
    else if (state.door == false)
    {
      close_door();
    }
    
  }  
}


void setState(void *arg)
{
  while(1)
  {
    if(gpio_get_level(EMERGENCY_BUTTON) == 0)
    {
      if(state.emergency == true)
      {
        state.emergency = false;
      }
      else
      {
        state.emergency = true;
      }
      ESP_LOGI("Emergency state is", "%d", state.emergency);
    }
    vTaskDelay(400/portTICK_PERIOD_MS);
  }  
}



void socket_task(void *arg)
{
  char rx_buffer[128];
  char host_ip[] = HOST_IP_ADDR;
  int addr_family = 0;
  int ip_protocol = 0;

  while (1)
  {
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(host_ip);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
    if (sock < 0)
    {
      ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
      break;
    }
    ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, PORT);

    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
    if (err != 0)
    {
      ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
      break;
    }
    ESP_LOGI(TAG, "Successfully connected");

    while (1)
    {
      ESP_LOGE("RX_BUFFER", "%s", rx_buffer);
      int err = send(sock, payload, strlen(payload), 0);
      if (err < 0)
      {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        break;
      }

      int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
      // Error occurred during receiving
      if (len < 0)
      {
        ESP_LOGE(TAG, "recv failed: errno %d", errno);
        break;
      }
      // Data received
      else
      {
        rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
        ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
        ESP_LOGI(TAG, "%s", rx_buffer);
        if (len == 8)
        {
          open_door();
        }
        else
        {
          close_door();
        }    
      }
    }

    if (sock != -1)
    {
      ESP_LOGE(TAG, "Shutting down socket and restarting...");
      shutdown(sock, 0);
      close(sock);
    }
  }
  vTaskDelete(NULL);
}
extern "C"  void app_main()
{
  ESP_LOGI(TAG, "[APP] Startup..");
  ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
  ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

  /* Initialize NVS partition */
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_init());
  }
  param.WN="1B KMT";
  param.WP="a1234567";

  btn_init();
  state_init();

  //wifi
  if(wifi_init_sta())
  {
    xTaskCreate(&socket_task, "socket_task", 9216, NULL, 3, NULL);
    xTaskCreate(&setState, "set_state_task", 1024*2, NULL, 2, NULL);
    xTaskCreate(&control, "control_task", 1024*2, NULL, 2, NULL);
  }
  else
  {

  }
}