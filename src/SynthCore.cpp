#include "SynthCore.h"

void SynthCore::set_digital_gain(uint16_t value){
  digital_gain = value;
}

void SynthCore::setup(uint8_t base_note, uint16_t sampling_rate){
  _baseNote =  base_note;
  for (int i = 0; i < 128; i++) {
          float ratio = pow(2.0f, (i - (float)_baseNote) / 12.0f);
          // Convert to Q10 fixed-point
          _noteStepTable[i] = (uint32_t)(ratio * 1024.0f);
      }
      _sampling_rate = sampling_rate;
}

void SynthCore::setChannelParameters(uint8_t channel, const ChannelParameters parameters){
  if (channel >= MAX_CHANNELS) return;
  _channels_paremeters[channel] = parameters;
  _channels_paremeters[channel].lfo.setFrequency(parameters.vibrato_frequency,_sampling_rate);
}

SynthCore::ChannelParameters SynthCore::getChannelParameters(uint8_t channel){
return _channels_paremeters[channel];
}

uint8_t SynthCore::_allocateVID(){

  if (_active_voice_count == MAX_VOICES){
    uint8_t oldestVID = _SortedVID[0];
    _Voices[oldestVID].active = false;
    _Voices[oldestVID].held = false;
    _removeIDFromSortedVID(oldestVID);
    return oldestVID;
  }

  for (int i = 0; i < MAX_VOICES ; i++) {
    Voice& current_voice = _Voices[i];
    if (!current_voice.active) {
      return i;
    }
  }
  return 0;
}

void SynthCore::createVoice(const SampleData* sample_data, uint8_t note, uint8_t velocity, uint8_t channel, bool ignore_note){
  uint8_t vid = _allocateVID();
  Voice& current_voice = _Voices[vid];


  current_voice.can_loop = true;
  current_voice.index = 0;
  if (sample_data->loop_start == 0 and sample_data->loop_end == 0) {current_voice.can_loop = false;}
  current_voice.sample_data = sample_data;
  current_voice.note = note;
  current_voice.velocity = velocity;
  current_voice.channel = channel;
  current_voice._scaled_length = current_voice.sample_data->length << 10; // calculate the Q10 single point scaled length
  current_voice._scaled_loop_start = current_voice.sample_data->loop_start << 10;
  current_voice._scaled_loop_end = current_voice.sample_data->loop_end << 10;
  current_voice.ignore_note = ignore_note;
  current_voice.held = true;
  current_voice.active = true;
  _SortedVID[_active_voice_count] = vid;
  _active_voice_count ++;

}

void SynthCore::_removeIDFromSortedVID(uint8_t VID) {
    int targetIndex = -1;

    for (int i = 0; i < _active_voice_count; i++) {
        if (_SortedVID[i] == VID) {
            targetIndex = i;
            break;
        }
    }

    if (targetIndex != -1) {
        for (int i = targetIndex; i < _active_voice_count - 1; i++) {
            _SortedVID[i] = _SortedVID[i + 1];
        }
        
        // Update the count and clear the now-trailing slot
        _active_voice_count--;
        _SortedVID[_active_voice_count] = 255; // Use 255 as an empty marker
    }
}

void SynthCore::releaseVoiceByNote(uint8_t note, uint8_t channel){
    for (int i = _active_voice_count - 1; i >= 0 ; i--){
      uint8_t vid = _SortedVID[i];
      Voice& current_voice = _Voices[vid];
      if (current_voice.channel != channel){continue;}
      if (current_voice.note != note){continue;}
      current_voice.held = false;
      break;
    }
}

void SynthCore::KillAllVoices(){
  for (int i = 0; i < MAX_VOICES ; i++){
      Voice& current_voice = _Voices[i];
      _SortedVID[i] = 255;
      current_voice.held = false;
      current_voice.active = false;
      _active_voice_count = 0;
  }
}

