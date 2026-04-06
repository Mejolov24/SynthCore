#include <stdint.h>
#ifndef SynthCore_h
#define SynthCore_h
#ifndef MAX_VOICES
    #define MAX_VOICES 32
#endif
#ifndef MAX_CHANNELS
    #define MAX_CHANNELS 16
#endif

class SynthCore{
    public:

    struct VoiceConfig {
        const int16_t* sample;
        uint32_t sample_length;
        uint32_t _scaled_length = 0;
        uint32_t index = 0;
        uint8_t note = 0;
        uint16_t pitch_bend = 1024;
        uint8_t volume = 127;
        uint8_t channel = 0;
        bool looping = false;
        bool active = false;
    };
/* TODO : id system for voices to prevent ghosting and allow overlap.
 Actually, Who needs this? no one in their right mind plays the same note on the same channel twice. */

    void addVoice(const VoiceConfig& settings);
    void removeVoice(uint8_t channel, uint8_t note);
    void stepAudio(); // in case you need to control audio manually for hardware tricks, like pwm playback
    void updateAudioBuffer(int16_t* buffer, uint16_t size);
    VoiceConfig Voices[MAX_VOICES];
    
    private:
    int16_t channel_output[MAX_CHANNELS];
    int32_t channel_buffer[MAX_CHANNELS];
    int16_t channel_pitch_bend[MAX_CHANNELS];
    static const uint8_t bend_range = 2;
    int16_t master_mix = 0;
    int16_t _processVoice(uint8_t voice);
};
#endif