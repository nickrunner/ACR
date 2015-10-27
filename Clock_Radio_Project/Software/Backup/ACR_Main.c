/*
 *
 * Created: 10/9/2014 1:28:37 PM
 *  Author: Nick Schrock, Kevin Sager
 */ 

/***************************************************************************************
This is the main module for the entire system.  The execution of the code begins here.  
The following functions are executed in the order they are described.  Subsequently, 
the program is thrown into an infinite loop that runs the state machine.  The steps 
taken to initialize the state machine are all completed prior to entrance into the 
infinite loop.

****************************************************************************************/

#include <stdio.h>
#include "uart.h"
#include <string.h>
#include <avr/io.h>
#include "LCD_Driver.h"
#include "SPI_Driver.h"
#include "TWI_Driver.h"
#include "RTC_Driver.h"
#include "Alarm_Driver.h"
#include "avr/interrupt.h"
#include "FM_Driver.h"

#define F_CPU 16000000UL
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#define ALARM_HOUR_ADDR 46
#define ALARM_MINUTE_ADDR 12 
#define INITIAL_ALARM_HOUR 1
#define INITIAL_ALARM_MINUTE 0
#define CHANNEL_ADDR 25

#include <stdbool.h>
#include <avr/eeprom.h>
#include <util/delay.h>

static uint16_t b1_state = 0;
static uint16_t b2_state = 0;
static int b1_flag = 0;
static int b2_flag = 0;
static int b1_count = 0;
static int b2_count = 0;
static char State = 'I';
static int alarm = 0;
static int timeout = 0;
static float volume = 10;

//Initialize Timer 0 for Push-Button Debounce
void timer0_init(void){
	cli();
	TCCR0A |= _BV(WGM01);		//Set to CTC mode
	TIFR0 |= _BV(OCF0A);		//flag to set on compare match
	TCCR0B |= _BV(CS02) | _BV(CS00);		//Prescalar1024
	OCR0A = 78;
	TIMSK0 |= _BV(OCIE0A);
	sei();
}

//Initialize Timer 1 for alarm PWM gen
void timer1_init(void){
	TCCR1A |= _BV(COM1A1) | _BV(WGM11);
	TCCR1B |= _BV(CS12) | _BV(WGM13);
	ICR1 = 3125;
	OCR1A = 0;
}

//Initialize Timer 2 for photocell PWM generation
void timer2_init(void){
	TCCR2A |= _BV(COM2B1) | _BV(COM2B0) | _BV(WGM20);
	TCCR2B |= _BV(WGM22) | _BV(CS22);		//clk/64 prescalar
	OCR2A = 125;
	OCR2B = 62;
}

//Initialize Watchdog Timer
void WDT_Init(void)
{
	//disable interrupts
	cli();
	//set up WDT interrupt
	WDTCSR = (1<<WDCE)|(1<<WDE);
	//Start watchdog timer with 4s prescaller
	WDTCSR = (1<<WDIE)|(1<<WDP3);
	//Enable global interrupts
	sei();
}

//Reads Alarm Time from EEPROM
uint8_t read_alarm_minute(){
	uint8_t alarm_minute ;
	alarm_minute = eeprom_read_byte (( uint8_t *) ALARM_MINUTE_ADDR) ;
	
	return alarm_minute;
}


uint8_t read_alarm_hour(){
	uint8_t alarm_hour;
	alarm_hour = eeprom_read_byte((uint8_t *)ALARM_HOUR_ADDR);
	
	return alarm_hour;
}

//Writes the user set alarm time to EEPROM
void write_alarm_time(uint8_t alarm_hour, uint8_t alarm_minute){
	eeprom_update_byte (( uint8_t *) ALARM_MINUTE_ADDR, alarm_minute );
	eeprom_update_byte (( uint8_t *) ALARM_HOUR_ADDR, alarm_hour );
}

//Reads Radio station from specific address in EEPROM
uint8_t read_radio_station(){
	uint8_t channel;
	channel = eeprom_read_byte((uint8_t *)CHANNEL_ADDR);

	return channel;
}

//Writes radio station to EEPROM
void write_radio_station(uint8_t channel){
	eeprom_update_byte (( uint8_t *) CHANNEL_ADDR, channel );
}


//Set up analog to digital converter in Free-Run mode
//Used for Photocell voltage conversion
void ADC_init(void){
	ADMUX = 0;
	ADMUX |= _BV(REFS0) | _BV(ADLAR) | 2;
	ADCSRA = 0;
	ADCSRA |= _BV(ADEN) | _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2);
	ADCSRB = 0;
}

