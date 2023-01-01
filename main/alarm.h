#ifndef _ALARM_H
#define _ALARM_H

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_event.h"
#include "driver/gpio.h"

void InitAlarm(int,int,int);

void AlarmTask(void *pvParameters);

#endif