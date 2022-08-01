#ifndef __CONFIG__
#define __CONFIG__
#include "stdint.h"
#include "stdbool.h"
#include "esp_log.h"


#define DOOR_BUTTON             GPIO_NUM_19
#define EMERGENCY_BUTTON        GPIO_NUM_18

#define CLOSE_LED               GPIO_NUM_27
#define OPEN_LED                GPIO_NUM_26

#define DOOR                    GPIO_NUM_25
#define BUZZER                  GPIO_NUM_21

#define ON            0
#define OFF           1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    char* WN;
    char* WP;
} param_t; 

param_t param;

typedef struct
{
    bool door;
    bool emergency;
}state_t;

state_t state;

#ifdef __cplusplus
}
#endif

#endif
