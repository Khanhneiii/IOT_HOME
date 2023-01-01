#include "rfid.h"
#include "HTTP_FB.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static bool add_mode = false;

static char *TAG = "RFID";

int add = 0;

static SemaphoreHandle_t UPD_HANDLE = NULL;


static void rc522_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    rc522_event_data_t* data = (rc522_event_data_t*) event_data;

    switch(event_id) {
        case RC522_EVENT_TAG_SCANNED: {
                rc522_tag_t* tag = (rc522_tag_t*) data->ptr;
                ESP_LOGI(TAG, "Tag scanned (sn: %" PRIu64 ")", tag->serial_number);
                if (check_id(tag->serial_number)) {
                    ESP_LOGI(TAG,"ID Found!");
                    servo_set_angle(90);
                }
                else if (add_mode) {
                    add_id(tag->serial_number);
                    ESP_LOGI(TAG,"ADD ID!");
                    add_mode = false;
                } 
            }
            break;
    }
}



static void printJsonObject(cJSON *item)
{
        char *json_string = cJSON_Print(item);
        if (json_string) 
        {
            ESP_LOGI("JSON","%s\n", json_string);
            cJSON_free(json_string);
        }
}


void update_cards(void *pvPara) {
    // ESP_LOGI(TAG,"Updating");
    while (1)
    {    
        if (xSemaphoreTake(UPD_HANDLE,portMAX_DELAY)) {
            FILE *json = fopen("/spiffs/id.txt","w");
            if (json == NULL) {
                ESP_LOGE(TAG, "Failed to open file for reading");
                return ;
            }
            char *card_buffer = malloc(20*sizeof(char));
            http_get_id(card_buffer);
            cJSON* json_cards = cJSON_Parse(card_buffer);
            printJsonObject(json_cards);
            // free(card_buffer);
            card_buffer = cJSON_PrintUnformatted(json_cards);
            ESP_LOGI(TAG,"%s",card_buffer);
            fprintf(json,card_buffer);
            fclose(json);
            free(card_buffer);
            cJSON_free(json_cards);
        }
    }
}


bool check_id(uint64_t number) {
    FILE *json = fopen("/spiffs/id.txt","r+");
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return false;
    }
    char file_buff[100];
    fgets(file_buff,sizeof(file_buff),json);

    cJSON *id_buffer = cJSON_Parse(file_buff);

    fclose(json);
    ESP_LOGI(TAG,"Print object");
    printJsonObject(id_buffer);
    cJSON *size = cJSON_GetObjectItem(id_buffer,"quantity");
    int size_val = 0;
    if (size) {
        size_val = size->valueint;
    }
    ESP_LOGI(TAG, "size: %d",size_val);

    int i;
    char idx[20];
    cJSON* val = cJSON_GetObjectItem(id_buffer,"1");
    for (i = 1;i <= size_val;i++) {
        sprintf(idx,"%d",i);
        
        ESP_LOGI("Idx","%s",idx);
        val = cJSON_GetObjectItem(id_buffer,idx);
        uint64_t valID = 0;
        if(val) {
            valID = (uint64_t)val->valuedouble;
            ESP_LOGI(TAG,"valID: %" PRIu64" ",valID);
        }
        if (number == valID) {
            return true;
        }
        // cJSON_free(val);
    }
    cJSON_free(val);
    
    // printJsonObject(id_buffer);
    return false;
}


void spiffs_config() {
    ESP_LOGI(TAG, "Initializing SPIFFS");
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    ESP_LOGI(TAG, "Opening file");
    FILE* f = fopen("/spiffs/id.txt", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root,"quantity",0);
    printJsonObject(root);
    fprintf(f,cJSON_PrintUnformatted(root));
    fclose(f);

    ESP_LOGI(TAG, "File created");

}

static void IRAM_ATTR gpio_interrupt_handler(void * pvParameter) {
    int pin = (int)pvParameter;

    switch (pin)
    {
    case ADD_BTN:
        add_mode = true;
        break;
    case UPD_BTN:
        //update function
        xSemaphoreGive(UPD_HANDLE);
        break;
    case CLS_BTN:
        servo_set_angle(0);
        break;
    }
}



void rfid_config() {
    rc522_config_t config = {
        .spi.host = VSPI_HOST,
        .spi.miso_gpio = 19,
        .spi.mosi_gpio = 23,
        .spi.sck_gpio = 18,
        .spi.sda_gpio = 5,
    };

    rc522_create(&config, &scanner);
    rc522_register_events(scanner, RC522_EVENT_ANY, rc522_handler, NULL);
    rc522_start(scanner);
    servo_set_angle(0);


    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL<<ADD_BTN) | (1ULL<<UPD_BTN) | (1ULL<<CLS_BTN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };

    gpio_config(&io_conf);
    gpio_set_intr_type(ADD_BTN,GPIO_INTR_NEGEDGE);
    gpio_set_intr_type(UPD_BTN,GPIO_INTR_NEGEDGE);
    gpio_set_intr_type(CLS_BTN,GPIO_INTR_NEGEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(ADD_BTN, gpio_interrupt_handler, (void *)ADD_BTN);
    gpio_isr_handler_add(UPD_BTN, gpio_interrupt_handler, (void *)UPD_BTN);
    gpio_isr_handler_add(CLS_BTN, gpio_interrupt_handler, (void *)CLS_BTN);
    UPD_HANDLE = xSemaphoreCreateBinary();
    xTaskCreate(update_cards,"update_cards",3096,NULL,2,NULL);
}

void add_id(uint64_t number){
    FILE *f = fopen("/spiffs/id.txt", "r+");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }
    char file_buff[100];
    fgets(file_buff,sizeof(file_buff),f);
    fclose(f);
    ESP_LOGI(TAG, "File buffer: %s",file_buff);
    cJSON *json = cJSON_Parse(file_buff);
    printJsonObject(json);
    cJSON *size = cJSON_GetObjectItem(json,"quantity");
    int size_val = 0;
    if (size) {
        size_val = size->valueint;
    }
    ESP_LOGI(TAG, "quantity: %d",size_val);
    size_val++;
    char key_id[20];

    sprintf(key_id,"%d",size_val);

    cJSON_AddNumberToObject(json,key_id,number);

    cJSON_SetNumberValue(cJSON_GetObjectItem(json,"quantity"),size_val);

    printJsonObject(json);

    f = fopen("/spiffs/id.txt", "w");

    char *data_buff = cJSON_PrintUnformatted(json);
    fprintf(f,data_buff);

    http_post_FB(CARD_URL,data_buff);

    cJSON_free(json);
    fclose(f);
}