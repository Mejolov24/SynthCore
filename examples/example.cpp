#include <stdint.h>
#include <SynthCore.h>
SynthCore synth; // Create an instance of the synthesizer engine class

bool buffer_starved(){
    return true; //TODO: add real buffer check for aduio
}

void setup(){
    synth.setup(69,22050,0); // base note 69, 22050 sample rate, 0 cents offset
    synth.createVoice(nullptr,69,1,0); // replace nullptr for a real sample

    while (true){
        if (buffer_starved()) {
            synth.updateAudioBuffer(); // updateAudioBuffer function is just a helper, if you want you can just use stepAudio() and handle the buffer yourself.
            int16_t* buffer = synth.getAudioBuffer();
            // do something with the buffer
        }
    }
    
}