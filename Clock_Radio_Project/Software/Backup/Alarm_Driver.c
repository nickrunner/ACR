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

#define LARGE_SPACE 13

	
 uint8_t alarm_hour=0;
 uint8_t alarm_minute=0;
 uint8_t alarm_second=0;

void alarm_Set(int hours, int minutes, int seconds){
	alarm_hour = hours;
	alarm_minute = minutes;
	alarm_second = seconds;
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
}

void alarm_Sound(float volume){
	//volume = 100-volume;
	if(volume>90){
		volume = 90;
	}
	OCR1A = (volume/100)*ICR1;
	for(int i=0; i<15; i++)
		one_sec_delay();
	OCR1A=0;
}

void alarm_Silence(void){
	OCR1A = 0;
}

int alarm_Check(){
	if(get_Hours() == alarm_hour && get_Minutes() == alarm_minute && get_Seconds() == alarm_second){
		return 1;
	}

	else{
		return 0;
	}
}