void setup(void){
	cli();
	
	//Initialize GPIO pins as inputs or outputs
	DDRB |= _BV(1);
	DDRD &= ~_BV(6);
	DDRD &= ~_BV(5);
	DDRD |= _BV(3);
	PORTD |= _BV(6);
	PORTD |= _BV(5);
	PORTD |= _BV(3);
	
	//Initialize all peripherals
	timer0_init();
	timer1_init();
	timer2_init();
	WDT_Init();
	ADC_init();
	SPI_Master_Init();
	I2C_Init();
	LCD_Init();
	//FM_Init();
	//Temp_Init();
	
	//Initialize time to 2:00PM and enable interrupts
	set_Time(0x02, 0x00, 0x00);
	sei();
}

//Push Button Interrupt Handler
ISR(TIMER0_COMPA_vect)
{
	b1_state = (b1_state<<1) | (PIND & (1<<PD6)) >> 6 | 0xfc00;
	b2_state = (b2_state<<1) | (PIND & (1<<PD5)) >> 5 | 0xfc00;
	
	//B1 pushed
	if(b1_state==0xfc00){
		alarm_Silence();
		b1_flag = 1;		
	}
	
	//B2 pushed
	if(b2_state==0xfc00){
		b2_flag = 1;
	}
	
	TIFR0 |= _BV(OCF0A);
	TCNT0 = 0;
}

//Grab ADC conversion
uint8_t get_conversion(void){
	ADCSRA |= _BV(ADIF) | _BV(ADSC);
	while((ADCSRA & ADIF) ==0);
	
	return ADCH;
}

//Watchdog timer interrupt handler
ISR(WDT_vect){
	timeout++;
	WDTCSR |= _BV(WDIE);
}

//Adjust LED's on display 
void backlight_adjust(){
	float sampled_voltage=0;
	float max_voltage = 2.5;
	float duty_cycle = 0.5;
	
	sampled_voltage = get_conversion()*0.0195;
	duty_cycle = (sampled_voltage/max_voltage);
	OCR2B = duty_cycle*OCR2A;
}

//Waits 1 second
void one_sec_delay(void){
	_delay_ms(1000);
}

//Used when jumping out of states to reset all flags, counts and buffers
void reset_variables(){
	RTC_ClearVariables();
	LCD_Clear();
	b1_flag=0;
	b2_flag=0;
	b1_count=0;
	b2_count=0;
}


//TODO: Implement functionality for second alarm set
void set_alarm_state(){
	
	//Initialize alarm time, alarm and time-out
	uint8_t hour = read_alarm_hour();
	uint8_t minute = read_alarm_minute();
	alarm = 1;
	
	while(timeout < 15){
		
		//Clear button flags
		b2_flag = 0;
		b1_flag = 0;
		
		//Draw Alarm time and flash
		alarm_Display(minute, hour, 13, 2);
		_delay_ms(400);
		LCD_Clear();
		_delay_ms(100);
		
		//Set minute with PB1
		if(b1_flag==1){
			minute++;
			if(minute==60)
			minute=0;
			timeout = 0;
		}
		
		//Set hour with PB2
		if(b2_flag==1){
			hour++;
			if(hour==24)
			hour=0;
			timeout = 0;
		}
	}
	
	//Initialize alarm and go back to Idle state
	reset_variables();
	write_alarm_time(hour, minute);
	alarm_Set(hour, minute, 0);
	State = 'I';
}

//Check to see if alarm should be going off
void check_Alarm(){
	if(alarm_Check()==1){
		alarm_Sound(volume);
		printf("%i\n", OCR1A);
	}
}

//Idle state displaying Large time
//TODO: Get a small date, station frequency, and temperature on here
//I doubt it will all fit so we may have to a scrolling row of this info. 
void idle_state(){
	RTC_DisplayLargeClock(13,2);
	if(alarm==1)
		LCD_DrawBell(5, 4);
	check_Alarm();
}

/*Displays all options to user.  User is able to scroll through list using
*PB1. User can enter the selected state by pressing PB2*/

//TODO: Implement watchdog timer user timeouts and third push button to exit ALL states

