# SynthCore

### ⚠️ Early development — not stable
## Overview
SynthCore is a freestanding C++ audio engine designed for real-time embedded audio systems where dynamic allocation and floating-point operations are not acceptable.

## Technical aspects
* Free standing C++
    - Only depends on <stdint.h> and <cmath>
* Stack only, no heap memory.
* Configurable at compile time.
    ```cpp
    #define MAX_VOICES 32
    #define MAX_CHANNELS 16
    #define BUFFER_SIZE 256
    ```
* Fixed-point arithmetic
* Pre-calculated LUTs for power()
    - Used to convert note values into playback speed efficiently, allowing for fast and lightweight playback, skipping calculations at runtime.

* Voice looping
* Per channel parameters
* Independent SampleData per voice 
    - (decoupled from channels)
* Double buffering helper
    - helper to get and update the buffers, allowing for seamless jitter-free audio.

## Definitions

### SampleData Interface
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
        uint16_t pitch_bend = 1024;
        uint8_t volume = 127;
        bool sustain = false;
    };
```
#### Use with
```cpp
setChannelParameters()
getChannelParameters()
```
## API
```cpp
    void createVoice(const SampleData* sample_data, uint8_t note, uint8_t velocity, uint8_t channel); // Creates a voice, if MAX_VOICES is reached, it will steal the oldest voice
    void releaseVoiceByNote(uint8_t note, uint8_t channel); // release the voice
    void setChannelParameters(uint8_t channel, const ChannelParameters parameters);
    ChannelParameters getChannelParameters(uint8_t channel);
    void setBaseNote(uint8_t base_note); // set the base note of all samples (Reference point), default value is 69
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

#include <SynthCore.h>

SynthCore synth;

// Fake PCM data (example only)
const int16_t pcm_data[] = {0, 1000, -1000, 0};

SampleData sample = {
    "Piano",
    pcm_data,
    4,
    0,  // loop start
    0   // loop end
};

int main() {
    // Optional: change base note (default = 69, A4)
    synth.setBaseNote(72);

    synth.createVoice(&sample, 69, 127, 0);

    while (1) {
        synth.updateAudioBuffer();
        int16_t* buffer = synth.getAudioBuffer();

        // Send buffer to DAC / I2S / PWM
    }
}
```

Copyright (c) 2026 Guillermo Beckers Rival
Licensed under the GNU GPLv3