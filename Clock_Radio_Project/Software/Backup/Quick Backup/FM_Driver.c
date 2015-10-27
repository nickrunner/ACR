/*
 * FM_Driver.c
 *
 * Created: 10/9/2014 1:28:37 PM
 * Author: Nick Schrock, Kevin Sager
 * Written in reference to https://www.sparkfun.com/products/11083
 */ 

/***************************************************************************************
//TODO: Write this Driver for the SI4705 FM Receiver
****************************************************************************************/


#define LARGE_SPACE 13
#define SMALL_SPACE 6
#define F_CPU 16000000UL
#define BAUD 9600

#include <avr/io.h>
#include <util/delay.h>
#include "FM_Driver.h"
#include "TWI_Driver.h"
#include "LCD_Driver.h"
#include <stdio.h>


uint16_t registers[16];
int current_channel = 879;
bool radio_on = false;

//To get the Si4703 inito 2-wire mode, SEN needs to be high and SDIO needs to be low after a reset
//The breakout board has SEN pulled high, but also has SDIO pulled high. Therefore, after a normal power up
//The Si4703 will be in an unknown state. RST must be controlled

void FM_Init(){
	DDRC |= _BV(SDA);

	//Clear SDIO and RESET
	PORTC &= ~_BV(SDA);
	PORTD &= ~_BV(RST);
	
	_delay_ms(10);
	
	//Set RESET
	PORTD |= _BV(RST);

	_delay_ms(10);
	
	//Set SDIO and SCK
	DDRC &= ~(_BV(5) | _BV(4));
}

void FM_PowerOn(){	
	//FM_Reset();
	radio_on = true;
	
	FM_ReadRegisters();

	registers[0x07] = 0x8100; 	//Enables the oscillator

	FM_UpdateRegisters();

	_delay_ms(500);

	FM_ReadRegisters();
	
	registers[POWERCFG] = 0x4001; 	//Enables the IC
	registers[POWERCFG] |= _BV(SMUTE) | _BV(DMUTE);
	registers[SYSCONFIG1] |= (1<<RDS); //Enable RDS
	registers[SYSCONFIG2] &= ~(1<<SPACE1 | 1<<SPACE0);  //200kHz channel spacing in the US
	registers[SYSCONFIG2] &= (0xFFF0);		//Clear Volume bits
	registers[SYSCONFIG2] |= 0x0F;
	
	//printf("\n\nUpdating registers after config initializations\n\n");
	
	FM_UpdateRegisters();
	
	
	_delay_ms(110);	//Wait for power-up
	
	//printf("\n\nInitialization Complete\n\n");
	PORTC &= ~_BV(0);
}

void FM_SetVolume(uint16_t level){
	I2C_WriteRegister(SI4703, SYSCONFIG2, level);
}

void FM_ReadRegisters(){
	int index = 0;
	I2C_Start();		//set_up-->TWINT, TWSTA, TWEN
	//printf("I2C started\n");
	I2C_SendAddress(SI4703 | READ);
	//printf("Address sent\n");	
	for(int i=0; i<16; i++){
		if(i<6){
			index = i+10;
		}
		else{
			index = i-6;
		}
		registers[index] = I2C_ReadRegister_16bit();
	}
	TWCR = TW_NACK;
	while(!TW_READY);
}

void FM_UpdateRegisters(){
	I2C_Start();
	I2C_SendAddress(SI4703);
	for(int i=0x02; i<0x08; i++){
		//printf("Writing %i to register %i\n", registers[i], i);
		I2C_WriteRegister_16bit(registers[i]);
	}
	I2C_Stop();
}

void FM_Reset(){
	registers[POWERCFG] = 0x0000;
	registers[CHANNEL] = 0x0000;
	registers[SYSCONFIG1] = 0x0000;
	registers[SYSCONFIG2] = 0x0000;
	registers[0x06] = 0x0000;
	registers[0x07] = 0x0100;
	registers[0x08] = 0x0000;
	FM_UpdateRegisters();
}

void FM_DisplayChannel(int channel, int xpos, int ypos, bool large){
	if(channel == 1011 || channel == 879){
		//LCD_Clear();
	}
	int dig1 = 0;
	if(channel<900){
		dig1 = 8;
	}
	else{
		dig1 = 9;
	}
	LCD_GoTo(xpos,ypos);
	if(large){
		if(channel<1000){
			LCD_WriteInt(dig1, true);
			//printf("Channel: %i \nfirst digit: %i\n",channel, digitAtPos(channel,3));
			LCD_GoTo(xpos+LARGE_SPACE-2, ypos);
			LCD_WriteInt(digitAtPos(channel,2), true);
			LCD_GoTo(xpos+(2*LARGE_SPACE)-2,ypos);
			LCD_WriteChar('.');
			LCD_GoTo(xpos+((2*LARGE_SPACE)+2), ypos);
			LCD_WriteInt(digitAtPos(channel,1),true);
		}
		else{
			xpos -=6;
			LCD_GoTo(xpos, ypos);
			LCD_WriteInt(digitAtPos(channel,4), true);
			LCD_GoTo(xpos+LARGE_SPACE-2, ypos);
			LCD_WriteInt(digitAtPos(channel,3), true);
			LCD_GoTo(xpos+2*(LARGE_SPACE-2), ypos);
			LCD_WriteInt(digitAtPos(channel,2),true);
			LCD_GoTo(xpos+(3*LARGE_SPACE-2)+2,ypos);
			LCD_WriteChar('.');
			LCD_GoTo(xpos+(3*LARGE_SPACE-2)+4, ypos);
			LCD_WriteInt(digitAtPos(channel,1),true);
		}
	}
	else{
		if(channel<1000){
			LCD_WriteInt(digitAtPos(channel,3), false);
			LCD_GoTo(xpos+SMALL_SPACE, ypos);
			LCD_WriteInt(digitAtPos(channel,2), false);
			LCD_GoTo(xpos+2*SMALL_SPACE, ypos);
			LCD_WriteChar('.');
			LCD_GoTo(xpos+2*SMALL_SPACE+4,ypos);
			LCD_WriteInt(digitAtPos(channel,1),false);
		}
		else{
			LCD_WriteInt(digitAtPos(channel,4), false);
			LCD_GoTo(xpos+SMALL_SPACE, ypos);
			LCD_WriteInt(digitAtPos(channel,3), false);
			LCD_GoTo(xpos+SMALL_SPACE+SMALL_SPACE, ypos);
			LCD_WriteInt(digitAtPos(channel,2),false);
			LCD_GoTo(xpos+3*SMALL_SPACE, ypos);
			LCD_WriteChar('.');
			LCD_GoTo(xpos+3*SMALL_SPACE+4,ypos);
			LCD_WriteInt(digitAtPos(channel,1),false);
		}
	}
}

