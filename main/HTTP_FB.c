#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "driver/gpio.h"
#include "esp_tls.h"
#include "cJSON.h"
#include <string.h>

#include "HTTP_FB.h"

SemaphoreHandle_t GetSignal = NULL;

 char *get_buffer;

static const char* TAG = "HTTP";

extern const uint8_t certificate_pem_start[] asm("_binary_certificate_pem_start");
extern const uint8_t certificate_pem_end[]   asm("_binary_certificate_pem_end");

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer; // Buffer to store response of http request from event handler
    static int output_len;      // Stores number of bytes read
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, data=%s", (char *)evt->data);
        /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            // If user_data buffer is configured, copy the response into the buffer
            if (evt->user_data)
            {
                memcpy(evt->user_data + output_len, evt->data, evt->data_len);
            }
            // else
            {
                // ESP_LOGI(TAG, "Output: %s",output_buffer);
                if (output_buffer == NULL)
                {
                    output_buffer = (char *)malloc(esp_http_client_get_content_length(evt->client));
                    output_len = 0;
                    if (output_buffer == NULL)
                    {
                        ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                        return ESP_FAIL;
                    }
                }
                memcpy(output_buffer + output_len, evt->data, evt->data_len);
            }
            output_len += evt->data_len;
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH"); 
        ESP_LOGI(TAG, "Output complete: %d",output_len);
        free(get_buffer);
        get_buffer = NULL;
        // if (output_buffer == NULL) ESP_LOGI(TAG, "Error output ");
        if (output_buffer != NULL)
        {
            get_buffer = malloc(strlen(output_buffer));
            ESP_LOGI(TAG, "Output complete: %d",strlen(output_buffer));
            memcpy(get_buffer,output_buffer, strlen(output_buffer));
            if (get_buffer && GetSignal) {
                ESP_LOGI(TAG, "Get complete ");
                xSemaphoreGive(GetSignal);
            }
            // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
            // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}

void create_Semaphore() {
    GetSignal = xSemaphoreCreateBinary();
}

void http_get_id(char* id_buff) {
    if (GetSignal == NULL) create_Semaphore();
    char local_response_buffer[20] ={0};

    esp_http_client_config_t config = {
        .url = "https://dht-home-b0439-default-rtdb.asia-southeast1.firebasedatabase.app/cards.json",
        .event_handler = http_event_handler,
        .user_data = local_response_buffer, 
        .cert_pem = (const char *)certificate_pem_start,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_GET);

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %" PRId64,  
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
        while (!xSemaphoreTake(GetSignal,portMAX_DELAY));
        memcpy(id_buff,get_buffer,strlen(get_buffer));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    vTaskDelay(500/portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "HTTP client ok");
    esp_http_client_cleanup(client);
    vTaskDelay(500/portTICK_PERIOD_MS);
}

void http_post_FB(const char* URL, const char* data_buff) {
    char local_response_buffer[20] ={0};

    esp_http_client_config_t config = {
        .url = URL,
        .event_handler = http_event_handler,
        .user_data = local_response_buffer, 
        .cert_pem = (const char *)certificate_pem_start,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_PUT);
    esp_http_client_set_header(client, "Content-Type", "text/plain");
    esp_http_client_set_post_field(client, data_buff, strlen(data_buff));
    esp_http_client_open(client,strlen(data_buff));
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP PUT Status = %d, content_length = %" PRId64,  
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP PUT request failed: %s", esp_err_to_name(err));
    }
    ESP_LOGI(TAG, "HTTP client ok");
    esp_http_client_cleanup(client);
}

