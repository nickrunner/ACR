/*
 * Alarm_Driver.c
 *
 * Created: 10/9/2014 1:28:37 PM
 *  Author: Nick Schrock, Kevin Sager
 */ 

/***************************************************************************************

The primary function of this module is to factor out some of the code responsible 
for controlling the alarm system.  This is a relatively short module used to set, 
sound, and silence the alarm.

****************************************************************************************/
  
 #include <avr/io.h>
 #include <stdbool.h>
 #include "RTC_Driver.h"
 #include "Alarm_Driver.h"
 #include "LCD_Driver.h"
 #include "FM_Driver.h"
 #include <stdio.h>

#define LARGE_SPACE 13

	
 uint8_t alarm_hour=0;
 uint8_t alarm_minute=0;
 uint8_t alarm_second=0;
 
 uint8_t alarm2_hour=0;
 uint8_t alarm2_minute=0;
 uint8_t alarm2_second=0;
 int snooze_num;
 
 bool alarm_ring = false;
 bool alarm2_ring = false;
 
 bool alarm_set = false;
 bool alarm2_set = false;

uint8_t get_Alarm_Minute(){
	return alarm_minute;
}

uint8_t get_Alarm_Second(){
	return alarm_second;
}
uint8_t get_Alarm_Hour(){
	return alarm_hour;
}
uint8_t get_Alarm2_Minute(){
	return alarm2_minute;
}
uint8_t get_Alarm2_Second(){
	return alarm2_second;
}
uint8_t get_Alarm2_Hour(){
	return alarm2_hour;
}

void set_Alarm2Ring(bool ring){
	alarm2_ring = ring;
}
void set_AlarmRing(bool ring){
	alarm_ring = ring;
}

void alarm_Set(int hours, int minutes, int seconds){
	alarm_hour = hours;
	alarm_minute = minutes;
	alarm_second = seconds;
	alarm_set = true;
}

void alarm2_Set(int hours, int minutes, int seconds){
	alarm2_hour = hours;
	alarm2_minute = minutes;
	alarm2_second = seconds;
	alarm2_set = true;
}

void alarm_Display(int minutes, int hours, int xpos, int ypos){
	
	int hours_pos1 = xpos - LARGE_SPACE;
	int hours_pos2 = xpos;
	int colon_pos = hours_pos2 + LARGE_SPACE;
	int minutes_pos1 = colon_pos + LARGE_SPACE;
	int minutes_pos2 = minutes_pos1 + LARGE_SPACE;
	
	//Write Hours to screen
	if(hours>9){
		hours_pos2 = hours_pos1;
	}
	LCD_Clear();
	LCD_GoTo(hours_pos2, ypos);
	LCD_WriteInt(hours,true);
	LCD_GoTo(colon_pos, ypos);
	LCD_WriteColon(colon_pos, ypos);

	//Write Minutes to Screen
	LCD_GoTo(minutes_pos1,ypos);
	if(minutes<10){
		LCD_WriteInt(0,true);
		LCD_GoTo(minutes_pos2,ypos);
	}
	LCD_WriteInt(minutes,true);
	
	if(RTC_GetPM()){
		LCD_GoTo(70,2);
		LCD_WriteString("PM");
	}
	else{
		LCD_GoTo(70,2);
		LCD_WriteString("AM");
	}
}



void alarm_Silence(void){
	alarm_ring = false;
	alarm2_ring = false;
	if(FM_GetRadioState()){
		FM_PowerDown();
	}
	OCR1A = 0;
}

int alarm_Check(){
	if(get_Hours() == alarm_hour && get_Minutes() == alarm_minute && get_Seconds() == alarm_second && !(PINC & _BV(3))){
		return 1;
	}
	
	if(get_Hours() == alarm2_hour && get_Minutes() == alarm2_minute && get_Seconds() == alarm2_second && !(PIND & _BV(4))){
		return 1;
	}

	else{
		return 0;
	}
}


void alarm_Snooze(){
	if(alarm_ring==1){
		alarm_minute += 10;
		if(alarm_minute>60){
			alarm_hour++;
			alarm_minute -=60;
		}
	}
		//alarm_Set(alarm_hour, alarm_minute, 0);
	if(alarm2_ring==1){
		alarm2_minute += 10;
		if(alarm2_minute>60){
			alarm2_hour++;
			alarm2_minute -=60;
		}
		//alarm2_Set(alarm2_hour, alarm2_minute, 0);
	}
	alarm_Silence();
}

