// RasCode Date/Time & Random Number Library
// Time operations and pseudo-random generation

#define _XOPEN_SOURCE 700
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

// Global random seed
static unsigned long sc_random_seed = 1;

// Initialize random number generator
// Usage: @srand[seed]
void sc_srand(unsigned long seed) {
    sc_random_seed = seed ? seed : 1;
}

// Generate pseudo-random number
// Usage: @rand[] -> 0 to RAND_MAX
long sc_rand(void) {
    // Linear congruential generator (simple, lightweight)
    sc_random_seed = (sc_random_seed * 1103515245 + 12345) & 0x7fffffff;
    return sc_random_seed;
}

// Random number in range [0, max)
// Usage: @rand_range[max] -> 0 to max-1
long sc_rand_range(long max) {
    if (max <= 0) return 0;
    return sc_rand() % max;
}

// Random number in range [min, max]
// Usage: @rand_between[min, max] -> min to max inclusive
long sc_rand_between(long min, long max) {
    if (min > max) {
        long tmp = min;
        min = max;
        max = tmp;
    }
    return min + (sc_rand() % (max - min + 1));
}

// Get current time in seconds since epoch
// Usage: @time[] -> seconds
long sc_time(void) {
    return (long)time(NULL);
}

// Get current time in milliseconds
// Usage: @time_ms[] -> milliseconds
long sc_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

// Get current time in microseconds
// Usage: @time_us[] -> microseconds
long sc_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long)(ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
}

// Convert time_t to year
// Usage: @year_from_time[time_val] -> year
long sc_year_from_time(long time_val) {
    struct tm *tm_info = localtime((time_t *)&time_val);
    return tm_info->tm_year + 1900;
}

// Convert time_t to month (1-12)
// Usage: @month_from_time[time_val] -> month
long sc_month_from_time(long time_val) {
    struct tm *tm_info = localtime((time_t *)&time_val);
    return tm_info->tm_mon + 1;
}

// Convert time_t to day (1-31)
// Usage: @day_from_time[time_val] -> day
long sc_day_from_time(long time_val) {
    struct tm *tm_info = localtime((time_t *)&time_val);
    return tm_info->tm_mday;
}

// Convert time_t to hour (0-23)
// Usage: @hour_from_time[time_val] -> hour
long sc_hour_from_time(long time_val) {
    struct tm *tm_info = localtime((time_t *)&time_val);
    return tm_info->tm_hour;
}

// Convert time_t to minute (0-59)
// Usage: @minute_from_time[time_val] -> minute
long sc_minute_from_time(long time_val) {
    struct tm *tm_info = localtime((time_t *)&time_val);
    return tm_info->tm_min;
}

// Convert time_t to second (0-59)
// Usage: @second_from_time[time_val] -> second
long sc_second_from_time(long time_val) {
    struct tm *tm_info = localtime((time_t *)&time_val);
    return tm_info->tm_sec;
}

// Format time as string
// Usage: @strftime[format, time_val, buf] -> buf
long sc_strftime(const char *format, long time_val, char *buf) {
    if (!format || !buf) return 0;
    
    time_t t = (time_t)time_val;
    struct tm *tm_info = localtime(&t);
    
    size_t len = strftime(buf, 256, format, tm_info);
    return len > 0 ? (long)buf : 0;
}

// Parse time from string
// Usage: @strptime[time_str, format] -> time_t
long sc_strptime(const char *time_str, const char *format) {
    if (!time_str || !format) return -1;
    
    struct tm tm_info = {0};
    if (!strptime(time_str, format, &tm_info)) {
        return -1;
    }
    
    time_t t = mktime(&tm_info);
    return (long)t;
}

// Get day of week (0=Sunday, 6=Saturday)
// Usage: @day_of_week[time_val] -> 0-6
long sc_day_of_week(long time_val) {
    struct tm *tm_info = localtime((time_t *)&time_val);
    return tm_info->tm_wday;
}

// Get day of year (0-365)
// Usage: @day_of_year[time_val] -> 0-365
long sc_day_of_year(long time_val) {
    struct tm *tm_info = localtime((time_t *)&time_val);
    return tm_info->tm_yday;
}

// Check if leap year
// Usage: @is_leap_year[year] -> 1 or 0
long sc_is_leap_year(long year) {
    if (year % 400 == 0) return 1;
    if (year % 100 == 0) return 0;
    if (year % 4 == 0) return 1;
    return 0;
}

// Get days in month
// Usage: @days_in_month[year, month] -> number of days
long sc_days_in_month(long year, long month) {
    if (month < 1 || month > 12) return 0;
    
    int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int days_count = days[month];
    
    if (month == 2 && sc_is_leap_year(year)) {
        days_count = 29;
    }
    
    return days_count;
}

/**
 * @rand_new[seed] - Create/seed a new random number generator
 * seed: seed value for RNG
 * Returns: seed value (for demonstration; full RNG would maintain state)
 */
long sc_rand_new(long seed) {
    // In a full implementation, this would create an RNG state
    // For now, this just seeds the global RNG
    srand((unsigned int)seed);
    return seed;
}
