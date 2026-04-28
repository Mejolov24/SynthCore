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
    ChannelParameters getChannelParameters(uint8_t channel);
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
        13, 14, 14, 15, 16, 17, 18, 19, 20, 22, 23, 24,           // Octave 0
        26, 27, 29, 31, 32, 34, 36, 38, 41, 43, 46, 48,           // Octave 1
        51, 54, 58, 61, 65, 69, 73, 77, 82, 87, 92, 97,           // Octave 2
        103, 109, 115, 122, 129, 137, 145, 154, 163, 173, 183, 194, // Octave 3
        206, 218, 231, 245, 259, 275, 291, 308, 327, 346, 367, 388, // Octave 4
        412, 436, 462, 490, 519, 550, 582, 617, 654, 693, 734, 777, // Octave 5
        823, 872, 924, 979, 1037, 1099, 1164, 1233, 1307, 1384, 1467, 1554, // Octave 6
        1646, 1744, 1848, 1958, 2074, 2198, 2328, 2467, 2613, 2769, 2933, 3108, // Octave 7
        3293, 3488, 3696, 3915, 4148, 4395, 4656, 4933, 5226, 5537, 5866, 6215, // Octave 8
        6585, 6976, 7391, 7830, 8296, 8789, 9312, 9865, 10452, 11073, 11732, 12429, // Octave 9
        13168, 13951, 14781, 15660, 16591, 17578, 18623, 19730 // Octave 10
    };
};
#endif