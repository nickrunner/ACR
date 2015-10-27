/*
 * FM_Driver.c
 *
 * Created: 10/9/2014 1:28:37 PM
 * Author: Nick Schrock, Kevin Sager
 * Written in reference to https://www.sparkfun.com/products/11083
 */ 
#include <stdbool.h>
#define SI4703 (0x10<<1)

//Device Registers
#define DEVICEID 0x00
#define CHIPID  0x01
#define POWERCFG  0x02
#define CHANNEL  0x03
#define SYSCONFIG1  0x04
#define SYSCONFIG2  0x05
#define STATUSRSSI  0x0A
#define READCHAN  0x0B
#define RDSA  0x0C
#define RDSB  0x0D
#define RDSC  0x0E
#define RDSD  0x0F

//Register 0x02 - POWERCFG
#define SMUTE  15
#define DMUTE  14
#define SKMODE  10
#define SEEKUP  9
#define SEEK  8

//Register 0x03 - CHANNEL
#define TUNE  15

//Register 0x04 - SYSCONFIG1
#define RDS  12
#define DE  11

//Register 0x05 - SYSCONFIG2
#define SPACE1  5
#define SPACE0  4

//Register 0x0A - STATUSRSSI
#define RDSR  15
#define STC  14
#define SFBL  13
#define AFCRL  12

#define STEREO  8

#define SCK 5
#define SDA	4
#define RST 2

#define MAX_CHANNEL 1079
#define MIN_CHANNEL 879

void FM_PrintRegisters();
void FM_Init();
void FM_SetVolume(uint16_t level);
void FM_ReadRegisters();
void FM_UpdateRegisters();
//int digitAtPos(int number, int pos);
void FM_DisplayChannel(int channel, int xpos, int ypos, bool large);
int FM_TuneToChannel(int channel);
int FM_ReadChannel();
void FM_Reset();
int FM_GetChannel();
uint8_t FM_GetSignalStrength();
void FM_DisplaySignalStrength(int xpos, int ypos);
void FM_Seek(void);
void FM_PowerOn();
bool FM_GetRadioState();
void FM_PowerDown();