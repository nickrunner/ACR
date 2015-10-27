//Code to Drive Real time clock
//Author: @NickSchrock

//Initialize 16 bit variables for User Input
/*uint16_t hours=0x00;
uint16_t minutes=0x00;
uint16_t seconds=0x00;
uint16_t months=0x00;
uint16_t days=0x00;
uint16_t years=0x00;

//Initialize 8 bit variables
uint8_t hours_byte=0x00;
uint8_t minutes_byte=0x00;
uint8_t seconds_byte=0x00;
uint8_t months_byte=0x00;
uint8_t days_byte=0x00;
uint8_t years_byte=0x00;*/

//#include <stdio.h>
#include <avr/io.h>
#include "RTC_Driver.h"
#include "TWI_Driver.h"
#include <math.h>
#include <stdbool.h>
#include "LCD_Driver.h"
//#include "uart.h"
#include "Alarm_Driver.h"


#define F_CPU 16000000UL
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1

int displayed_hours;
int displayed_minutes;
int displayed_seconds;


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

uint8_t get_Hours(){
	return BCDToDecimal(I2C_ReadRegister(DS1307,HOURS_REGISTER));
}

uint8_t get_Minutes(){
	return BCDToDecimal(I2C_ReadRegister(DS1307,MINUTES_REGISTER));
}

uint8_t get_Seconds(){
	/*USART_Init(MYUBRR);
	stdout = &uart_output;
	stdin = &uart_input;*/
	
	//printf("Made it into the getSeconds function\n");
	uint8_t seconds = I2C_ReadRegister(DS1307,SECONDS_REGISTER);
	//printf("Successfully read something from the RTC\n");
	uint8_t seconds_dec = BCDToDecimal(seconds);
	//printf("Converted from BCD to Decimal\n");
	return seconds_dec;
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
	DecimalToBCD(hours);
	DecimalToBCD(minutes);
	DecimalToBCD(seconds);

	I2C_WriteRegister(DS1307,HOURS_REGISTER, hours);
	I2C_WriteRegister(DS1307,MINUTES_REGISTER, minutes);
	I2C_WriteRegister(DS1307,SECONDS_REGISTER, seconds);
}



//Use this function to hard-code in an initial date to the RTC
void set_Date(uint8_t months, uint8_t days, uint8_t years)
{	
	DecimalToBCD(months);
	DecimalToBCD(days);
	DecimalToBCD(years);


	I2C_WriteRegister(DS1307,MONTHS_REGISTER, months);
	I2C_WriteRegister(DS1307,DAYS_REGISTER, days);
	I2C_WriteRegister(DS1307,YEARS_REGISTER, years);
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
				//while(get_Seconds()<59){
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
	
void RTC_DisplayLargeClock(int xpos, int ypos){
	char update;
	if(get_Hours != displayed_hours){
		update = 'H';
		displayed_hours = get_Hours();
		displayed_minutes = get_Minutes();
		displayed_seconds = get_Seconds();
		}
	else if(get_Minutes != displayed_minutes){
		update = 'M';
		displayed_minutes = get_Minutes();
		displayed_seconds = get_Seconds();
	}
	else if(get_Seconds != displayed_seconds){
		update = 'S';
		displayed_seconds = get_Seconds();
	}
	else 
		update = 'N';
	}

	switch(update)
	{
		case 'H':
			//Write Hours to screen
			if(get_Hours()>9){
			xpos -=7;
			}
			LCD_Clear();
			LCD_GoTo(xpos, ypos);
			LCD_WriteInt(get_Hours(),true);

			//Write Minutes to Screen
			LCD_GoTo(xpos+16,ypos);
			if(get_Minutes()<10){
				LCD_WriteInt(0,true);
			}
			LCD_WriteInt(get_Minutes(),true);
			//LCD_WriteInt(13, true);
			
			//Write Seconds to Screen
			LCD_GoTo(xpos+48,ypos);
			if(get_Seconds()<10){
				LCD_WriteInt(0,true);
			}
			LCD_WriteInt(get_Seconds(),true);
			
			break;

		case 'M':
			//Write Minutes to Screen
			LCD_GoTo(xpos+16,ypos);
			if(get_Minutes()<10){
				LCD_WriteInt(0,true);
			}
			LCD_WriteInt(get_Minutes(),true);
			//LCD_WriteInt(13, true);


			//Write Seconds to Screen
			LCD_GoTo(xpos+48,ypos);
			if(get_Seconds()<10){
				LCD_WriteInt(0,true);
			}
			LCD_WriteInt(get_Seconds(),true);

			break;

		case 'S':
			//Write Seconds to Screen
			LCD_GoTo(xpos+48,ypos);
			if(get_Seconds()<10){
				LCD_WriteInt(0,true);
			}
			LCD_WriteInt(get_Seconds(),true);
			break;

		case 'N':
			break;

	}	
}



