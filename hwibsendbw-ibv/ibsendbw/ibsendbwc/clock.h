#ifndef CLOCK_H
#define CLOCK_H

#include <time.h>
#include <string>

class Clock {
 public:
  Clock() = delete;

  static unsigned long long Now();

  static unsigned long long UTCNowAsMS();

  static long long UTCNowAsNS();

  static unsigned long UTCNowAsSeconds();

  static unsigned long long Convert(timespec& spec);

  static long long Convert2NS(timespec& spec);

  static unsigned long long Convert(time_t time);

  static std::string GetCurrentTimeString();
};

#endif