#include "clock.h"
#include <boost/date_time.hpp>


unsigned long long Clock::Now() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return Convert(ts);
}

unsigned long long Clock::UTCNowAsMS() {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return (Convert(ts) / 1000);
}

long long Clock::UTCNowAsNS() {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return Convert2NS(ts);
}

unsigned long Clock::UTCNowAsSeconds() {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return ts.tv_sec;
}

unsigned long long Clock::Convert(timespec& ts) {
  return ts.tv_sec * 1000000ull + ts.tv_nsec / 1000;
}

long long Clock::Convert2NS(timespec& ts) {
  return ts.tv_sec * 1000000000ull + ts.tv_nsec;
}

unsigned long long Clock::Convert(time_t time) { return time * 1000ull; }

std::string Clock::GetCurrentTimeString() {
  boost::posix_time::time_facet* facet =
      new boost::posix_time::time_facet("%Y-%m-%d %H:%M:%s");
  std::ostringstream timestring;
  timestring.imbue(std::locale(std::cout.getloc(), facet));

  timestring << boost::posix_time::second_clock::universal_time();

  return timestring.str();
}