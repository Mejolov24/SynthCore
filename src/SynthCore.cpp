#include "SynthCore.h"
Q_TYPE SynthCore::lfo_lut[LFO_LUT_SIZE];

void SynthCore::set_digital_gain(Q_TYPE value){
  digital_gain = value;
}

void SynthCore::setup(uint8_t base_note, uint16_t sampling_rate, float cents_ofsset){
  _baseNote =  base_note;
  _sampling_rate = sampling_rate;
  float tuning_ratio = pow(2.0f, cents_ofsset / 1200.0f);
  // LUTS
  for (int i = 0; i < 128; i++) {
          float ratio = pow(2.0f, (i - (float)_baseNote) / 12.0f);
          // Convert to QType
          ratio *= tuning_ratio;
          _noteStepTable[i] = static_cast<Q_STEP_TYPE>(ratio);
      }
  
  const float scale = static_cast<float>(1 << Q_FRAC_SIZE);
  for (size_t i = 0; i < LFO_LUT_SIZE; i++){
      float angle = (static_cast<float>(i) / LFO_LUT_SIZE) * (M_PI * 2);

      Q_BIT_TYPE raw = static_cast<Q_BIT_TYPE>(sin(angle) * scale);
      lfo_lut[i] = Q_TYPE::from_raw(raw);
  }

}

