#include <Arduino.h>
#include "SynthCore.h"

void SynthCore::setBaseNote(uint8_t base_note){
_baseNote =  base_note;
for (int i = 0; i < 128; i++) {
        float ratio = pow(2.0f, (i - (float)_baseNote) / 12.0f);
        
        // Convert to Q10 fixed-point
        _noteStepTable[i] = (uint32_t)(ratio * 1024.0f);
    }
}

void SynthCore::setChannelPitchBend(uint8_t channel, u_int16_t pitch_bend){
  _channel_pitch_bend[channel] = pitch_bend;
}
void SynthCore::addVoice(const VoiceConfig& settings){
  for (int i = 0; i < MAX_VOICES ; i++) {
    if (!Voices[i].active) {
    Voices[i] = settings;
    Voices[i].active = true;
    if (Voices[i]._scaled_length == 0) {Voices[i]._scaled_length = Voices[i].sample_length << 10;} // calculate the Q10 single point scaled length
    return;
    }
  }
}

void SynthCore::removeVoice(uint8_t note, uint8_t channel){
    for (int i = 0; i < MAX_VOICES ; i++){
      if (Voices[i].active && Voices[i].note == note){
            Voices[i].active = false;}
    }
}

int16_t SynthCore::_processVoice(uint8_t voice_index){
  VoiceConfig &voice = Voices[voice_index];


uint32_t base_step = _noteStepTable[voice.note];

  // Get the current pitch bend (Q10: 1024 = 1.0)
  // Check if it's the drum channel (9), otherwise get the channel bend
  uint32_t current_bend = (voice.channel == 9) ? 1024 : _channel_pitch_bend[voice.channel];
  // Both are Q10, so multiplying them results in Q20. Shifting by 10 brings us back to Q10.
  uint32_t step = (base_step * current_bend) >> 10;
  int16_t sample = 0;
if (!voice.active) return 0;

if (voice.index < voice._scaled_length){
  sample = voice.sample[voice.index >> 10];
  voice.index += step;
  sample = (sample * voice.volume) >> 7;
  }
  else{
    if (voice.looping){voice.index = 0;}
    else{voice.active = false;}
    return 0;
      }

    return sample;
}

void SynthCore::stepAudio(){
  int16_t mix = 0;
  int32_t sum = 0;
  uint8_t active_channels_count = 0;

for (int i = 0; i < MAX_CHANNELS; i++) {
    _channel_sum_buffer[i] = 0; 
}

for (int i = 0; i < MAX_VOICES; i++){
_channel_sum_buffer[Voices[i].channel] += _processVoice(i);
}

for (int i = 0; i < MAX_CHANNELS; i++){
    sum += _channel_sum_buffer[i];
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