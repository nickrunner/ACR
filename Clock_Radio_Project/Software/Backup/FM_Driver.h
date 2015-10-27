#define SI4703 0x10

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

void FM_PrintRegisters();
void FM_Init();
void FM_SetVolume(uint16_t level);
void FM_ReadRegisters();
void FM_UpdateRegisters();
//int digitAtPos(int number, int pos);
void FM_DisplayChannel(int channel, int xpos, int ypos);
int FM_TuneToChannel(int channel);
int FM_ReadChannel();
void FM_Seek(uint8_t direction);