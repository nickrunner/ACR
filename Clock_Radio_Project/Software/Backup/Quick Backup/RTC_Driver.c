//Code to Drive Real time clock
//Author: @NickSchrock

/***************************************************************************************
This module is dedicated to controlling the functions of the DS1407 Real-Time Clock IC.
The primary functions used in the main state are the function that displays the large
clock.  This is the primary display for the idle state so it must be accurate.
****************************************************************************************/

#include <stdio.h>
#include <avr/io.h>
#include "RTC_Driver.h"
#include "TWI_Driver.h"
#include <math.h>
#include <stdbool.h>
#include "LCD_Driver.h"
#include "Alarm_Driver.h"


#define F_CPU 16000000UL
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#define LARGE_SPACE 13
#define SMALL_SPACE 6

int displayed_hours=100;
int displayed_minutes=100;
int displayed_seconds=100;
int displayed_month=100;
int displayed_day=100;
int displayed_year=100;
bool pm = false;


uint8_t BCDToDecimal (uint8_t bcd)
{
	return ((bcd >> 4)*10+(bcd & 0x0F)); //Converts BCD formatted bit to a decimal formatted bit
}

uint8_t DecimalToBCD (uint8_t decimalByte)
{
	return (((decimalByte / 10) << 4) | (decimalByte % 10));
}

//Subroutine to read hour, minute, second
void get_Time(void){

	//Set 8 bit variables to value of register in RTC
	uint8_t hours_byte = I2C_ReadRegister(DS1307,HOURS_REGISTER);
	uint8_t minutes_byte = I2C_ReadRegister(DS1307,MINUTES_REGISTER);
	uint8_t seconds_byte = I2C_ReadRegister(DS1307,SECONDS_REGISTER);
	

	if (hours_byte & 0x40) // 12hr mode:
		hours_byte &= 0x1F; // use bottom 5 bits (pm bit = temp & 0x20)
	else
		hours_byte &= 0x3F; // 24hr mode: use bottom 6 bits
	
	//Convert to Decimal format
	hours_byte = BCDToDecimal (hours_byte);
	minutes_byte = BCDToDecimal (minutes_byte);
	seconds_byte = BCDToDecimal (seconds_byte);
	
}


int RTC_GetPM(){
	return pm;
}

void RTC_TogglePM(){
	printf("Before toggle: %i \n", pm);
	pm ^= true;
	printf("After toggle %i\n", pm);
}

uint8_t get_Hours(){
	int hours;
	uint8_t hours_byte = BCDToDecimal(I2C_ReadRegister(DS1307,HOURS_REGISTER));
	hours = hours_byte;
	if(hours_byte > 12){
		hours -=12;
	}
	if(hours_byte == 0){
		hours = 12;
	}
	if(hours_byte >= 12){
		pm = true;
	}
	else{
		pm = false;
	}
	return hours;
}

uint8_t get_Minutes()
{
	return BCDToDecimal(I2C_ReadRegister(DS1307,MINUTES_REGISTER));
}

uint8_t get_Seconds()
{
	uint8_t seconds = I2C_ReadRegister(DS1307,SECONDS_REGISTER);
	uint8_t seconds_dec = BCDToDecimal(seconds);
	
	return seconds_dec;
}

uint8_t get_Month(){
	return BCDToDecimal(I2C_ReadRegister(DS1307, MONTHS_REGISTER));
}

uint8_t get_Day(){
	return BCDToDecimal(I2C_ReadRegister(DS1307, DAYS_REGISTER));
}

uint8_t get_Year(){
	return BCDToDecimal(I2C_ReadRegister(DS1307, YEARS_REGISTER));
}


void get_Date(void){

	//Set 8 bit variables to value of register in RTC
	uint8_t months_byte = I2C_ReadRegister(DS1307,MONTHS_REGISTER);
	uint8_t days_byte = I2C_ReadRegister(DS1307,DAYS_REGISTER);
	uint8_t years_byte = I2C_ReadRegister(DS1307,YEARS_REGISTER);
	
	//Convert to Decimal format
	months_byte = BCDToDecimal (months_byte);
	days_byte = BCDToDecimal (days_byte);
	years_byte = BCDToDecimal (years_byte);
}

