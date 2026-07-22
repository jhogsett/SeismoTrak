#ifndef SEISMOTRAK_FILTERS_H
#define SEISMOTRAK_FILTERS_H

#include <Arduino.h>

// classes recommended and created by Claude Sonnet 4.6

// Provide a true return value once for every N calls, otherwise return false
class Decimator {
public:
  Decimator(uint8_t n) : _n(n == 0 ? 1 : n) {}

  bool tick() { 
    return (++_count % _n) == 0; 
  }

private:
  uint8_t _n;
  uint8_t _count = 0;
};

// Require N calls with true for a true return value, otherwise return false
class ConsecutiveFilter {
public:
  ConsecutiveFilter(uint8_t n) : _n(n == 0 ? 1 : n) {}

  bool update(bool input) {
    if(input) { 
      if(_count < _n){
        _count++;
      } 
    } else { 
      _count = 0; 
    }

    return _count >= _n;
  }

private:
  uint8_t _n;
  uint8_t _count = 0;
};

#endif