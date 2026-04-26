#include <stdint.h>
#include <cmath>
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

#define CLAMP(value, low, high) ((value) < (low) ? (low) : ((value) > (high) ? (high) : (value)))

class SynthCore{
    public:
    
    struct ChannelParameters{
        uint16_t pitch_bend = 1024;
        uint8_t volume = 127;
        bool sustain = false;
    };
    void createVoice(const SampleData* sample_data, uint8_t note, uint8_t velocity, uint8_t channel); // return VID
    void releaseVoiceByNote(uint8_t note, uint8_t channel);
    void setChannelParameters(uint8_t channel, const ChannelParameters parameterts);
    void setBaseNote(uint8_t base_note); // used for calcuation of step (pitch change)
    void stepAudio(); // in case you need to control audio manually.
    void updateAudioBuffer();
    int16_t* getAudioBuffer(); // returns pointer to buffer A or buffer B. (returns the oposite of previous buffer)
    int16_t master_mix = 0;
    int16_t channel_output[MAX_CHANNELS];
    
    private:

    struct Voice {
        const SampleData* sample_data;
        uint32_t _scaled_loop_start = 0; // cached at note cration, used for fixed point operations instead of float.
        uint32_t _scaled_loop_end = 0;
        uint32_t _scaled_length = 0;
        uint32_t index = 0; // audio index
        bool can_loop = true; // set to false if loop A and B are both 0.
        bool active = false;
        bool held = false; // used for sustain.

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
    uint8_t _baseNote = 69; // use A4 as base note by default, change via setBaseNote()
    uint8_t _active_voice_count = 0;
    uint8_t _allocateVID();
    void _removeVoice(uint8_t VID);
    void _removeIDFromSortedVID(uint8_t VID);

    static const uint8_t _bend_range = 2;
    // fast look up table for the note A4 (Midi 69) used for step, if you want to change it use setbasenote();
    uint32_t _noteStepTable[128] = {
    8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 15, 16,             // Octave 0
    17, 18, 19, 20, 21, 22, 24, 25, 27, 28, 30, 32,          // Octave 1
    34, 36, 38, 40, 43, 45, 48, 51, 54, 57, 60, 64,          // Octave 2
    68, 72, 76, 81, 85, 91, 96, 102, 108, 114, 121, 128,     // Octave 3
    136, 144, 152, 161, 171, 181, 192, 203, 215, 228, 242, 256, // Octave 4
    271, 287, 304, 323, 342, 362, 384, 406, 431, 456, 483, 512, // Octave 5
    543, 575, 609, 645, 683, 724, 767, 813, 861, 912, 967, 1024, // Octave 6
    1085, 1149, 1218, 1290, 1367, 1448, 1534, 1625, 1722, 1824, 1933, 2048, // Octave 7
    2170, 2299, 2435, 2580, 2734, 2896, 3069, 3251, 3444, 3649, 3866, 4096, // Octave 8
    4339, 4597, 4871, 5160, 5467, 5793, 6137, 6502, 6889, 7298, 7732, 8192, // Octave 9
    8679, 9195, 9742, 10321, 10935, 11585, 12274, 13004             // Octave
};
};
#endif