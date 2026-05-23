#include "audio.h"
#include "driver/dac_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"
#include <math.h>

#define SAMPLE_RATE 40000
#define PI 3.14159265

static dac_oneshot_handle_t dac_handle;
static uint8_t volume_scale = 100;

void audio_init(void)
{
    dac_oneshot_config_t config = {
        .chan_id = DAC_CHAN_0  // GPIO25
    };

    dac_oneshot_new_channel(&config, &dac_handle);
}

void audio_set_volume(uint8_t volume_percent)
{
    if (volume_percent > 100)
        volume_percent = 100;

    volume_scale = volume_percent;
}

void audio_stop(void)
{
    dac_oneshot_output_voltage(dac_handle, 128);
}

void audio_play_tone(uint16_t freq, uint16_t duration_ms)
{
    uint32_t total_samples = (SAMPLE_RATE * duration_ms) / 1000;
    float amplitude = 120.0 * (volume_scale / 100.0);

    for (uint32_t i = 0; i < total_samples; i++)
    {
        float theta = 2.0 * PI * freq * i / SAMPLE_RATE;
        uint8_t value = (uint8_t)((sin(theta) * amplitude) + 128);

        dac_oneshot_output_voltage(dac_handle, value);

        esp_rom_delay_us(1000000 / SAMPLE_RATE);
    }

    audio_stop();
}

void audio_play_sweep(uint16_t start_freq,
                      uint16_t end_freq,
                      uint16_t step,
                      uint16_t tone_duration_ms)
{
    for (uint16_t f = start_freq; f <= end_freq; f += step)
    {
        audio_play_tone(f, tone_duration_ms);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void play_audio(const unsigned char *data,
                unsigned int len)
{
    const int WAV_HEADER = 44;

    if (len <= WAV_HEADER)
        return;

    for (unsigned int i = WAV_HEADER;
         i < len;
         i++)
    {
        dac_oneshot_output_voltage(
                dac_handle,
                data[i]);

        esp_rom_delay_us(125); // 8 kHz
    }

    audio_stop();
}

// =====================================================
// Demo function to play a simple melody (for testing)
// =====================================================

void audio_play_alarm(void)
{
    audio_play_sweep(1000,1000,200,80);
    vTaskDelay(pdMS_TO_TICKS(100));

    audio_play_sweep(1000,1000,200,80);
    vTaskDelay(pdMS_TO_TICKS(100));

    audio_play_sweep(1000,1000,200,80);
}

void audio_play_siren(void)
{
    for(int i=0;i<2;i++)
    {
        audio_play_sweep(
            1000,
            4000,
            200,
            80);

        audio_play_sweep(
            4000,
            1000,
            200,
            80);
    }
}

void audio_play_chirp(void)
{
    audio_play_sweep(
        3000,
        6000,
        100,
        80);

    vTaskDelay(pdMS_TO_TICKS(50));

    audio_play_sweep(
        3500,
        6500,
        100,
        80);
}

void audio_play_evening_tone(void)
{
    audio_play_sweep(
        500,
        1000,
        300,
        80);

    vTaskDelay(pdMS_TO_TICKS(100));

    audio_play_sweep(
        1000,
        1500,
        300,
        80);
}