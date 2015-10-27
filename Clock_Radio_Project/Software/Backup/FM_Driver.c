//Driver for SI4704
//Author: Nick Schrock, Kevin Sager


#define LARGE_SPACE 13
#define SMALL_SPACE 6
uint16_t registers[16];

//To get the Si4703 inito 2-wire mode, SEN needs to be high and SDIO needs to be low after a reset
//The breakout board has SEN pulled high, but also has SDIO pulled high. Therefore, after a normal power up
//The Si4703 will be in an unknown state. RST must be controlled

void FM_Init(){
	DDRD |= _BV(RST);
	DDRC |= _BV(SDA)
	PORTD &= ~_BV(RST);
	PORTC &= ~_BV(SDA);
	_delay_ms(1);
	PORTD |= _BV(RST);
	_delay_ms(1);

	FM_ReadRegisters();
	registers[0x07] = 0x8100; 	//Enables the oscillator
	FM_UpdateRegisters();

	_delay_ms(500);

	FM_ReadRegisters();
	registers[POWERCFG] = 0x4001; 	//Enables the IC
	registers[SYSCONFIG1] |= (1<<RDS); //Enable RDS
	registers[SYSCONFIG2] &= ~(1<<SPACE1 | 1<<SPACE0);  //200kHz channel spacing in the US
	registers[SYSCONFIG2] &= 0xFFF0;		//Clear Volume bits
	FM_SetVolume(0x0001);
	FM_UpdateRegisters();

	_delay_ms(110);	//Wait for power-up
}

void FM_SetVolume(uint16_t level){
	I2C_WriteRegister(SI4703, SYSCONFIG2, level);
}

void FM_ReadRegisters(){
	int index = 0;
	for(int i=0; i++; i<16){
		index = i;
		if(i>5){
			index = i+5;
		}
		registers[i] = I2C_ReadRegister_16bit(SI4703, index);
	}
}

void FM_UpdateRegisters(){
	int index = 0;
	for(int i=0; i++; i<16){
		index = i;
		if(i>5){
			index = i+5;
		}
		I2C_WriteRegister(SI4703, index, registers[i]);
	}
}

int digitAtPos(int number, int pos)
{
	return ( number / (int)pow(10.0, pos-1) ) % 10;
}

void FM_DisplayChannel(int channel, xpos, ypos){
	LCD_Clear();
	LCD_GoTo(xpos,ypos);
	if(channel<1000){
		LCD_WriteInt(digitAtPos(channel,1), true);
		LCD_GoTo(xpos+LARGE_SPACE, ypos);
		LCD_WriteInt(digitAtPos(channel,2), true);
		LCD_GoTo(xpos+LARGE_SPACE+SMALL_SPACE, ypos);
		LCD_WriteInt(digitAtPos(channel,3),true);
	}
	else{
		LCD_WriteInt(digitAtPos(channel,1), true);
		LCD_GoTo(xpos+LARGE_SPACE, ypos);
		LCD_WriteInt(digitAtPos(channel,2), true);
		LCD_GoTo(xpos+LARGE_SPACE+LARGE_SPACE, ypos);
		LCD_WriteInt(digitAtPos(channel,3),true);
		LCD_GoTo(xpos+2*LARGE_SPACE+SMALL_SPACE);
		LCD_WriteInt(digitAtPos(channel,4),true);
	}
}


void FM_TuneToChannel(int channel){
	//TODO: write function to select a channel at a certain frequency
	//eg: 973 > 97.3
	channel *= 10;
	channel -= 8750;
	FM_ReadRegisters();
	registers[CHANNEL] &= 0xFE00;	//Clear all channel bits
	registers[CHANNEL] |= channel;	//Enter in new channel
	registers[CHANNEL] |= (1<TUNE);	//Start the tune bit
	FM_UpdateRegisters();

	while(si4703_registers[STATUSRSSI] & (1<<STC)) != 0);	//wait for tuning to complete
	
	FM_ReadRegisters();
	registers[CHANNEL] &= ~(1<<TUNE);	//Clear tune bit
	FM_UpdateRegisters();

	while((si4703_registers[STATUSRSSI] & (1<<STC)) == 0);	//wait for IC to clear STC
}

int FM_ReadChannel(){
	//TODO: write function to return the current channel
	//97.3 > 973
}

void FM_Seek(uint8_t direction){
	//TODO: write function to seek to the nearest channel up or down
}





