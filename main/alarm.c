#include "alarm.h"
#include "esp_log.h"
#include "rfid.h"
#include "driver/gpio.h"

static const char* TAG = "ALARM";

static int GAS_PIN,PIR_PIN,PIR_STATE;

    bool PirState = false;

static void IRAM_ATTR isr_handler(void *arg) {
    PirState = !PirState;
}

void InitAlarm(int gas,int pir,int state) {
    GAS_PIN = gas;
    PIR_PIN = pir;
    PIR_STATE = state;
    gpio_config_t io = {
        .pin_bit_mask = (1ULL<<GAS_PIN) | (1ULL<<PIR_PIN) ,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 0,                           
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config_t isr_io = {
        .pin_bit_mask = (1ULL<<PIR_STATE),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 0,                           
        .pull_down_en = 1,
        .intr_type = GPIO_INTR_POSEDGE
    };
    gpio_config(&io);
    gpio_config(&isr_io);
    gpio_set_intr_type(PIR_STATE,GPIO_INTR_POSEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIR_STATE, isr_handler, (void *)ADD_BTN);
}

void AlarmTask(void *pvParameters) {
    while (1) {
        if (gpio_get_level(GAS_PIN) == 0){
            ESP_LOGI(TAG,"GAS!");
            servo_set_angle(90);
        }
        if (gpio_get_level(PIR_PIN) == 1 && PirState) {
            ESP_LOGI(TAG,"Motion!");
        }
        vTaskDelay(500/portTICK_PERIOD_MS);
    }
}