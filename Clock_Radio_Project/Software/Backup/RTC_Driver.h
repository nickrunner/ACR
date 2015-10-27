/*
 * Header definitions for RTC
 *
 * Created: 9/26/2014 2:00:04 PM
 *  Author: Nick
 */ 

#define SECONDS_REGISTER 0x00
#define MINUTES_REGISTER 0x01
#define HOURS_REGISTER 0x02
#define DAYOFWK_REGISTER 0x03
#define DAYS_REGISTER 0x04
#define MONTHS_REGISTER 0x05
#define YEARS_REGISTER 0x06

uint8_t DecimalToBCD (uint8_t decimalByte);
uint8_t BCDToDecimal (uint8_t bcd);
void get_Time(void);
void set_Time(uint8_t hours, uint8_t minutes, uint8_t seconds);
uint8_t get_Hours();
uint8_t get_Minutes();
uint8_t get_Seconds();
//void RTC_DisplayTickingClock(int xpos, int ypos);
void RTC_DisplayLargeClock(int xpos, int ypos);
void RTC_DisplaySmallClock(int xpos, int ypos);
void RTC_ClearVariables();