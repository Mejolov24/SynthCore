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

    struct LFO {
        uint32_t phase = 0;
        uint32_t step_size = 0;

        static constexpr int16_t lut[LFO_LUT_SIZE] = {
            0, 25, 50, 75, 100, 125, 150, 175, 200, 225, 249, 274, 298, 322, 346, 370,
            392, 415, 437, 459, 480, 501, 522, 542, 562, 581, 600, 618, 636, 654, 671, 688,
            706, 724, 741, 758, 775, 791, 807, 822, 837, 851, 865, 878, 891, 903, 914, 925,
            936, 946, 955, 964, 972, 980, 987, 993, 999, 1004, 1009, 1013, 1016, 1019, 1021, 1023,
            1024, 1023, 1021, 1019, 1016, 1013, 1009, 1004, 999, 993, 987, 980, 972, 964, 955, 946,
            936, 925, 914, 903, 891, 878, 865, 851, 837, 822, 807, 791, 775, 758, 741, 724,
            706, 688, 671, 654, 636, 618, 600, 581, 562, 542, 522, 501, 480, 459, 437, 415,
            392, 370, 346, 322, 298, 274, 249, 225, 200, 175, 150, 125, 100, 75, 50, 25,
            0, -25, -50, -75, -100, -125, -150, -175, -200, -225, -249, -274, -298, -322, -346, -370,
            -392, -415, -437, -459, -480, -501, -522, -542, -562, -581, -600, -618, -636, -654, -671, -688,
            -706, -724, -741, -758, -775, -791, -807, -822, -837, -851, -865, -878, -891, -903, -914, -925,
            -936, -946, -955, -964, -972, -980, -987, -993, -999, -1004, -1009, -1013, -1016, -1019, -1021, -1023,
            -1024, -1023, -1021, -1019, -1016, -1013, -1009, -1004, -999, -993, -987, -980, -972, -964, -955, -946,
            -936, -925, -914, -903, -891, -878, -865, -851, -837, -822, -807, -791, -775, -758, -741, -724,
            -706, -688, -671, -654, -636, -618, -600, -581, -562, -542, -522, -501, -480, -459, -437, -415,
            -392, -370, -346, -322, -298, -274, -249, -225, -200, -175, -150, -125, -100, -75, -50, -25
        };

        void setFrequency(float hz, uint16_t sampling_rate){step_size = static_cast<uint32_t>((hz * LFO_LUT_SIZE * 4294967296.0f) / sampling_rate);}
        int16_t getSample(){
            uint32_t index = phase >> 24;
            return lut[index];
        }
        void tick(){phase += step_size;}
    };

    public:
    
    struct ChannelParameters{
        uint16_t pitch_bend = 1024;
        float vibrato_frequency = 0;
        uint8_t bend_range = 2; // semitone range, -2 and +2 
        uint8_t vibrato_range = 2;
        uint8_t volume = 127;
        bool sustain = false;

        LFO lfo;
    };
    void createVoice(const SampleData* sample_data, uint8_t note, uint8_t velocity, uint8_t channel, bool ignore_note = false); // Creates a voice, if MAX_VOICES is reached, it will steal the oldest voice
    void releaseVoiceByNote(uint8_t note, uint8_t channel); // release the voice
    void KillAllVoices(); // useful for when voices get stuck
    void setChannelParameters(uint8_t channel, const ChannelParameters parameters);
    ChannelParameters getChannelParameters(uint8_t channel);
    void setup(uint8_t base_note, uint16_t sampling_rate); // set the base note of all samples (Reference point) and the sample rate for some effects such as vibrato
    void set_digital_gain(uint16_t value);
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
    int16_t _sampling_rate = 0;
    uint16_t digital_gain = 0;
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