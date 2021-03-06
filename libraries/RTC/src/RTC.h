#ifndef RTC_H
#define RTC_H

#include <Arduino.h>

class APM3_RTC
{
public:
  APM3_RTC();

  void getTime(); //Query the RTC for the current time/date. Loads .seconds, .minute, etc.
  void setTime(uint8_t hour, uint8_t min, uint8_t sec, uint8_t hund,
               uint8_t dayOfMonth, uint8_t month, uint16_t year); //Set current time to provided hundredths/seconds/etc
  void setToCompilerTime();                                       //Set to time when sketch was compiled

  uint32_t hour;
  uint32_t minute;
  uint32_t seconds;
  uint32_t hundredths;

  uint32_t dayOfMonth;
  uint32_t month;
  uint32_t year;
  uint32_t century;

  uint32_t weekday; //0 to 6 representing the day of the week
  const char *textWeekday;

private:
  //Helper functions to convert compiler date/time to ints
  int toVal(char const *pcAsciiStr);
  int mthToIndex(char const *pcMon);
};
#endif //RTC_H
