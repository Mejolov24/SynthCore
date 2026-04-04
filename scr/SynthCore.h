#ifndef MAX_VOICES
    #define MAX_VOICES 32  // Default value
#endif

class SynthCore{
    public:

    struct VoiceConfig {
        const int16_t* sample;
        uint32_t sample_length;
        uint32_t index;
        uint8_t note;
        uint16_t pitch_bend;
        uint8_t volume;
        uint8_t channel;
        bool looping;
        bool active;
    };
/* TODO : id system for voices to prevent ghosting and allow overlap.
 Actually, Who needs this? no one in their right mind plays the same note on the same channel twice. */

    void addVoice(const int16_t* sample,uint32_t sample_length, uint8_t note, uint16_t pitch_bend, uint8_t volume,uint8_t channel , bool looping);
    void removeVoice(uint8_t channel, uint8_t note);
    void stepAudio(); // in case you need to control audio manually for hardware tricks, like pwm playback
    void getAudioBuffer(int16_t* arr, uint8_t size);
    VoiceConfig Voices[MAX_VOICES];
    
    private:
    uint8_t channel_instrument[16];
    int16_t channel_output[17]; // 17 is master mix
    float channel_pitch_bend[16];
    bool active_channels[16];
    static const uint8_t bend_range = 2;
};