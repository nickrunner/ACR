/*
 * Header definitions for RTC
 *
 * Created: 9/26/2014 2:00:04 PM
 *  Author: Nick Schrock, Kevin Sager
 */ 

#define SECONDS_REGISTER 0x00
#define MINUTES_REGISTER 0x01
#define HOURS_REGISTER 0x02
#define DAYOFWK_REGISTER 0x03
#define DAYS_REGISTER 0x04
#define MONTHS_REGISTER 0x05
#define YEARS_REGISTER 0x06

#include <stdbool.h>

uint8_t DecimalToBCD (uint8_t decimalByte);
uint8_t BCDToDecimal (uint8_t bcd);
void get_Time(void);
void set_Time(uint8_t hours, uint8_t minutes, uint8_t seconds);
void set_Date(uint8_t months, uint8_t days, uint8_t years);
uint8_t get_Hours();
uint8_t get_Minutes();
uint8_t get_Seconds();
uint8_t get_Month();
uint8_t get_Year();
uint8_t get_Day();
void RTC_DisplayDate(int xpos, int ypos);
void RTC_DisplayLargeClock(int xpos, int ypos);
void RTC_DisplaySmallClock(int xpos, int ypos);
void RTC_ClearVariables();
int RTC_GetPM();
void RTC_TogglePM();