void set_Time(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
	I2C_WriteRegister(DS1307,HOURS_REGISTER, DecimalToBCD(hours));
	I2C_WriteRegister(DS1307,MINUTES_REGISTER, DecimalToBCD(minutes));
	I2C_WriteRegister(DS1307,SECONDS_REGISTER, DecimalToBCD(seconds));
}



//Use this function to hard-code in an initial date to the RTC
void set_Date(uint8_t months, uint8_t days, uint8_t years)
{
	I2C_WriteRegister(DS1307,MONTHS_REGISTER, DecimalToBCD(months));
	I2C_WriteRegister(DS1307,DAYS_REGISTER, DecimalToBCD(days));
	I2C_WriteRegister(DS1307,YEARS_REGISTER, DecimalToBCD(years));
}



void RTC_DisplaySmallClock(int xpos, int ypos)
{
	while(1)
	{
		if(get_Hours()>9)
		xpos -=7; // xpos = xpos - 7
		LCD_Clear();
		LCD_GoTo(xpos, ypos);
		LCD_WriteInt(get_Hours(),false);
		LCD_WriteChar(':');
		
		for(int i=get_Minutes(); i<60; i++)
		{
			LCD_GoTo(xpos+12,ypos);
			if(get_Minutes()<10){
				LCD_WriteInt(0,false);
			}
			LCD_WriteInt(get_Minutes(),false);
			LCD_WriteChar(':');
			
			for(int j=get_Seconds(); j<60; j++){
				LCD_GoTo(xpos+30,ypos);
				if(get_Seconds()<10){
					LCD_WriteInt(0,false);
				}
				LCD_WriteInt(get_Seconds(),false);
				one_sec_delay();
			}
		}
	}
}

void RTC_DisplayDate(int xpos, int ypos){
	
	char update;

	int month_pos1 = xpos;
	int month_pos2 = month_pos1 + SMALL_SPACE;
	int slash_pos1 = month_pos2 + SMALL_SPACE;
	int day_pos1 = slash_pos1 +SMALL_SPACE;
	int day_pos2 = day_pos1 + SMALL_SPACE;
	int slash_pos2 = day_pos2 + SMALL_SPACE;
	int year_pos1 = slash_pos2 + SMALL_SPACE;
	int year_pos2 = year_pos1 + SMALL_SPACE;


	if(get_Year() != displayed_year){
		update = 'Y';
		displayed_year = get_Year();
		displayed_month = get_Month();
		displayed_day = get_Day();
	}
	else if(get_Month() != displayed_month){
		update = 'M';
		displayed_month = get_Month();
		displayed_day = get_Day();
	}
	else if(get_Day() != displayed_day){
		update = 'D';
		displayed_day = get_Day();
	}
	else {
		update = 'N';
	}

	switch(update)
	{
		case 'Y':
		//Write year to screen
		//LCD_Clear();
		LCD_GoTo(year_pos1, ypos);
		if(displayed_year<10){
			LCD_WriteInt(0,false);
			LCD_GoTo(year_pos2,ypos);
		}
		LCD_WriteInt(displayed_year,false);


		//Write month to Screen
		if(get_Month()>9){
			LCD_GoTo(month_pos1, ypos);
		}
		else{
			LCD_GoTo(month_pos1, ypos);
			LCD_WriteChar(' ');
			LCD_GoTo(month_pos2,ypos);
		}
		LCD_WriteInt(displayed_month,false);
		LCD_GoTo(slash_pos1, ypos);
		LCD_WriteChar('/');
		
		//Write day to Screen
		LCD_GoTo(day_pos1, ypos);
		if(displayed_day<10){
			LCD_WriteInt(0,false);
			LCD_GoTo(day_pos2, ypos);
		}
		LCD_WriteInt(displayed_day,false);
		LCD_GoTo(slash_pos2, ypos);
		LCD_WriteChar('/');
		
		break;

		case 'M':
		//Write month to Screen
		if(get_Month()>9){
			LCD_GoTo(month_pos1, ypos);
		}
		else{
			LCD_GoTo(month_pos1, ypos);
			LCD_WriteChar(' ');
			LCD_GoTo(month_pos2,ypos);
		}
		LCD_WriteInt(displayed_month,false);
		LCD_GoTo(slash_pos1, ypos);
		LCD_WriteChar('/');


		//Write day to Screen
		LCD_GoTo(day_pos1, ypos);
		if(displayed_day<10){
			LCD_WriteInt(0,false);
			LCD_GoTo(day_pos2, ypos);
		}
		LCD_WriteInt(displayed_day,false);
		LCD_GoTo(slash_pos2, ypos);
		LCD_WriteChar('/');
		break;

		case 'D':
		//Write day to Screen
		LCD_GoTo(day_pos1, ypos);
		if(displayed_day<10){
			LCD_WriteInt(0,false);
			LCD_GoTo(day_pos2, ypos);
		}
		LCD_WriteInt(displayed_day,false);
		LCD_GoTo(slash_pos2, ypos);
		LCD_WriteChar('/');
		break;
		
		case 'N':
		break;

	}


}

