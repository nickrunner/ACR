/*
 * Alarm_Driver.h
 *
 * Created: 10/9/2014 1:32:03 PM
 *  Author: Nick
 */ 


void alarm_Set(int hours, int minutes, int seconds);
void alarm_Sound(float volume);
void alarm_Silence(void);
void alarm_Display(int hours, int minutes, int xpos, int ypos);
int alarm_Check();
void one_sec_delay(void);