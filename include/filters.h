#ifndef SEISMOTRAK_FILTERS_H
#define SEISMOTRAK_FILTERS_H

#include <Arduino.h>

// classes recommended and created by Claude Sonnet 4.6

class Decimator {
  uint8_t _n, _count = 0;
public:
  Decimator(uint8_t n) : _n(n) {}
  bool tick() { return (++_count % _n) == 0; }
};

class ConsecutiveFilter {
  uint8_t _n, _count = 0;
public:
  ConsecutiveFilter(uint8_t n) : _n(n) {}
  bool update(bool input) {
    if (input) { if (_count < _n) _count++; }
    else        { _count = 0; }
    return _count >= _n;
  }
};

#endif