void menu_state(){
	State = 'I';
	//Draw Menu
	LCD_Clear();
	LCD_GoTo(1, 0);
	LCD_WriteString("Set Radio");
	LCD_GoTo(1,1);
	LCD_WriteString("Set Alarm 1");
	LCD_GoTo(1,2);
	LCD_WriteString("Set Alarm 2");
	LCD_GoTo(1,3);
	LCD_WriteString("Set Time");
	LCD_GoTo(1,4);
	LCD_WriteString("Set Date");
	
	int loop = 0;
	int time_out = 0;
	
	while(time_out<30){
		
		//Reset Button Flags every loop
		b1_flag = 0;
		b2_flag = 0;
		
		_delay_ms(500);
		
		//Increment button counts on button flag
		if(b1_flag==1){
			b1_count++;
		}
		if(b2_flag==1){
			b2_count++;
		}
			
		//Radio Select
		if(b1_count==1){
			if(b1_flag ==1){
				LCD_InvertRow(0);
				if(loop>0){
					LCD_InvertRow(4);
				}
			}
			if(b2_flag == 1){
				State = 'R';
				break;
			}
		}
		
		//Set Alarm 1 Select
		if(b1_count == 2){
			if(b1_flag == 1){
				LCD_InvertRow(0);
				LCD_InvertRow(1);
			}
			if(b2_flag == 1){
				State = 'A';
				break;
			}
		}
		
		//Set Alarm 2 Select
		if(b1_count == 3){
			if(b1_flag ==1){
				LCD_InvertRow(1);
				LCD_InvertRow(2);
			}
			if(b2_flag == 1){
				State = 'A';
				break;
			}
		}
		
		//Time-set Select
		if(b1_count == 4){ 
			if(b1_flag ==1){
				LCD_InvertRow(2);
				LCD_InvertRow(3);
			}
			if(b2_flag == 1){
				State = 'T';
				break;
			}
		}
		
		//Date-set Select
		if(b1_count == 5){
			if(b1_flag ==1){
				LCD_InvertRow(3);
				LCD_InvertRow(4);
			}
			if(b2_flag == 1){
				State = 'D';
				break;
			}
			b1_count = 0;
			loop++;
		}
		
		time_out++;
		//_delay_ms(500);
	}
	
	//Reset all button counts and flags
	reset_variables();

}

//Allows user to set time.  A lot of code is borrowed from the set alarm state
void set_time_state(){
	//Initialize alarm time, alarm and time-out
	int hour = get_Hours();
	int minute = get_Minutes();
	int time_out=0;
	
	while(time_out < 15){
		
		//Clear button flags
		b2_flag = 0;
		b1_flag = 0;
		
		//Draw Time and flash
		alarm_Display(minute, hour, 13, 2);
		_delay_ms(400);
		LCD_Clear();
		_delay_ms(100);
		
		time_out++;
		
		//Set minute with PB1
		if(b1_flag==1){
			minute++;
			if(minute==60)
			minute=0;
			time_out=0;
		}
		
		//Set hour with PB2
		if(b2_flag==1){
			hour++;
			if(hour==24)
			hour=0;
			time_out=0;
		}
	}
	
	//Initialize time and go back to Idle state
	reset_variables();
	printf("%i\n", minute);
	set_Time(hour, minute, 0x00);
	State = 'I';
}

//TODO: Make this state functional
void set_date_state(){
	LCD_GoTo(1,3);
	LCD_WriteString("Date-Set State");
	_delay_ms(5000);
	reset_variables();
	State = 'I';
}

//Still in its infant stages
//TODO: Make this state functional
void set_radio_state(){
	int channel = read_radio_station();
	if(channel < 879 || channel > 107){
		channel = 879;
	}
	int time_out = 0;
	FM_DisplayChannel(channel, 30, 2);
	while(time_out<15){
		b2_flag = 0;
		b1_flag = 0;
		_delay_ms(400);
		if(b1_flag){
			channel++;
			FM_DisplayChannel(channel, 30, 2);
		}
		time_out++;
	}
	printf("%i", FM_TuneToChannel(channel));
	printf("Channel Tune Success!");
	reset_variables();
	State = 'I';
}

int main(void){
	
	//Initialize UART communication with computer
	USART_Init(MYUBRR);
	stdout = &uart_output;
	stdin = &uart_input;

	setup();

	while(1){
		
		if(b1_flag==1)
			State = 'M';

		switch(State)
		{
			case 'I':
			idle_state();
			break;

			case 'M':
			menu_state();
			break;

			case 'A':
			set_alarm_state();
			break;

			case 'T':
			set_time_state();
			break;

			case 'R':
			set_radio_state();
			break;

			case 'D':
			set_date_state();
			break;
		}

		check_Alarm();
		backlight_adjust();
		
	}

	return 0;

}