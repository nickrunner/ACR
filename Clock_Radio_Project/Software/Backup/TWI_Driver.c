/// twi_master.c  
// Nick Schrock
// Written with reference to http://w8bh.net/avr/AvrDS1307.pdf
// twi_master code   

/***************************************************************************************
This is the driver module for the two wire I2C communication protocol.  The functions in
 this module are used by the microcontroller to send and receive data from the SI4705 FM
 receiver and the DS1407 Real-Time clock.  This module is vital for the overall system 
 because it enables communication between the microcontroller and the various peripheral 
 devices.  
****************************************************************************************/

#include <stdio.h>
//#include "uart.h"
#include <avr/io.h>			//Library for use with AVR
#include <avr/interrupt.h>	//Library for AVR Interrupts

#define F_CPU 16000000UL
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
//#define F_CPU	16000000L	//CPU clock speed
#define F_SCL	100000		//I2C clock speed
//#include <util/twi.h>		//Header for built in functions 328P (delays)
#include <stdlib.h>			//Header for studio library
#include "TWI_Driver.h"		//'Home made' header for Two Wire Interface Drivers to be called


//Initialize Bit register to proper frequency
void I2C_Init(){
	TWSR = 0;							//set prescalar to 0
	TWBR = ((F_CPU/F_SCL)-16)/2;		//Set Bit register to SCL frequency
	}
	
uint8_t I2C_Start(){
	TWCR = TW_START;				//send start condition (TWINT, TWISTA, TWEN)
	while(!TW_READY);				//wait for TWINT bit in TWCR to be logic 0
	return (TW_STATUS==0x08);		//return 1 if found returns value of status register 0000 1000
}

uint8_t I2C_SendAddress(uint8_t address){
	TWDR = address;				//load slave address
	TWCR = TW_SEND;				//send the address (TWINT, TWEN)
	while(!TW_READY);			//wait to be ready TWINT in TWCR to be logic 0
	return(TW_STATUS==0x18);		//return 1 if found returns value of status register 0001 1000
}

uint8_t I2C_Write (uint8_t data){
	TWDR = data;				//TWDR = Data register
	TWCR = TW_SEND;				//Conrol Register (TWINT and TWEN)
	while(!TW_READY);			//wait for TWINT low
	return (TW_STATUS!=0x28);	//return 1 if found Returns Ack SLave addr;register 1111 1000
}

void I2C_Stop(){
	TWCR = TW_STOP;		//send stop condition
}

uint8_t I2C_ReadNACK(){// reads a data byte from slave
	//printf("Made it to Read_Nack function\n");
	TWCR = TW_NACK;		// nack last byte = not reading more data after
	//printf("set TWCR to TW_NACK\n");
	while(!TW_READY);	//wait TWINT not logic 1
	//printf("TWINT is pulled low\n");
	return TWDR;		//return whats in the Data Register
}

uint16_t I2C_Read(){
	while(!TW_READY);
	return TWDR;
}


void I2C_WriteRegister(uint8_t device_address, uint8_t deviceRegister, uint8_t data){	

	I2C_Start();		//Start Condition (TWINT,TWSTA,TWEN)
	I2C_SendAddress(device_address);//Send Master DS1307 Addr with 'Ack' code
	I2C_Write(deviceRegister);		//Load, send TWCR, and wait TWINT cleared
	I2C_Write(data);	//Interrupt, enable, wait, Ack
	I2C_Stop();		//Stop Condition: (TWINT(set),TWSTO,TWEN)	
}

void I2C_WriteRegister_16bit(uint16_t data){
	uint8_t low_byte = data & 0x00FF;
	printf("low byte: %i", low_byte);
	uint8_t high_byte = data >> 8;
	printf("\nhigh byte: %i", high_byte);
	I2C_Write(high_byte);
	I2C_Write(low_byte);
}

uint8_t I2C_ReadRegister(uint8_t device_address, uint8_t deviceRegister){
	uint8_t data = 0;	//Initialize what will be sent so no confusion
	I2C_Start();		//set_up-->TWINT, TWSTA, TWEN
	I2C_SendAddress(device_address);	//Master RTC Addr with ACK
	I2C_Write(deviceRegister);	//Load, send BID, wait, returns Ack SLave
	I2C_Start();		//set_up-->TWINT, TWSTA, TWEN
	I2C_SendAddress(device_address+READ);	//Send Master RTC Addr with Ack, and Read Bit (0xd1)
	data = I2C_ReadNACK();		//Master Requests byte with Nack(0x58)
	I2C_Stop();		//TWINT set, TWSTO and TWEN
	return data;	//Read from SLave's info from TWI Ack Status Code
}

uint16_t I2C_ReadRegister_16bit(){
	uint16_t data = 0;	
	uint8_t high_byte = I2C_Read();
	uint8_t low_byte = I2C_Read();	//Master Requests byte with Nack(0x58)
	while(!TW_READY);
	printf("High Byte: %i\n", high_byte);
	printf("Low Byte: %i\n", low_byte);
	data = high_byte << 8;
	data |= low_byte;
	return data;	//Read from SLave's info from TWI Ack Status Code
}




