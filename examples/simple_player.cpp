#include <Arduino.h>
#include <SynthCore.h>
SynthCore synth; // Create an instance of the synthesizer engine class
using voiceCfg = SynthCore::VoiceConfig; // an alias for not typing the class everytime
// this example will simply play the note A4 and fill a buffer to then print it to serial.

// generate sample.

#define SAMPLE_RATE 22050
#define DURATION_SEC 1        // length of sample
#define FREQUENCY 440.0f     // A4

#define SAMPLE_COUNT (SAMPLE_RATE * DURATION_SEC)

int16_t sine_wave_sample[SAMPLE_COUNT];

void generateSine() {
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        float t = (float)i / SAMPLE_RATE;
        float value = sinf(2.0f * PI * FREQUENCY * t);

        // scale to 16-bit signed
        sine_wave_sample[i] = (int16_t)(value * 32767.0f);
    }
}


void setup(){
    generateSine();
    Serial.begin(9600); // Serial for debugging
    synth.setBaseNote(69); // by default is 69, no need to set it, but for this example we will.
    // create configuration structure for the new voice:
    voiceCfg my_voice;
    my_voice.sample = sine_wave_sample;
    my_voice.sample_length =  my_voice.sample_length = sizeof(sine_wave_sample) / sizeof(sine_wave_sample[0]);
    my_voice.note = 69; // note number is based on midi standard.
    synth.addVoice(my_voice); // yep, just that, everything is inside the configuration :)
}

void loop(){
    int16_t audio_buffer[16]; // create the buffer, in actual use cases we use bigger buffers like 256, 512 or even 1024
    synth.updateAudioBuffer(audio_buffer,16); // update the buffer we just created

    // you dont need to do anything else, but for the sake of understanding, we will print the buffer

    for (int i = 0; i < 16; i++) {
        Serial.print(audio_buffer[i]);
        Serial.print(" "); // Add a space so the numbers don't touch
    }
    
    Serial.println(); // Move to a new line after printing the 16 samples
    delay(100);       // Slow it down so you can actually read it
}

// updateAudioBuffer function is just a helper, if you want you can just use stepAudio() and handle the buffer yourself.