int16_t SynthCore::_processVoice(uint8_t VID){
  Voice& voice = _Voices[VID];
  if (!voice.active) return 0;
  if (!voice.sample_data) return 0;
  ChannelParameters& channelData = _channels_paremeters[voice.channel];
  const SampleData& sample_data = *(voice.sample_data);
  uint32_t boundaryA = 0;
  uint32_t boundaryB = voice._scaled_length;
  bool looping = false;
  // looping logic
  if (voice.can_loop and (voice.held or channelData.sustain) ){looping = true;}
  if (looping){
    boundaryA = voice._scaled_loop_start;
    boundaryB = voice._scaled_loop_end;
  }
  else if(! voice.held and not channelData.sustain){
    voice.active = false;
    voice.held = false;
    _removeIDFromSortedVID(VID);
    return 0;
  }
  if (voice.index >= boundaryB){
    if (looping){
      uint32_t loop_length = boundaryB - boundaryA;
      voice.index = boundaryA + ((voice.index - boundaryB) % loop_length);
    }
    else{
        voice.active = false;
        voice.held = false;
        _removeIDFromSortedVID(VID);
        return 0;
        }
    }
    // audio processing
    uint32_t int_index = voice.index >> 10;
    uint32_t frac = voice.index & 0x3FF;int16_t sampleA = sample_data.data[int_index];
    int16_t sampleB = sampleA; // Default fallback
    if (int_index + 1 < sample_data.length) {
        sampleB = sample_data.data[int_index + 1];
    }
    int32_t interpolated_sample = sampleA + (((int32_t)sampleB - sampleA) * (int32_t)frac >> 10);
    uint32_t base_step = _noteStepTable[voice.note];
    if (voice.ignore_note) base_step = _noteStepTable[_baseNote];
    
    int16_t lfo_value = 0;
    if(channelData.vibrato_frequency != 0 and channelData.vibrato_range > 0){lfo_value = channelData.lfo.getSample();}
    int32_t raw_bend = static_cast<int32_t>(channelData.pitch_bend - 1024);
    int32_t scaled_bend_offset = (raw_bend * channelData.bend_range) / 12;
    int32_t scaled_lfo_offset = (lfo_value * channelData.vibrato_range) / 12;
    uint32_t current_bend = 1024 + scaled_bend_offset + scaled_lfo_offset;

    uint64_t step = ((uint64_t)base_step * current_bend);
    voice.index += (step >> 10);
    return static_cast<int16_t>((static_cast<int32_t>(interpolated_sample) * digital_gain * voice.velocity) >> 20);
}

void SynthCore::stepAudio(){
  int16_t mix = 0;
  int32_t sum = 0;
  uint8_t active_channels_count = 0;

for (int i = 0; i < MAX_CHANNELS; i++) {
    _channel_sum_buffer[i] = 0; 
}

for (int i = _active_voice_count - 1; i >= 0; i--){
uint8_t vid = _SortedVID[i];
    if (_Voices[vid].active) {
        _channel_sum_buffer[_Voices[vid].channel] += _processVoice(vid);
    }
}

for (int i = 0; i < MAX_CHANNELS; i++){
  _channel_sum_buffer[i] = (_channel_sum_buffer[i]* _channels_paremeters[i].volume) >> 10;
  channel_output[i] = _channel_sum_buffer[i];
  sum += _channel_sum_buffer[i];
  _channels_paremeters[i].lfo.tick();
}

mix = (int16_t)CLAMP(sum, -32768, 32767);
master_mix = mix;
}

int16_t* SynthCore::getAudioBuffer(){
  if (!_buffer_index) return _BufferB; 
  else return _BufferA;
}

void SynthCore::updateAudioBuffer(){
  int16_t* _current_buffer;
  if (!_buffer_index){_current_buffer = _BufferA;}
  else {_current_buffer = _BufferB;}

  for (int i = 0; i < BUFFER_SIZE; i++){
    stepAudio();
    _current_buffer[i] = master_mix;
  }
  _buffer_index = !_buffer_index;
}