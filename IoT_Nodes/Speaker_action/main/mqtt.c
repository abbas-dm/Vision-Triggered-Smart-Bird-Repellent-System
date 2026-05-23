#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "mqtt_client.h"
#include "esp_crt_bundle.h"

#include "driver/dac.h"

#include "mqtt.h"
#include "thermal_sensor.h"
#include "audio.h"

#include "cJSON.h"

#include "morning.h"
#include "afternoon.h"
#include "evening.h"
#include "night.h"

#define WIFI_SSID      "ESP_Test"
#define WIFI_PASS      "ESP@22970701"

#define MQTT_TOPIC     "bird/control"

#define TEMP_THRESHOLD 3.0

static const char *TAG = "MQTT_APP";

static esp_mqtt_client_handle_t mqtt_client = NULL;

static void mqtt_start(void);

/* ===================================================== */
/* JSON PROCESSING */
/* ===================================================== */

static void process_bird_detection(const char *payload)
{
    cJSON *root = cJSON_Parse(payload);

    if (root == NULL)
    {
        ESP_LOGE(TAG, "JSON parse failed");
        return;
    }

    cJSON *event = cJSON_GetObjectItem(root, "event");
    cJSON *time_pattern = cJSON_GetObjectItem(root, "time_pattern");
    cJSON *count = cJSON_GetObjectItem(root, "count");
    cJSON *timestamp = cJSON_GetObjectItem(root, "timestamp");

    if (cJSON_IsString(event) && strcmp(event->valuestring, "bird_detected") == 0)
    {
        ESP_LOGI(TAG, "Bird detected");

        if (cJSON_IsString(time_pattern))
        {
            ESP_LOGI(TAG, "Time Slot: %s", time_pattern->valuestring);

            if (strcmp(time_pattern->valuestring, "morning") == 0)
            {
                ESP_LOGI(TAG, "Morning deterrence");
                // audio_play_sweep(3000, 8000, 500, 80);
                play_audio(
                    morning_wav,
                    morning_wav_len
                );
            }

            else if (strcmp(time_pattern->valuestring, "afternoon") == 0)
            {
                ESP_LOGI(TAG, "Afternoon deterrence");
                // audio_play_sweep(5000, 12000, 600, 80);
                play_audio(
                    afternoon_wav,
                    afternoon_wav_len
                );
            }

            else if (strcmp(time_pattern->valuestring, "evening") == 0)
            {
                ESP_LOGI(TAG, "Evening deterrence");
                // audio_play_sweep(5000, 12000, 600, 80);
                play_audio(
                    evening_wav,
                    evening_wav_len
                );
            }

            else if (strcmp(time_pattern->valuestring, "night") == 0)
            {
                ESP_LOGI(TAG, "Night deterrence");
                // audio_play_sweep(8000, 15000, 700, 80);
                play_audio(
                    night_wav,
                    night_wav_len
                );
            }
        }

        if (cJSON_IsNumber(count))
        {
            ESP_LOGI(TAG, "Bird Count: %d", count->valueint);
        }

        if (cJSON_IsString(timestamp))
        {
            ESP_LOGI(TAG, "Timestamp: %s", timestamp->valuestring);
        }
    }

    cJSON_Delete(root);
}

/* ===================================================== */
/* WIFI */
/* ===================================================== */

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
        ESP_LOGI(TAG, "WiFi connecting...");
    }

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "WiFi disconnected");
        esp_wifi_connect();
    }

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        mqtt_start();
    }
}

static void wifi_init(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL);

    esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL);

    wifi_config_t wifi_config =
    {
        .sta =
        {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

/* ===================================================== */
/* MQTT EVENT */
/* ===================================================== */

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch (event->event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected");
            esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC, 0);
            break;

        case MQTT_EVENT_DATA:
        {
            ESP_LOGI(TAG, "Topic: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "Payload: %.*s", event->data_len, event->data);

            char json_buffer[512];
            memset(json_buffer, 0, sizeof(json_buffer));

            int len = event->data_len;

            if (len >= sizeof(json_buffer))
            {
                len = sizeof(json_buffer) - 1;
            }

            memcpy(json_buffer, event->data, len);
            json_buffer[len] = '\0';
            process_bird_detection(json_buffer);

            break;
        }

        default:
            break;
    }
}

/* ===================================================== */
/* MQTT START */
/* ===================================================== */

static void mqtt_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg =
    {
        .broker.address.uri = "mqtts://ada17b2cb97e4a3e80fc342ed5871861.s1.eu.hivemq.cloud:8883",
        .credentials.username = "Capstone_project",
        .credentials.authentication.password = "Capstone@esp32",
        .broker.verification.crt_bundle_attach = esp_crt_bundle_attach,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

/* ===================================================== */
/* INIT */
/* ===================================================== */

void mqtt_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init();

    vTaskDelay(pdMS_TO_TICKS(2000));
}