void RTC_DisplayLargeClock(int xpos, int ypos){
	char update;
	
	int hours_pos1 = xpos - LARGE_SPACE;
	int hours_pos2 = xpos;
	int colon_pos = hours_pos2 + LARGE_SPACE;
	int minutes_pos1 = colon_pos + LARGE_SPACE;
	int minutes_pos2 = minutes_pos1 + LARGE_SPACE;
	
	LCD_GoTo(70, ypos);
	if(pm == true){
		LCD_WriteString("PM");
	}
	else{
		LCD_WriteString("AM");
	}
	
	if(get_Hours() != displayed_hours){
		update = 'H';
		displayed_hours = get_Hours();
		displayed_minutes = get_Minutes();
		displayed_seconds = get_Seconds();
	}
	else if(get_Minutes() != displayed_minutes){
		update = 'M';
		displayed_minutes = get_Minutes();
		displayed_seconds = get_Seconds();
	}
	else if(get_Seconds() != displayed_seconds){
		update = 'S';
		displayed_seconds = get_Seconds();
	}
	else {
		update = 'N';
	}
	

	switch(update)
	{
		case 'H':
		//Write Hours to screen
		if(displayed_hours>9){
			hours_pos2 = hours_pos1;
		}
		//LCD_Clear();
		LCD_GoTo(hours_pos1, ypos-1);
		LCD_WriteString("      ");
		LCD_GoTo(hours_pos1, ypos);
		LCD_WriteString("      ");
		LCD_GoTo(hours_pos1, ypos-2);
		LCD_WriteString("      ");
		LCD_GoTo(hours_pos2, ypos);
		LCD_WriteInt(displayed_hours,true);
		LCD_GoTo(colon_pos, ypos);
		LCD_WriteColon();

		//Write Minutes to Screen
		LCD_GoTo(minutes_pos1,ypos);
		if(displayed_minutes<10){
			LCD_WriteInt(0,true);
			LCD_GoTo(minutes_pos2,ypos);
		}
		LCD_WriteInt(displayed_minutes,true);
		
		//Write Seconds to Screen
		LCD_GoTo(70,1);
		if(displayed_seconds<10){
			LCD_WriteInt(0,false);
			LCD_GoTo(76,1);
		}
		LCD_WriteInt(displayed_seconds,false);
		
		break;

		case 'M':
		//Write Minutes to Screen
		LCD_GoTo(minutes_pos1,ypos);
		if(displayed_minutes<10){
			LCD_WriteInt(0,true);
			LCD_GoTo(minutes_pos2,ypos);
		}
		LCD_WriteInt(displayed_minutes,true);


		//Write Seconds to Screen
		LCD_GoTo(70,1);
		if(displayed_seconds<10){
			LCD_WriteInt(0,false);
			LCD_GoTo(76,1);
		}
		LCD_WriteInt(displayed_seconds,false);

		break;

		case 'S':
		//Write Seconds to Screen
		LCD_GoTo(70,1);
		if(displayed_seconds<10){
			LCD_WriteInt(0,false);
			LCD_GoTo(76,1);
		}
		LCD_WriteInt(displayed_seconds,false);
		
		case 'N':
		break;

	}
}

void RTC_ClearVariables(void){
	displayed_seconds=100;
	displayed_minutes=100;
	displayed_hours=100;
	displayed_month=100;
	displayed_day=100;
	displayed_year=100;
}



