#ifndef __CONFIG__
#define __CONFIG__
#include "stdint.h"
#include "stdbool.h"
#include "esp_log.h"


#define BUTTON1     GPIO_NUM_34
#define BUTTON2     GPIO_NUM_14

#define LED1        GPIO_NUM_19
#define LED2        GPIO_NUM_18

#define DOOR        GPIO_NUM_23
#define BUZZER      GPIO_NUM_22

#define ON            0
#define OFF           1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* WN;
    char* WP;
} param_t; 

param_t param;

typedef struct
{
    bool door_state;
    bool emergency_state;
}device_t;

device_t device;

#ifdef __cplusplus
}
#endif

#endif