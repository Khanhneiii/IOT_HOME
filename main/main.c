#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <string.h>
#include "rfid.h"
#include "mqtt_ctrl.h"
#include "wifi.h"
#include "dht11.h"
#include "alarm.h"
#include  "WTV020SD16P.h"

#define SERVO_PIN 4
#define GAS_PIN 16
#define PIR_PIN 22
#define PIR_STATE 27
#define MAIN "ESP"



void dht_task(void *pvParameter) {
  while(1) {
        ESP_LOGI("DHT","Temp: %d", DHT11_read().temperature);

        ESP_LOGI("DHT","Hemi: %d", DHT11_read().humidity);

        vTaskDelay(4000/portTICK_PERIOD_MS);
    }
}

void app_main(void){

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(MAIN, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    // DHT11_init(4);

    InitAlarm(GAS_PIN,PIR_PIN,PIR_STATE);

    init_servo(SERVO_PIN);

    spiffs_config();

    rfid_config();      

    mqtt_app_start();

    xTaskCreate(AlarmTask,"AlarmTask",2048,NULL,1,NULL);

    // xTaskCreate(dht11_read_task,"dht11_read_task",3096,NULL,2,NULL);
    // xTaskCreate(dht_task,"dht_task",2048,NULL,2,NULL);
}