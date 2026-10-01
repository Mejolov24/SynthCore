#include <stdint.h>
#include <cmath>
#include <SQType.h>
#include "ISampleData.h"

#ifndef SynthCore_h
#define SynthCore_h

#ifndef Q_BIT_TYPE
    #define Q_BIT_TYPE int16_t
#endif
#ifndef Q_IDX_BIT_TYPE
    #define Q_IDX_BIT_TYPE uint32_t
#endif
#ifndef Q_FRAC_SIZE
    #define Q_FRAC_SIZE 12
#endif
#define Q_TYPE SQType<Q_BIT_TYPE, Q_FRAC_SIZE>
#define Q_IDX_TYPE SQType<Q_IDX_BIT_TYPE, Q_FRAC_SIZE>
#define Q_STEP_INT int32_t
#define Q_STEP_TYPE SQType<int32_t, Q_FRAC_SIZE>
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
#ifndef DEFAULT_BEND_RANGE
    #define DEFAULT_BEND_RANGE 2
#endif
#ifndef LFO_LUT_SIZE
    #define LFO_LUT_SIZE 256
#endif


#define CLAMP(value, low, high) ((value) < (low) ? (low) : ((value) > (high) ? (high) : (value)))

class SynthCore{
    private:
    static Q_TYPE lfo_lut[LFO_LUT_SIZE];
    struct LFO {
        uint32_t phase = 0;
        uint32_t step_size = 0;

        void setFrequency(float hz, uint16_t sampling_rate){step_size = static_cast<uint32_t>((hz * 4294967296.0f) / sampling_rate);}
        Q_TYPE getSample(){
            uint32_t index = phase >> 24;
            return lfo_lut[index];
        }
        void tick(){phase += step_size;}
    };

    public:
    
    struct ChannelParameters{
        Q_TYPE pitch_bend = Q_TYPE(0);
        float vibrato_frequency = 0;
        uint8_t bend_range = 2; // semitone range, -2 and +2 
        uint8_t vibrato_range = 2;
        Q_TYPE volume = Q_TYPE(1);
        bool sustain = false;

        LFO lfo;
    };
    void createVoice(const SampleData* sample_data, uint8_t note, Q_TYPE volume, uint8_t channel, bool ignore_note = false); // Creates a voice, if MAX_VOICES is reached, it will steal the oldest voice
    void releaseVoiceByNote(uint8_t note, uint8_t channel); // release the voice
    void KillAllVoices(); // useful for when voices get stuck
    void setChannelParameters(uint8_t channel, const ChannelParameters parameters);
    ChannelParameters getChannelParameters(uint8_t channel);
    void setup(uint8_t base_note, uint16_t sampling_rate, float cents_ofsset = 0); // set the base note of all samples (Reference point) and the sample rate for some effects such as vibrato
    void set_digital_gain(Q_TYPE value);
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
        Q_IDX_TYPE Q_loop_start = 0; // cached at note cration, used for fixed point operations instead of float.
        Q_IDX_TYPE Q_loop_end = 0;
        Q_IDX_TYPE Q_length = 0;
        Q_IDX_TYPE Q_index = 0; // audio index
        uint32_t vibrato_phase = 0;
        bool can_loop = true; // set to false if loop A and B are both 0.
        bool active = false;
        bool held = false; // used for sustain.
        bool ignore_note = false; // used for drums or similar

        uint8_t note = 69;
        uint8_t channel = 0;
        Q_TYPE Q_volume = Q_TYPE(1);
    };

    int32_t _channel_sum_buffer[MAX_CHANNELS];
    ChannelParameters _channels_paremeters[MAX_CHANNELS];
    Voice _Voices[MAX_VOICES];
    uint8_t _SortedVID[MAX_VOICES];// used as a LUT for voice stealing and similar.
    
    int16_t _processVoice(uint8_t VID);
    int16_t _BufferA[BUFFER_SIZE];
    int16_t _BufferB[BUFFER_SIZE];
    int16_t _sampling_rate = 0;
    Q_TYPE digital_gain = Q_TYPE(1);
    bool _buffer_index = 0;
    uint8_t _baseNote = 69; // use A4 as base note by default, change via setBaseNote()
    uint8_t _active_voice_count = 0;
    uint8_t _allocateVID();
    void _removeVoice(uint8_t VID);
    void _removeIDFromSortedVID(uint8_t VID);

    static const uint8_t _bend_range = 2;
    // fast look up table for the note A4 (Midi 69) used for step, if you want to change it use setbasenote();
    Q_STEP_TYPE _noteStepTable[128];
};
#endif