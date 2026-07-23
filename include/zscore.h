#ifndef ZSCORE_H
#define ZSCORE_H

#include <Arduino.h>
#include <windowed_mean.h>

class ZScore
{
public:

  // TODO: make the newed objects instead static
  ZScore(uint16_t event_window_length, uint16_t baseline_window_length, float primed_value, float noise_floor, float event_threshold){
    _pmean = new WindowedMean(event_window_length, primed_value);    
    _pmad_event = new WindowedMean(event_window_length, 0.0);    
    _pmad_baseline = new WindowedMean(baseline_window_length, 0.0);    
    _noise_floor = noise_floor;
    _event_threshold = event_threshold;
    _last_event_score = 0.0;
    _last_sample_score = 0.0;
    _last_baseline_score = 0.0;
    _event_triggered = false;
    _event_active = false;
  }

  float sample(float _sample){
    // maintain mean sample
    float mean = _pmean->sample(_sample);

    // compute sample deviation from the mean
    float dev = fabs(_sample - mean);

    // maintain mean deviation
    float mad_event = _pmad_event->sample(dev);

    // ensure the current baseline accounts for the noise floor
    float safe_baseline_mean = _pmad_baseline->mean();
    if (safe_baseline_mean < _noise_floor) {
        safe_baseline_mean = _noise_floor;
    }

    // compute the immediate z score of the latest sample for event detection
    _last_sample_score = dev / safe_baseline_mean;

    // compute a tentative z score based on the deviation and baseline means
    _last_event_score = mad_event / safe_baseline_mean;

    // ensure the baseline (background) is not affected when the event threshold is crossed
    if(_last_event_score < _event_threshold){
      float mad_baseline = _pmad_baseline->sample(mad_event);

      // ensure the new baseline accounts for the noise floor
      if(mad_baseline < _noise_floor){
        mad_baseline = _noise_floor;
      }

      // compute the z score based on the mean deviation and baseline
      _last_baseline_score = mad_event / mad_baseline;

      // Automatically turns off when the smooth score falls below threshold
      _event_active = false; 

    } else {
      // during an event use the z score computed on the unaffected baseline
      _last_baseline_score = _last_event_score;

      // Maintain the "Currently Happening" state
      _event_active = true;
      
      // Trip the "Event Triggered" trap if it isn't already tripped
      _event_triggered = true; 
    }

    return _last_baseline_score;
  }

  float mean(){
    return _pmean->mean();
  }

  float sample_score(){
    return _last_sample_score;
  }

  float event_score(){
    return _last_event_score;
  }

  float baseline_score(){
    return _last_baseline_score;
  }

  bool is_event_active(){
      return _event_active;
  }

  // Returns true if an earthquake was detected since the last check.
  // Automatically resets itself to false the exact moment you read it.
  bool get_event_triggered(){
      bool temp_trap = _event_triggered; // Save the current state
      _event_triggered = false;          // Reset the trap immediately 
      return temp_trap;                  // Return the saved state
  }

private:
  WindowedMean * _pmean;
  WindowedMean * _pmad_event;
  WindowedMean * _pmad_baseline;
  float _noise_floor;
  float _event_threshold;
  float _last_event_score;
  float _last_sample_score;
  float _last_baseline_score;
  bool _event_triggered;
  bool _event_active;
};

#endif