void SynthCore::setChannelParameters(uint8_t channel, const ChannelParameters parameters){
  if (channel >= MAX_CHANNELS) return;
  if(parameters.vibrato_frequency != _channels_paremeters[channel].vibrato_frequency){
    uint32_t previous_phase = _channels_paremeters[channel].lfo.phase;
    _channels_paremeters[channel] = parameters;
    _channels_paremeters[channel].lfo.phase = previous_phase;
    _channels_paremeters[channel].lfo.setFrequency(parameters.vibrato_frequency,_sampling_rate);
  }
  else{
    LFO previous_lfo = _channels_paremeters[channel].lfo;
    _channels_paremeters[channel] = parameters;
    _channels_paremeters[channel].lfo = previous_lfo;
  }
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

uint8_t SynthCore::createVoice(const SampleData* sample_data, uint8_t note, Q_TYPE volume, uint8_t channel, bool ignore_note){
  uint8_t vid = _allocateVID();
  Voice& current_voice = _Voices[vid];

  current_voice.Q_index = 0;
  current_voice.can_loop =
    sample_data->loop_end > sample_data->loop_start &&
    sample_data->loop_end <= sample_data->length;
  current_voice.sample_data = sample_data;
  current_voice.note = note;
  current_voice.Q_volume = volume;
  current_voice.channel = channel;

  current_voice.Q_length = Q_IDX_TYPE(current_voice.sample_data->length);
  current_voice.Q_loop_start = Q_IDX_TYPE(current_voice.sample_data->loop_start);
  current_voice.Q_loop_end = Q_IDX_TYPE(current_voice.sample_data->loop_end );

  current_voice.ignore_note = ignore_note;
  current_voice.held = true;
  current_voice.active = true;
  _SortedVID[_active_voice_count] = vid;
  _active_voice_count ++;

  return vid;
}

uint8_t SynthCore::getVIDByChannel(uint8_t note, uint8_t channel){
    Voice* voice = nullptr;
      // find voice
    for (int i = _active_voice_count - 1; i >= 0 ; i--){
      uint8_t vid = _SortedVID[i];
      voice = &_Voices[vid];
      if (voice->channel != channel){continue;}
      if (voice->note != note){continue;}
      return vid;
    }
    return 255;
}

void SynthCore::releaseVoice(uint8_t VID){
  Voice& voice = _Voices[VID];
  voice.held = false;
}

SynthCore::Voice SynthCore::getVoiceInfo(uint8_t VID){
  return _Voices[VID];
}


void SynthCore::slideVoice(uint8_t VID, uint8_t channel, uint8_t target_note, uint16_t duration_ms){
  Voice* voice = &_Voices[VID];

  uint32_t sample_duration = (uint32_t)duration_ms * _sampling_rate / 1000;
  if(voice->pitch_slide.active){
    voice->pitch_slide.start_note += voice->pitch_slide.GetPitchOffset();
  }
  else{
    voice->pitch_slide.start_note = Q_STEP_TYPE(voice->note);
  }
  voice->pitch_slide.elapsed_samples = 0;
  voice->pitch_slide.sample_duration = sample_duration;
  voice->pitch_slide.target_note = target_note;
  voice->pitch_slide.active = true;

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
  
  Q_IDX_TYPE boundaryA = 0;
  Q_IDX_TYPE boundaryB = voice.Q_length;
  bool looping = false;
  // looping logic
  if (voice.can_loop and (voice.held or channelData.sustain) ){looping = true;}
  if (looping){
    boundaryA = voice.Q_loop_start;
    boundaryB = voice.Q_loop_end;
  }
  else if(! voice.held and not channelData.sustain){
    voice.active = false;
    voice.held = false;
    _removeIDFromSortedVID(VID);
    return 0;
  }
  if (voice.Q_index >= boundaryB){
    if (looping){
      Q_IDX_TYPE loop_length = boundaryB - boundaryA;
      voice.Q_index = boundaryA + ((voice.Q_index - boundaryB) % loop_length);
    }
    else{
        voice.active = false;
        voice.held = false;
        _removeIDFromSortedVID(VID);
        return 0;
        }
    }
    // interpolation
    uint32_t int_index = voice.Q_index;
    Q_TYPE frac = Q_TYPE::from_raw(voice.Q_index.raw & ((1 << Q_FRAC_SIZE) - 1));
    int16_t sampleA = sample_data.data[int_index];
    int16_t sampleB = sampleA; // Default fallback

    if (int_index + 1 < sample_data.length) {
        sampleB = sample_data.data[int_index + 1];
    }

    int32_t interpolated_sample = sampleA + ((static_cast<int32_t>(sampleB) - sampleA) * frac.raw >> Q_FRAC_SIZE);
    // pitch bends
    Q_STEP_TYPE base_step = _noteStepTable[voice.note];
    if (voice.ignore_note) base_step = _noteStepTable[_baseNote];
    
    Q_STEP_TYPE lfo_value = 0;
    if(channelData.vibrato_frequency != 0.0f and channelData.vibrato_range > Q_STEP_TYPE(0)){lfo_value = Q_STEP_TYPE(channelData.lfo.getSample());}
    Q_STEP_TYPE scaled_lfo_offset = (lfo_value * channelData.vibrato_range) / Q_STEP_TYPE(12);

    Q_STEP_TYPE bend = Q_STEP_TYPE(channelData.pitch_bend);
    Q_STEP_TYPE pitch_bend = Q_STEP_TYPE(1) + bend / Q_STEP_TYPE(12);

    Q_STEP_TYPE pitch_slide = voice.pitch_slide.GetPitchOffset() / Q_STEP_TYPE(12);
    if(voice.pitch_slide.finished){
      voice.note = voice.pitch_slide.target_note;
      base_step = _noteStepTable[voice.note];
      voice.pitch_slide.finished = false;
    }
    Q_STEP_TYPE final_bend = pitch_bend + scaled_lfo_offset + pitch_slide;

    Q_STEP_TYPE step = (base_step * final_bend);
    voice.Q_index += step;

int32_t output = (static_cast<int64_t>(interpolated_sample) * digital_gain.raw) >> Q_FRAC_SIZE;
  output = (static_cast<int64_t>(output) * voice.Q_volume.raw) >> Q_FRAC_SIZE;
  return static_cast<int16_t>(CLAMP(output, INT16_MIN, INT16_MAX));
}

uint8_t SynthCore::isChannelActive(uint8_t channel){
  for (int i = 0; i < MAX_VOICES; i++) {
    if (_Voices[i].held && _Voices[i].channel == channel) {
      return i;
    }
    return 255;
  }
}

void SynthCore::stepAudio(){
  int16_t mix = 0;
  int32_t sum = 0;
  uint8_t active_channels_count = 0;

for (int i = 0; i < MAX_CHANNELS; i++) {
    _channel_sum_buffer[i] = 0; 
    _active_channels[i] = false;
}

for (int i = _active_voice_count - 1; i >= 0; i--){
uint8_t vid = _SortedVID[i];
    if (_Voices[vid].active) {
        _channel_sum_buffer[_Voices[vid].channel] += _processVoice(vid);
        _active_channels[_Voices[vid].channel] = true;
    }
}

for (int i = 0; i < MAX_CHANNELS; i++){
  _channel_sum_buffer[i] = static_cast<int32_t>((static_cast<int64_t>(_channel_sum_buffer[i]) * _channels_paremeters[i].volume.raw) >> Q_FRAC_SIZE);
  channel_output[i] = _channel_sum_buffer[i];
  sum += _channel_sum_buffer[i];
  _channels_paremeters[i].lfo.tick();
}

mix = (int16_t)CLAMP(sum, INT16_MIN, INT16_MAX);
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