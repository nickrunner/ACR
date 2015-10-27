/*
 * Alarm_Driver.h
 *
 * Created: 10/9/2014 1:32:03 PM
 *  Author: Nick Schrock, Kevin Sager
 */ 


void alarm_Set(int hours, int minutes, int seconds);
void alarm_Sound(float volume);
void alarm_Silence(void);
void alarm_Display(int hours, int minutes, int xpos, int ypos);
int alarm_Check();
void one_sec_delay(void);
void alarm2_Set(int hours, int minutes, int seconds);
void alarm_Snooze();
uint8_t get_Alarm_Minute();
uint8_t get_Alarm_Hour();
uint8_t get_Alarm_Second();
uint8_t get_Alarm2_Minute();
uint8_t get_Alarm2_Second();
uint8_t get_Alarm2_Hour();
void set_Alarm2Ring(bool ring);
void set_AlarmRing(bool ring);
