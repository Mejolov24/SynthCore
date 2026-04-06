#include <Arduino.h>
#include "SynthCore.h"


void SynthCore::addVoice(const VoiceConfig& settings){
  for (int i = 0; i < MAX_VOICES ; i++) {
    if (!Voices[i].active) {
    Voices[i] = settings;
    Voices[i].active = true;
    if (Voices[i]._scaled_length == 0) {Voices[i]._scaled_length = Voices[i].sample_length * 1024;}
    return;
    }
  }
}

void SynthCore::removeVoice(uint8_t channel, uint8_t note){
    for (int i = 0; i < MAX_VOICES ; i++){
      if (Voices[i].active && Voices[i].note == note){
            Voices[i].active = false;}
    }
}

int16_t SynthCore::_processVoice(uint8_t voice){
  int32_t mix = 0;
  int32_t sum = 0;
if (!Voices[voice].active) return 0;

if (Voices[voice].index < Voices[voice]._scaled_length){
  uint32_t index = Voices[voice].index >> 10;
  int16_t sample = Voices[voice].sample[index];

  Voices[voice].index += Voices[voice].pitch_bend;
  sum += (sample * Voices[voice].volume) >> 7;
  }
  else{
    if (Voices[voice].looping){Voices[voice].index = 0;}
    else{Voices[voice].active = false;}
      }

    mix = sum;
    mix = constrain(sum, -32768, 32767);
    return mix;
}

void SynthCore::stepAudio(){
  int16_t mix = 0;
  int32_t sum = 0;
  uint8_t active_channels_count = 0;

for (int i = 0; i < MAX_CHANNELS; i++) {
    channel_buffer[i] = 0; 
}

for (int i = 0; i < MAX_VOICES; i++){
channel_buffer[Voices[i].channel] += _processVoice(i);
}

for (int i = 0; i < MAX_CHANNELS; i++){
    sum += channel_buffer[i];
}

    mix = constrain(sum, -32768, 32767);
    master_mix = mix;
}


void SynthCore::updateAudioBuffer(int16_t* buffer, uint16_t size){
  for (int i = 0; i < size; i++){
    SynthCore::stepAudio();
    buffer[i] = master_mix;
  }
}