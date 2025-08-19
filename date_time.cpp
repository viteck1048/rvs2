/*
typedef unsigned char byte;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef byte uint8_t;*/

#include <cstdint>
typedef uint8_t byte;
#include "date_time.h"
#include <Arduino.h>
#include <string.h>



// Number of days in each month (non-leap year)
const byte daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Month name to number mapping
struct MonthMap {
  const char* name;
  byte number;
};

const MonthMap monthMap[] = {
  {"Jan", 1}, {"Feb", 2}, {"Mar", 3}, {"Apr", 4},
  {"May", 5}, {"Jun", 6}, {"Jul", 7}, {"Aug", 8},
  {"Sep", 9}, {"Oct", 10}, {"Nov", 11}, {"Dec", 12}
};

// Day of week name to number mapping (1=Monday, 7=Sunday)
struct DayMap {
  const char* name;
  byte number;
};

const DayMap dayMap[] = {
  {"Mon", 1}, {"Tue", 2}, {"Wed", 3}, {"Thu", 4},
  {"Fri", 5}, {"Sat", 6}, {"Sun", 7}
};

// Check if a year is a leap year
bool isLeapYear(uint16_t year) {
  return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

// Convert month name to month number (1-12)
byte getMonthNumber(const char* monthName) {
  for (byte i = 0; i < 12; i++) {
    if (strncmp(monthMap[i].name, monthName, 3) == 0) {
      return monthMap[i].number;
    }
  }
  return 0; // Invalid month
}

// Convert day name to day number (1-7)
byte getDayNumber(const char* dayName) {
  for (byte i = 0; i < 7; i++) {
    if (strncmp(dayMap[i].name, dayName, 3) == 0) {
      return dayMap[i].number;
    }
  }
  return 0; // Invalid day
}

uint32_t dateStrToTimestamp(const char* dateStr) {
  // Example format: "Date: Wed, 21 Oct 2015 07:28:00 GMT"
  char dayName[4] = {0};
  byte day = 0;
  char monthName[4] = {0};
  uint16_t year = 0;
  byte hour = 0;
  byte minute = 0;
  byte second = 0;
  
  // Find the start of the date part (after "Date: ")
  const char* dateStart = strstr(dateStr, "Date: ");
  if (dateStart == NULL) {
    return 0; // Invalid format
  }
  dateStart += 6; // Skip "Date: "
  
  // Parse the date components
  if (sscanf(dateStart, "%3s, %hhu %3s %hu %hhu:%hhu:%hhu", 
             dayName, &day, monthName, &year, &hour, &minute, &second) != 7) {
    return 0; // Invalid format
  }
  
  byte month = getMonthNumber(monthName);
  if (month == 0) {
    return 0; // Invalid month
  }
  
  // Calculate days since 1970-01-01
  uint32_t timestamp = 0;
  
  // Days from 1970 to the year before
  uint16_t yearsSince1970 = year - 1970;
  uint32_t days = yearsSince1970 * 365;
  
  // Add leap days from 1970 to the year before
  for (uint16_t y = 1970; y < year; y++) {
    if (isLeapYear(y)) {
      days++;
    }
  }
  
  // Add days from months in the current year
  for (byte m = 1; m < month; m++) {
    days += daysInMonth[m];
    if (m == 2 && isLeapYear(year)) {
      days++; // February in leap year
    }
  }
  
  // Add days in the current month
  days += day - 1; // -1 because we want days since 1970-01-01, and day starts from 1
  
  // Convert days to seconds and add time
  timestamp = days * 86400UL; // 86400 seconds in a day
  timestamp += hour * 3600UL;
  timestamp += minute * 60UL;
  timestamp += second;
  
  return timestamp;
}

void timestampToDate(uint32_t timestamp, byte* day, byte* month, byte* dayOfWeek, 
                     byte* hour, byte* minute, byte* second) {
  // Calculate seconds, minutes, hours
  *second = timestamp % 60;
  timestamp /= 60;
  *minute = timestamp % 60;
  timestamp /= 60;
  *hour = timestamp % 24;
  timestamp /= 24; // Now timestamp is days since 1970-01-01
  
  // Calculate day of week (1970-01-01 was Thursday = 4)
  *dayOfWeek = ((timestamp + 3) % 7) + 1; // +3 because 1970-01-01 was Thursday, +1 to make Monday=1
  
  // Calculate year, month, day
  uint16_t year = 1970;
  uint32_t days = timestamp;
  
  while (true) {
    uint16_t daysInYear = isLeapYear(year) ? 366 : 365;
    if (days < daysInYear) {
      break;
    }
    days -= daysInYear;
    year++;
  }
  
  // Calculate month and day
  *month = 1;
  while (true) {
    byte daysInCurrentMonth = daysInMonth[*month];
    if (*month == 2 && isLeapYear(year)) {
      daysInCurrentMonth++; // February in leap year
    }
    
    if (days < daysInCurrentMonth) {
      break;
    }
    
    days -= daysInCurrentMonth;
    (*month)++;
  }
  
  *day = days + 1; // +1 because day starts from 1
}
