#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

// Initialize DAC audio system
void audio_init(void);

// Play single tone
void audio_play_tone(uint16_t freq, uint16_t duration_ms);

// Play frequency sweep
void audio_play_sweep(uint16_t start_freq,
                      uint16_t end_freq,
                      uint16_t step,
                      uint16_t tone_duration_ms);

// Stop audio (sets output to mid-level)
void audio_stop(void);

// Set volume (0 - 100)
void audio_set_volume(uint8_t volume_percent);

void play_audio(const unsigned char *data, unsigned int len);

// Demo functions for specific sound patterns
void audio_play_alarm(void);
void audio_play_siren(void);
void audio_play_chirp(void);
void audio_play_evening_tone(void);

#endif