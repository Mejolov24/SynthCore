#include <Arduino.h>
#include <SynthCore.h>
#include "sine_wave.h"
SynthCore synth; // Create an instance of the synthesizer engine class
using voiceCfg = SynthCore::VoiceConfig; // an alias for not typing the class everytime

// this example will simply play the note A4 and fill a buffer to then print it to serial.

void setup(){
    Serial.begin(9600); // Serial for debugging
    // create configuration structure for the new voice:
    voiceCfg my_voice;
    my_voice.sample = sine_wave_sample;
    my_voice.sample_length = sine_wave_sample_length;
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