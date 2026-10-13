# SynthCore
## Overview
SynthCore is a freestanding C++ audio engine designed for real-time embedded audio systems with fixed point calculations

## Technical aspects
* Free standing C++
    - Only depends on <stdint.h> and <cmath>
* Stack only, no heap memory.
* Configurable at compile time.
    ```cpp
    #define MAX_VOICES 32
    #define MAX_CHANNELS 16
    #define BUFFER_SIZE 256
    #define Q_BIT_TYPE int16_t // Data type for Fixed point calculations
    #define Q_FRAC_SIZE 12 // 12 bits of precision (4 bits for integer, 12 bits for decimals) *recomended on most systems*
    ```
* Fixed-point arithmetic
    - Uses [SQTYPE](https://github.com/Mejolov24/SQType) for its calculations
* Pre-calculated LUTs for power()
    - Used to convert note values into playback speed efficiently, allowing for fast and lightweight playback, skipping calculations at runtime.

## Features
 - Pitch bend
 - Pitch slide (*like pitch bend, but for a single voice, also automatic*)
 - Vibrato

## Planned Features
 - ADSR envelope
 - Compile time desactivation of features
## Definitions

### SampleData
```cpp
struct SampleData {
    const char* name;
    const int16_t* data; // pointer to the raw PCM data
    uint32_t length; // Size of the PCM data in samples
    uint32_t loop_start;
    uint32_t loop_end;
    // define loop_start and loop_end as 0 if the sample doesn't loop.
};
```

### Channel Parameters Struct
```cpp
    struct ChannelParameters{
        Q_TYPE pitch_bend = Q_TYPE(0); // in semitones
        Q_TYPE pitch_slide_duration = Q_TYPE(1); // in seconds
        float vibrato_frequency = 0; // in herz
        Q_IDX_TYPE vibrato_range = Q_IDX_TYPE(2); // in semitones
        Q_TYPE volume = Q_TYPE(1); // linear scaling
        bool sustain = false;
    
        LFO lfo;
    };
```
#### Use with
```cpp
setChannelParameters()
getChannelParameters()
```
## API
```cpp
    uint8_t createVoice(const SampleData* sample_data, uint8_t note, Q_TYPE volume, uint8_t channel, bool ignore_note = false); // Creates a voice, if MAX_VOICES is reached, it will steal the oldest voice. Returns the created voice VID
    uint8_t getVIDByChannel(uint8_t note, uint8_t channel);
    Voice getVoiceInfo(uint8_t VID);
    void slideVoice(uint8_t VID, uint8_t channel, uint8_t target_note, uint16_t duration_ms);
    void releaseVoice(uint8_t VID);
    uint8_t isChannelActive(uint8_t channel);
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
```

## Usage Example
```cpp
#define MAX_VOICES 32
#define MAX_CHANNELS 16
#define BUFFER_SIZE 256
#define Q_BIT_TYPE int16_t
#define Q_FRAC_SIZE 12

#include <SynthCore.h>

SynthCore synth;

// example pcm data
const int16_t pcm_data[] = {0, 1000, -1000, 0};

SampleData sample = {
    "Piano",
    pcm_data,
    4, // lenght
    0,  // loop start
    0   // loop end
};

int main() {
    synth.setup(69 , 16000, 0) // initializes LUTS, Base note 69 (A4), 16khz, with a 0 cents offset. (440hz)
    synth.createVoice(&sample, 69, 1, 0);

    // get another voice with an unknown VID
    uint8_t vid = getVIDByChannel(72, 1);
    SynthCore::Voice voice = getVoiceInfo(vid);
    // now you can do something, like stopping it, or sliding it to a new note.
    void slideVoice(vid, 76, 100); // slide to note 76, in 100 miliseconds
    void releaseVoice(vid); // release the voice

    while (1) {
        synth.updateAudioBuffer();
        int16_t* buffer = synth.getAudioBuffer();
        // Send buffer to DAC / I2S / PWM
    }
}
```

## Related proyects
[![CardStudio](https://github-readme-stats.vercel.app/api/pin/?username=Mejolov24&repo=CardStudio)](https://github.com/Mejolov24/CardStudio)
 - Cardputer firmware that uses it

[![SyntCore-Midi](https://github-readme-stats.vercel.app/api/pin/?username=Mejolov24&repo=SynthCore-Midi)](https://github.com/Mejolov24/SynthCore-Midi)
 - Midi helper

[![SynthTracer](https://github-readme-stats.vercel.app/api/pin/?username=Mejolov24&repo=SynthTracer)](https://github.com/Mejolov24/SynthTracer)
 - Oscilloscope visualization over serial, Not really dependent on it, but made *for* it.


Copyright (c) 2026 Guillermo Beckers Rival
Licensed under the GNU GPLv3
