//SPI driver for the ATMega328P
//Author: Nick Schrock, Kevin Sager

/***************************************************************************************
This is a short module that is called to initialize the SPI protocol and send data.  
It is relatively short because a lot of the work is done by the only SPI device in the 
system (the LCD Display). 
****************************************************************************************/

//I would like to eventually modify this to be a multi-functional SPI driver much 
//like TWI_Driver.c

#include <avr/io.h>

void SPI_Master_Init(void){

	//Set PB2(~SS), PB3(MOSI), PB5(SCK) as outputs
	DDRB |= _BV(2) | _BV(3) |_BV(5) |_BV(4) |_BV(0);

	//Make SS high		
	PORTB |= _BV(2);

	//Enable SPI in Master Mode (Prescalar Fck/16)
	SPCR |= _BV(SPE) | _BV(MSTR) | _BV(SPR0); 
}

/*void SPI_Master_Init_Interrupt(void){
	DDRB |= _BV(4) | _BV(5) | _BV(7);
	SPCR |= _BV(SPIE) | _BV(SPE) | _BV(MSTR) | _BV(SPR0);
}*/

void SPI_Master_Send(unsigned char data){
	//Select Slave
	//PORTB &= ~(_BV(2));

	//Set data register to send
	SPDR = data;

	//wait for trasmission to complete
	while(!(SPSR & _BV(SPIF)));

	//Make SS high
	//PORTB |= _BV(2);
}
