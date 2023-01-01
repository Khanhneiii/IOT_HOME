#ifndef _MQTT_CTRL_H

#define _MQTT_CTRL_H

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "mqtt_client.h"

#define TAG "MQTT"

#define LED_PIN 2

#define BROKER "mqtt://io.adafruit.com:1883"
#define USER "NhaKhanh"
#define AIO_KEY "aio_dZsq080TvL3vnaL923FCxPxW2ynX"

#define TOPIC "NhaKhanh/feeds/led1"

void subcribe_topic(esp_mqtt_client_handle_t);

void mqtt_app_start();

#endif