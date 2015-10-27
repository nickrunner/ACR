/*
 *ACR_Main.c
 *
 * Created: 10/9/2014 1:28:37 PM
 * Author: Nick Schrock, Kevin Sager
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

//Alarm Addresses
#define ALARM_HOUR_ADDR 46
#define ALARM_MINUTE_ADDR 12 
#define ALARM2_HOUR_ADDR 20
#define ALARM2_MINUTE_ADDR 23
#define ALARM2_TONE_ADDR 30
#define ALARM_TONE_ADDR 31
#define INITIAL_ALARM_HOUR 1
#define INITIAL_ALARM_MINUTE 0

//Radio Preset Addresses
#define CHANNEL1_ADDR1 1
#define CHANNEL1_ADDR2 2
#define CHANNEL2_ADDR1 3
#define CHANNEL2_ADDR2 4
#define CHANNEL3_ADDR1 5
#define CHANNEL3_ADDR2 6
#define CHANNEL4_ADDR1 7
#define CHANNEL4_ADDR2 8
#define CHANNEL5_ADDR1 9
#define CHANNEL5_ADDR2 10

#define TIMEOUT_SECONDS 60
#define MAX_CHANNEL 1079
#define MIN_CHANNEL 879

#include <stdbool.h>
#include <avr/eeprom.h>
#include <util/delay.h>

static uint16_t b1_state = 0;
static uint16_t b2_state = 0;
static uint16_t b3_state = 0;
static int b1_flag = 0;
static int b2_flag = 0;
static int b3_flag = 0;
static int b3_hold = 0;
static int b3_hold_state = 0;
static int b1_hold = 0;
static int b1_hold_state = 0;
static int b2_hold = 0;
static int b2_hold_state = 0;
static int b1_count = 0;
static int b2_count = 0;
static char State = 'I';
static int alarm = 0;
static int alarm2 = 0;
static int set_alarm2;
static int timeout = 0;
static float volume = 10;
static uint16_t preset[5];
static int months[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

//Initialize Timer 0 for 5 ms Push-Button Debounce
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

//Reads from EEPROM
uint8_t read_eeprom(int addr){
	uint8_t byte ;
	byte = eeprom_read_byte ((uint8_t*)addr) ;
	
	return byte;
}


//Writes to EEPROM
void write_RadioStation(uint16_t word, int addr1, int addr2){
	uint8_t low_byte = word & 0x00FF;
	uint8_t high_byte = word >> 8;
	eeprom_update_byte (( uint8_t*)addr1, high_byte );
	eeprom_update_byte (( uint8_t*)addr2, low_byte );
}

uint16_t read_RadioStation(int addr1, int addr2){
	uint8_t high_byte;
	uint8_t low_byte;
	high_byte = eeprom_read_byte((uint8_t*)addr1);
	low_byte = eeprom_read_byte((uint8_t*)addr2);
	uint16_t word = high_byte << 8;
	word |= low_byte;
	return word;
}


//Writes to EEPROM
void write_eeprom(uint8_t byte, int addr){
	eeprom_update_byte (( uint8_t*)addr, byte );
}

void write_presets(){
	write_RadioStation(preset[0], CHANNEL1_ADDR1, CHANNEL1_ADDR2);
	write_RadioStation(preset[1], CHANNEL2_ADDR1, CHANNEL2_ADDR2);
	write_RadioStation(preset[2], CHANNEL3_ADDR1, CHANNEL3_ADDR2);
	write_RadioStation(preset[3], CHANNEL4_ADDR1, CHANNEL4_ADDR2);
	write_RadioStation(preset[4], CHANNEL5_ADDR1, CHANNEL5_ADDR2);
}

void display_presets(){
	LCD_GoTo(0,0);
	//LCD_WriteString("Presets");
	for(int i=0; i<5; i++){
		LCD_GoTo(0,i+1);
		//LCD_WriteInt(i+1,false);
		//LCD_GoTo(6,i+1);
		//LCD_WriteChar(')');
		FM_DisplayChannel(preset[i], 0, i+1, false);
	}
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
	DDRB |= _BV(1);		//Alarm PWM OUT
	DDRC &= ~_BV(2);	//Photocell ADC IN
	DDRC &= ~_BV(3);	//Slide Switch
	DDRC |= _BV(1);		//LED
	DDRC |= _BV(0);		//LED
	DDRD &= ~_BV(4);	//Slide Switch
	DDRD &= ~_BV(6);	//Button
	DDRD &= ~_BV(5);	//Button
	DDRD &= ~_BV(7);	//Button
	DDRD |= _BV(3);		//Backlight Adjust PWM OUT
	DDRD |= _BV(2);		//RST FM_Chip
	PORTD |= _BV(6);
	PORTD |= _BV(5);
	PORTD |= _BV(7);
	PORTD |= _BV(3);
	PORTD |= _BV(2);
	PORTC |= _BV(3);
	PORTD |= _BV(4);
	PORTC |= _BV(1);
	PORTC |= _BV(0);
	
	//Initialize all peripherals
	timer0_init();
	timer1_init();
	timer2_init();
	WDT_Init();
	ADC_init();
	SPI_Master_Init();
	I2C_Init();
	LCD_Init();
	FM_Init();
	
	//Initialize time to 2:00PM and enable interrupts
	set_Time(2,0,40);
	printf("Time set\n");
	set_Date(11,5,14);
	write_eeprom(INITIAL_ALARM_HOUR, ALARM_HOUR_ADDR);
	write_eeprom(INITIAL_ALARM_HOUR, ALARM2_HOUR_ADDR);
	write_eeprom(INITIAL_ALARM_MINUTE, ALARM2_MINUTE_ADDR);
	write_eeprom(INITIAL_ALARM_MINUTE, ALARM_MINUTE_ADDR);
	write_eeprom(0x01, ALARM_TONE_ADDR);
	write_eeprom(0x01, ALARM2_TONE_ADDR);
	sei();
}

//Push Button Interrupt Handler
ISR(TIMER0_COMPA_vect)
{
	b3_state = (b3_state<<1) | (PIND & (1<<PD6)) >> 6 | 0xf800;
	b2_state = (b2_state<<1) | (PIND & (1<<PD5)) >> 5 | 0xf800;
	b1_state = (b1_state<<1) | (PIND & (1<<PD7)) >> 7 | 0xf800;
	
	b3_hold_state = (b3_state<<1) | (PIND & (1<<PD6)) >> 6 | 0xfc00;
	b2_hold_state = (b2_state<<1) | (PIND & (1<<PD5)) >> 5 | 0xfc00;
	b1_hold_state = (b1_state<<1) | (PIND & (1<<PD7)) >> 7 | 0xfc00;
	
	//B1 pushed
	if(b1_state==0xfc00){
		b1_flag = 1;			
	}
	
	if(b1_hold_state==0xfc00){
		b1_hold++;
	}	
	else{
		b1_hold = 0;
	}
		
	//B2 pushed
	if(b2_state==0xfc00){
		b2_flag = 1;
	}
	
	if(b2_hold_state==0xfc00){
		b2_hold++;
	}
	else{
		b2_hold = 0;
	}
	
	//B3 Pushed
	if(b3_state==0xfc00){
		b3_flag = 1;
	}
	if(b3_hold_state==0xfc00){
		b3_hold++;
	}
	else{
		b3_hold = 0;
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
	uint8_t hour;
	uint8_t minute;
	uint8_t tone;
	
	//Initialize alarm time, alarm and time-out
	if(alarm2){
		hour = read_eeprom(ALARM2_HOUR_ADDR);
		minute = read_eeprom(ALARM2_MINUTE_ADDR);
		tone = read_eeprom(ALARM2_TONE_ADDR);
	}
	else{
		hour = read_eeprom(ALARM_HOUR_ADDR);
		minute = read_eeprom(ALARM_MINUTE_ADDR);
		tone = read_eeprom(ALARM_TONE_ADDR);
	}
	alarm = 1;
		
	bool pm = false;
	int displayed_hour = hour;
	
	if(hour>12){
		displayed_hour -= 12;
	}
	if(hour >= 12){
		pm = true;
	}

	while(timeout < TIMEOUT_SECONDS/4){
		
		//Clear button flags
		b2_flag = 0;
		b1_flag = 0;
		b3_flag = 0;
		
		//Draw Alarm time and flash
		alarm_Display(minute, displayed_hour, 13, 2);
		if(tone == 1){
			LCD_DrawBell(3,35);
		}
		else{
			LCD_DrawNote(3,35);
		}
		if(pm){
			LCD_GoTo(70,2);
			LCD_WriteString("  ");
			LCD_GoTo(70,2);
			LCD_WriteString("PM");
		}
		else{
			LCD_GoTo(70,2);
			LCD_WriteString("  ");
			LCD_GoTo(70,2);
			LCD_WriteString("AM");
		}
		LCD_GoTo(5,5);
		LCD_WriteString("Set Alarm");
		_delay_ms(100);
		
		//Set minute with PB1
		if(b1_flag==1){
			minute++;
			if(minute==60)
				minute=0;
			timeout=0;
			b1_flag=0;
		}
		
		while(b1_hold > 100){
			alarm_Display(minute, displayed_hour, 13, 2);
			if(tone){
				LCD_DrawBell(3,35);
			}
			else{
				LCD_DrawNote(3,35);
			}
			if(pm){
				LCD_GoTo(70,2);
				LCD_WriteString("  ");
				LCD_GoTo(70,2);
				LCD_WriteString("PM");
			}
			else{
				LCD_GoTo(70,2);
				LCD_WriteString("  ");
				LCD_GoTo(70,2);
				LCD_WriteString("AM");
			}
			LCD_GoTo(5,5);
			LCD_WriteString("Set Alarm");
			minute++;
			if(minute==60)
			minute=0;
			timeout=0;
			b1_flag=0;
			_delay_ms(50);
		}
		
		//Set hour with PB2
		if(b2_flag==1){
			hour++;
			displayed_hour = hour;
			if(hour>12){
				displayed_hour -= 12;
			}
			if(displayed_hour == 12){
				pm ^= true;
			}
			if(hour==24){
				hour=0;
			}
			timeout=0;
			b2_flag = 0;
		}
		while(b2_hold > 100){
			alarm_Display(minute, displayed_hour, 13, 2);
			if(tone){
				LCD_DrawBell(3,35);
			}
			else{
				LCD_DrawNote(3,35);
			}
			if(pm){
				LCD_GoTo(70,2);
				LCD_WriteString("  ");
				LCD_GoTo(70,2);
				LCD_WriteString("PM");
			}
			else{
				LCD_GoTo(70,2);
				LCD_WriteString("  ");
				LCD_GoTo(70,2);
				LCD_WriteString("AM");
			}
			LCD_GoTo(5,5);
			LCD_WriteString("Set Alarm");
			hour++;
			displayed_hour = hour;
			if(hour>12){
				displayed_hour -= 12;
			}
			if(displayed_hour == 12){
				pm ^= true;
			}
			if(hour==24){
				hour=0;
			}
			timeout=0;
			b2_flag = 0;
			_delay_ms(100);
		}
		
		while(b3_hold != 0){
			if(b3_hold > 38){
				tone ^= 1;
				if(tone){
					LCD_DrawBell(3,35);
				}
				else{
					LCD_DrawNote(3,35);
				}
				_delay_ms(1000);
				b3_flag = 0;
			}
			b3_hold = 0;
			_delay_ms(200);
		}
		
		if(b3_flag == 1){
			b3_flag = 0;
			break;
		}
	}
	
	//Initialize alarm and go back to Idle state
	reset_variables();
	if(alarm2){
		write_eeprom(hour, ALARM2_HOUR_ADDR);
		write_eeprom(minute, ALARM2_MINUTE_ADDR);
		write_eeprom(tone, ALARM2_TONE_ADDR);
		alarm2_Set(hour,minute,0);
		alarm2=0;
	}
	else{
		write_eeprom(hour, ALARM_HOUR_ADDR);
		write_eeprom(minute, ALARM_MINUTE_ADDR);
		write_eeprom(tone, ALARM_TONE_ADDR);
		alarm_Set(hour, minute, 0);
	}
	
	State = 'I';
}

//Check to see if alarm should be going off
void check_Alarm(){
	if(alarm_Check()==1){
		alarm_Sound(volume);
	}
}

//Idle state displaying Large time
//TODO: Get a small date, station frequency, and temperature on here
//I doubt it will all fit so we may have to a scrolling row of this info. 
void idle_state(){
	RTC_DisplayLargeClock(13,2);
	RTC_DisplayDate(13,3);
	if(FM_GetRadioState()){
		FM_DisplayChannel(FM_GetChannel(), 40, 5, false);
		FM_DisplaySignalStrength(70,5);
	}
	else{
		LCD_GoTo(40,5);
		LCD_WriteString("      ");
	}
	if(alarm==1 && !(PINC & _BV(3))){
		if(read_eeprom(ALARM_TONE_ADDR)==1){
			LCD_DrawBell(5,0);
		}
		else{
			LCD_DrawNote(5,0);
		}
	}
	else{
		LCD_GoTo(0,5);
		LCD_WriteString("  ");
	}
	if(set_alarm2==1 && !(PIND & _BV(4))){
		if(read_eeprom(ALARM2_TONE_ADDR)==1){
			LCD_DrawBell(5,14);
		}
		else{
			LCD_DrawNote(5,14);
		}
	}
	else{
		LCD_GoTo(14,5);
		LCD_WriteString("  ");
	}
	
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
	
	while(timeout<TIMEOUT_SECONDS/4){
		
		//Reset Button Flags every loop
		b1_flag = 0;
		b2_flag = 0;
		b3_flag = 0;
		
		_delay_ms(100);
		
		//Increment button counts on button flag
		if(b1_flag==1){
			timeout=0;
			b1_count++;
		}
		if(b2_flag==1){
			timeout=0;
			b2_count++;
		}
		if(b3_flag == 1){
			b3_flag=0;
			State = 'I';
			break;
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
				alarm2 = 1;
				set_alarm2=1;
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
				b1_count = 0;
				loop++;
			}

		}
		if(b1_count == 0 && loop>0){
			if(b2_flag == 1){
				State = 'D';
				break;
			}
		}
	check_Alarm();
	}
	
	//Reset all button counts and flags
	reset_variables();
}

//Allows user to set time.  A lot of code is borrowed from the set alarm state
void set_time_state(){
	//Initialize alarm time, alarm and time-out
	int hour = get_Hours();
	if(RTC_GetPM()){
		hour += 12;
	}
	if(hour == 24){
		hour=0;
	}
	int minute = get_Minutes();
	int displayed_hour = get_Hours();
	
	while(timeout < TIMEOUT_SECONDS/4){
		
		//Draw Time 
		alarm_Display(minute, displayed_hour, 13, 2);
		LCD_GoTo(25,4);
		LCD_WriteString("Set Time");
		_delay_ms(100);
		
		//Set minute with PB1
		if(b1_flag==1){
			minute++;
			if(minute==60)
			minute=0;
			timeout=0;
			b1_flag=0;			
		}	
		
		while(b1_hold > 100){
			alarm_Display(minute, displayed_hour, 13, 2);
			LCD_GoTo(25,4);
			LCD_WriteString("Set Time");
			minute++;
			if(minute==60)
				minute=0;
			timeout=0;
			b1_flag=0;
			_delay_ms(50);			
		}	
		
		//Set hour with PB2
		if(b2_flag==1){;
			hour++;
			displayed_hour = hour;
			if(hour>12){
				displayed_hour -= 12;
			}
			if(hour == 24){
				hour=0;
			}			
			timeout=0;
			b2_flag = 0;
			set_Time(hour, minute, 0x00);
		}
		while(b2_hold > 100){
			alarm_Display(minute, displayed_hour, 13, 2);
			LCD_GoTo(25,4);
			LCD_WriteString("Set Time");
			hour++;
			displayed_hour = hour;
			if(hour>12){
				displayed_hour -= 12;
			}
			if(hour == 24){
				hour=0;
			}
			timeout=0;
			b2_flag = 0;
			set_Time(hour, minute, 0x00);
			get_Hours();
			_delay_ms(100);
		}
		
		if(b3_flag ==1){
			b3_flag = 0;
			break;
		}
		check_Alarm();
	}
	
	//Initialize time and go back to Idle state
	reset_variables();
	set_Time(hour, minute, 0x00);
	State = 'I';
}

//TODO: Make this state functional
void set_date_state(){
	int day = get_Day();
	int month = get_Month();
	int year = get_Year();
	
	LCD_Clear();
	
	while(timeout<TIMEOUT_SECONDS/4){
		RTC_DisplayDate(13,2);
		LCD_GoTo(25,4);
		LCD_WriteString("Set Date");
		
		//Set day with PB1
		if(b1_flag==1){
			day++;
			if(day>months[month-1])
				day=1;
			timeout=0;
			set_Date(month,day,year);
			b1_flag=0;
		}
		
		while(b1_hold > 100){
			RTC_DisplayDate(13,2);
			LCD_GoTo(25,4);
			LCD_WriteString("Set Date");
			day++;
			if(day>months[month-1])
				day=1;
			timeout=0;
			set_Date(month,day,year);
			b1_flag=0;
			_delay_ms(100);
		}
		
		//Set month with PB2
		if(b2_flag==1){
			month++;
			if(month==13)
				month=1;
			timeout=0;
			set_Date(month,day,year);
			b2_flag=0;
		}
		
		while(b2_hold > 100){
			RTC_DisplayDate(13,2);
			LCD_GoTo(25,4);
			LCD_WriteString("Set Date");
			month++;
			if(month==13)
				month=1;
			timeout=0;
			set_Date(month,day,year);
			b2_flag=0;
			_delay_ms(100);
		}
		if(b3_flag == 1){
			b3_flag = 0;
			break;
		}
		check_Alarm();
	}
	reset_variables();
	State = 'I';
}

//Still in its infant stages
//TODO: Make this state functional
void set_radio_state(){
	uint16_t channel = FM_GetChannel();
	if(!FM_GetRadioState())
		FM_PowerOn();
	if(channel < MIN_CHANNEL || channel > MAX_CHANNEL){
		channel = MIN_CHANNEL;
	}
	b1_flag = 0;
	b2_flag = 0;
	b3_flag = 0;
	LCD_Clear();
	FM_DisplayChannel(channel,35,2, true);
	FM_TuneToChannel(channel);
	timeout = 0;
	while(timeout<TIMEOUT_SECONDS/4){
		//printf("In Loop\n");
		display_presets();
		FM_DisplayChannel(channel, 35,2, true);
		if(b1_flag == 1){
			channel += 2;
			if(channel < MIN_CHANNEL || channel > MAX_CHANNEL){
				channel = MIN_CHANNEL;
			}	
			display_presets();					
			FM_DisplayChannel(channel, 35,2, true);
			FM_TuneToChannel(channel);
			timeout = 0;
			b1_flag = 0;
		}

		while(b1_hold > 100){
			channel += 2;
			if(channel < MIN_CHANNEL || channel > MAX_CHANNEL){
				channel = MIN_CHANNEL;
			}
			display_presets();
			FM_DisplayChannel(channel, 35,2, true);
			timeout = 0;
			b1_flag = 0;
			_delay_ms(100);
		}
		
		//Set preset if button held
		while(b2_hold != 0){
			if(b2_hold > 38){
				for(int i=4; i>0; i--){
					preset[i] = preset[i-1];
				}
				preset[0] = channel;
				write_presets();
				display_presets();
				_delay_ms(1000);
				b2_flag = 0;
			}
			b2_hold = 0;
			_delay_ms(200);
		}
		
		if(b2_flag == 1){
			FM_Seek();
			timeout = 0;
			channel = FM_GetChannel();
			b2_flag = 0;
		}
		//Add preset if B2 is held

		if(b3_flag == 1){
			b3_flag = 0;
			break;
		}
	}
	
	write_RadioStation(channel, CHANNEL1_ADDR1, CHANNEL1_ADDR2);
	reset_variables();
	State = 'I';
}

void preset_select(int num){
	if(num==0){
		FM_TuneToChannel(read_RadioStation(CHANNEL1_ADDR1, CHANNEL1_ADDR2));
	}
	if(num==1){
		FM_TuneToChannel(read_RadioStation(CHANNEL2_ADDR1, CHANNEL2_ADDR2));
	}
	if(num==2){
		FM_TuneToChannel(read_RadioStation(CHANNEL3_ADDR1, CHANNEL3_ADDR2));
	}
	if(num==3){
		FM_TuneToChannel(read_RadioStation(CHANNEL4_ADDR1, CHANNEL4_ADDR2));
	}
	if(num==5){
		FM_TuneToChannel(read_RadioStation(CHANNEL5_ADDR1, CHANNEL5_ADDR2));
	}
}

void alarm_Sound(float volume){
	//Alarm 1 is ringing
	if(volume>90){
		volume = 90;
	}
	if(get_Alarm_Minute() == get_Minutes()){
		set_AlarmRing(1);
		if(read_eeprom(ALARM_TONE_ADDR)){
			OCR1A = (volume/100)*ICR1;
		}
		else{
			FM_PowerOn();
		}
	}
	//Alarm 2 is ringing
	else{
		set_Alarm2Ring(1);
		if(read_eeprom(ALARM2_TONE_ADDR)){
			OCR1A = (volume/100)*ICR1;
		}
		else{
			FM_PowerOn();
		}
	}

	LCD_Clear();
	
	timeout = 0;
	while(timeout<TIMEOUT_SECONDS/4){
		RTC_DisplayLargeClock(13,2);
		RTC_DisplayDate(13,3);
		_delay_ms(300);
		reset_variables();
		_delay_ms(200);  
		if(b1_flag){
			alarm_Silence();
			alarm_Set(read_eeprom(ALARM_HOUR_ADDR), read_eeprom(ALARM_MINUTE_ADDR),0);
			alarm2_Set(read_eeprom(ALARM2_HOUR_ADDR), read_eeprom(ALARM2_MINUTE_ADDR),0);
			b1_flag = 0;
			break;
		} 
		if(b2_flag){
			alarm_Snooze();
			b2_flag = 0;
			break;
		}
	}
	OCR1A=0;
	set_Alarm2Ring(0);
	set_AlarmRing(0);
	reset_variables();
	State = 'I';
}

int main(void){
	
	//Initialize UART communication with computer
	USART_Init(MYUBRR);
	stdout = &uart_output;
	stdin = &uart_input;
	printf("START\n");
	setup();

	while(1){
		
		if(b1_flag==1){
			State = 'M';
		}
		if(b2_flag==1){
			b2_flag=0;
			if(FM_GetRadioState()){
				preset_select(b2_count);
				b2_count++;
				if(b2_count == 5){
					b2_count = 0;
				}
			}
		}
		if(b3_flag){
			if(FM_GetRadioState())
				FM_PowerDown();
			else{
				FM_PowerOn();
			}
			b3_flag = 0;
		}

		switch(State)
		{
			case 'I':
			idle_state();
			break;

			case 'M':
			timeout = 0;
			menu_state();
			break;

			case 'A':
			timeout = 0;
			set_alarm_state();
			break;

			case 'T':
			timeout = 0;
			set_time_state();
			break;

			case 'R':
			timeout = 0;
			set_radio_state();
			break;

			case 'D':
			timeout = 0;
			set_date_state();
			break;
		}

		check_Alarm();
		backlight_adjust();
		
	}

	return 0;

}