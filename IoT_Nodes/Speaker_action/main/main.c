#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "audio.h"
#include "thermal_sensor.h"
#include "esp_log.h"
#include "mqtt.h"

#define UART_PORT       UART_NUM_0
#define UART_BUF_SIZE   128
#define TEMP_THRESHOLD  3.0

static const char *TAG = "MAIN";

void app_main(void)
{
    audio_init();
    audio_set_volume(80);
    esp_err_t err = thermal_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize thermal sensor");
        return;
    }
    mqtt_init();
    ESP_LOGI(TAG, "System Initialized");
}