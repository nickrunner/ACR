
//TWI master control header definitions
//Written with reference to http://w8bh.net/avr/AvrDS1307.pdf
//Author: Nick Schrock, Kevin Sager



#define TW_START	0xA4
#define TW_READY	(TWCR & 0x80)	//Check if TWINT bit in TWCR control register is high
#define TW_STATUS	(TWSR & 0xF8)	//returns value of status register
#define DS1307		0xD0			//references address of RTC (DS1307)
#define TW_SEND		0x84			//sets TWINT and TWEN (basically start sending data)
#define TW_STOP		0x94			//sets TWINT, TWSTO, TWEN (sets stop condition)
#define TW_NACK		0x84
#define READ		1

void I2C_WriteRegister(uint8_t device_address, uint8_t deviceRegister, uint8_t data);
uint8_t I2C_ReadRegister(uint8_t device_address, uint8_t deviceRegister);
void I2C_Init();
uint8_t I2C_Start();
uint8_t I2C_SendAddress(uint8_t address);
uint8_t I2C_Write (uint8_t data);
void I2C_Stop();
uint8_t I2C_ReadNACK();
uint16_t I2C_ReadRegister_16bit();
void I2C_WriteRegister_16bit(uint16_t data);
void I2C_StartWait(unsigned char address);
uint8_t I2C_ReadACK();