int FM_GetChannel(){
	return current_channel; 
}

int FM_TuneToChannel(int channel){
	//TODO: write function to select a channel at a certain frequency
	//eg: 973 > 97.3
	if (channel>MAX_CHANNEL || channel<MIN_CHANNEL){
		channel = MIN_CHANNEL;
	}
	current_channel = channel;
	int step = 0;
	channel *= 10;
	channel -= 8750;
	channel /= 20; 
	
	//printf("\n\nAttempting to Tune to Channel: %i\n\n", channel );
	
	//printf("\n\nReading registers after initialization\n\n");
	FM_ReadRegisters();
	//FM_PrintRegisters();

	
	registers[CHANNEL] &= 0xFE00;	//Clear all channel bits
	registers[CHANNEL] |= channel;	//Enter in new channel
	registers[CHANNEL] |= (1<<TUNE);	//Start the tune bit
	
	//printf("\n\nUpdating Channel Register 3\n\n");
	FM_UpdateRegisters();
	
	
	while(!(registers[STATUSRSSI] & (1<<STC))){
		FM_ReadRegisters();
		_delay_ms(1);
		//printf("waiting for tuning to complete...\n");//wait for tuning to complete
		}
	
	//printf("\n\nReading Registers after Channel Update\n\n");
	FM_ReadRegisters();

	//FM_PrintRegisters();
	
	//printf("\n\nUpdating registers: Clearing TUNE bit\n");
	registers[CHANNEL] &= ~(1<<TUNE);	//Clear tune bit
	
	FM_UpdateRegisters();

	while(registers[STATUSRSSI] & (1<<STC)){
		//printf("Waiting for STC bit to clear...\n ");
		FM_ReadRegisters();
	}
	
	//printf("\n\nTuning Complete\n\n");
	//FM_PrintRegisters();
	PORTC &= ~_BV(1);
	return step;
}

uint8_t FM_GetSignalStrength(){
	FM_ReadRegisters();
	uint8_t strength = 0x00;
	strength |= registers[STATUSRSSI];
	return strength;
}

void FM_DisplaySignalStrength(int xpos, int ypos){
	LCD_GoTo(xpos, ypos);
	int strength = FM_GetSignalStrength();
	if(strength>10){
		LCD_DrawBar(xpos, ypos, 0xC0);
		xpos++;
		xpos++;
		LCD_GoTo(xpos,ypos);
		LCD_WriteChar(' ');
	}
	if(strength>20){
		LCD_DrawBar(xpos, ypos, 0xF0);
		xpos++;
		xpos++;
		LCD_GoTo(xpos,ypos);
		LCD_WriteChar(' ');
	}
	if(strength>30){
		LCD_DrawBar(xpos, ypos, 0xFC);
		xpos++;
		xpos++;
		LCD_GoTo(xpos,ypos);
		LCD_WriteChar(' ');
	}
	if(strength>40){
		LCD_DrawBar(xpos, ypos, 0xFF);
		xpos++;
		xpos++;
	}
}

void FM_Seek(void){
	current_channel += 2;
	FM_TuneToChannel(current_channel);
	while(FM_GetSignalStrength()<20){
		current_channel += 2;
		if(current_channel < MIN_CHANNEL || current_channel > MAX_CHANNEL){
			current_channel = MIN_CHANNEL;
		}
		FM_DisplayChannel(current_channel, 35, 3, true);
		FM_TuneToChannel(current_channel);
	}
}

bool FM_GetRadioState(){
	return radio_on;
}

void FM_PowerDown(){
	radio_on = false;
	FM_ReadRegisters();
	registers[SYSCONFIG1] &= ~(1<<RDS);
	registers[POWERCFG] |= _BV(0) | _BV(6);
	FM_UpdateRegisters();
	PORTC |= _BV(1);
	PORTC |= _BV(0);
}

void FM_PrintRegisters(void){
	for(int i=0; i<16; i++){
		printf("%i: %x\n", i, registers[i]);
	}
}

/*int FM_ReadChannel(){
	//TODO: write function to return the current channel
	//97.3 > 973
}

void FM_Seek(uint8_t direction){
	//TODO: write function to seek to the nearest channel up or down
}
*/




