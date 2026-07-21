#ifndef WINDOWED_MEAN_H
#define WINDOWED_MEAN_H
#include "Arduino.h"

class WindowedMean
{
public:
  WindowedMean(long window_length, float primed_value);
  float sample(float sample);
  float mean(void);

private:
  long _window_length;
  float _mean_accum;
  float _last_mean;

};

WindowedMean::WindowedMean(long window_length, float primed_value){
  _window_length = window_length;
  _mean_accum = primed_value * _window_length;
  _last_mean = primed_value;
}

float WindowedMean::sample(float sample){
  _mean_accum -= _last_mean;
  _mean_accum += sample;
  _last_mean = _mean_accum / _window_length;
  return _last_mean;
}

float WindowedMean::mean(void){
  return _last_mean;
}
#endif
