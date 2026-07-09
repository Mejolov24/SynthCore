#include <stdint.h>
#include "ISampleData.h"
#ifndef SynthCore_h
#define SynthCore_h
#ifndef MAX_VOICES
    #define MAX_VOICES 32
#endif
#ifndef MAX_CHANNELS
    #define MAX_CHANNELS 16
#endif
#ifndef BUFFER_SIZE
    #define BUFFER_SIZE 256
#endif
#ifndef DEFAULT_BASE_NOTE
    #define DEFAULT_BASE_NOTE 60
#endif
#ifndef DEFAULT_SAMPLING_RATE
    #define DEFAULT_SAMPLING_RATE 22050
#endif

#define CLAMP(value, low, high) ((value) < (low) ? (low) : ((value) > (high) ? (high) : (value)))

class SynthCore{
    private:
    static const float Ln2 = 0.6931471805599453f;

    static constexpr float Exp(float x){
        float sum = 1.0f;
        float fraction = 1.0f;
        for(int i = 1; i < 16; ++i){fraction *= x / static_cast<float>(i); sum += fraction;}
        return sum;
    }
    static constexpr float Pow2(float x){return Exp(x * Ln2);}
    static constexpr std::array<uint32_t, 128> generateNoteStepTable(uint8_t baseNote){
        std::aray<uint32_t, 128> table{};
        for (int i = 0; i < 128; i++){
            float ratio = Pow2((static_cast<float>(i) - static_cast<float>(baseNote)) / 12.0f )
            table[i] = static_cast<uint32_t>(ratio * 1024.0f);
        }
        return table;
    }

    public:
    
    struct ChannelParameters{
        uint16_t pitch_bend = 1024;
        uint16_t vibrato = 1024;
        uint8_t pitch_range = 2; // semitone range, -2 and +2 
        uint8_t volume = 127;
        bool sustain = false;
    };
    void createVoice(const SampleData* sample_data, uint8_t note, uint8_t velocity, uint8_t channel, bool ignore_note = false); // Creates a voice, if MAX_VOICES is reached, it will steal the oldest voice
    void releaseVoiceByNote(uint8_t note, uint8_t channel); // release the voice
    void KillAllVoices(); // useful for when voices get stuck
    void setChannelParameters(uint8_t channel, const ChannelParameters parameters);
    ChannelParameters getChannelParameters(uint8_t channel);
    void setup(uint8_t base_note, uint16_t sampling_rate); // set the base note of all samples (Reference point) and the sample rate for some effects such as vibrato
    void stepAudio(); // in case you need to control audio manually. processes one engine tick
    void updateAudioBuffer(); // processes the voices and generates buffer
    int16_t* getAudioBuffer(); // returns pointer to buffer A or buffer B. (returns the opposite of previous buffer)
    int16_t master_mix = 0; // final stage of processing, has the value of all voices mixed together.
    int16_t channel_output[MAX_CHANNELS]; // separated channel output for voices, useful for plotting.
    
    // beacuse of the system used for handling voice removal and stealing, we must send voices with different notes when using drums
    // otherwise the engine gets confused and flags all notes as the same note, to prevent that, set the parameter ignore_note to true.

    private:

    struct Voice {
        const SampleData* sample_data;
        uint32_t _scaled_loop_start = 0; // cached at note cration, used for fixed point operations instead of float.
        uint32_t _scaled_loop_end = 0;
        uint32_t _scaled_length = 0;
        uint32_t index = 0; // audio index
        uint32_t vibrato_phase = 0;
        bool can_loop = true; // set to false if loop A and B are both 0.
        bool active = false;
        bool held = false; // used for sustain.
        bool ignore_note = false; // used for drums or similar

        uint8_t note = 69;
        uint8_t channel = 0;
        uint8_t velocity = 127;
    };

    int32_t _channel_sum_buffer[MAX_CHANNELS];
    ChannelParameters _channels_paremeters[MAX_CHANNELS];
    Voice _Voices[MAX_VOICES];
    uint8_t _SortedVID[MAX_VOICES];// used as a LUT for voice stealing and similar.

    
    int16_t _processVoice(uint8_t VID);
    int16_t _BufferA[BUFFER_SIZE];
    int16_t _BufferB[BUFFER_SIZE];
    bool _buffer_index = 0;

    int16_t _sampling_rate = DEFAULT_SAMPLING_RATE;
    uint8_t _baseNote = DEFAULT_BASE_NOTE;
    uint8_t _active_voice_count = 0;
    uint8_t _allocateVID();
    void _removeVoice(uint8_t VID);
    void _removeIDFromSortedVID(uint8_t VID);

    const uint8_t _bend_range = 2;
    std::array<uint32_t,128> _noteStepTable = generateNoteStepTable(_baseNote);
};
#endif