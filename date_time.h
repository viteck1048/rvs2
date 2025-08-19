#ifndef DATE_TIME_H
#define DATE_TIME_H

/**
 * Converts a date string in the format "Date: Day, DD Mon YYYY HH:MM:SS GMT"
 * to Unix timestamp (seconds since January 1, 1970).
 * 
 * @param dateStr The date string to parse
 * @return Unix timestamp (seconds since January 1, 1970)
 */
uint32_t dateStrToTimestamp(const char* dateStr);

/**
 * Converts a Unix timestamp to date components.
 * 
 * @param timestamp Unix timestamp (seconds since January 1, 1970)
 * @param day Output: day of month (1-31)
 * @param month Output: month (1-12)
 * @param dayOfWeek Output: day of week (1=Monday, 7=Sunday)
 * @param hour Output: hours (0-23)
 * @param minute Output: minutes (0-59)
 * @param second Output: seconds (0-59)
 */
void timestampToDate(uint32_t timestamp, byte* day, byte* month, byte* dayOfWeek, 
                     byte* hour, byte* minute, byte* second);

#endif // DATE_TIME_H
