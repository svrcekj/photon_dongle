/*
 * i2c.h
 *
 *  Created on: 27. 4. 2017
 *      Author: jiri svrcek
 */

#ifndef USER_APPLICATIONS_DONGLE2_I2C_H_
#define USER_APPLICATIONS_DONGLE2_I2C_H_

#include <application.h>

// 7-bit address (0100 1001 b), i.e.:
// 10010010 = 0x92 for I2C write,
// 10010011 = 0x93 for I2C read
#define DEV_I2C_ADDRESS				0x49
#define DEFAULT_I2C_SPEED   		350000	// MBauds
#define MIN_I2C_SPEED	   		 	1000
#define MAX_I2C_SPEED	    		1000000

#define DEFAULT_I2C_TIMEOUT			50 // milliseconds

#define WIRING_I2C_CHUNK_LIMIT		32 // bytes

void I2c_BeginTx(void);
bool I2c_WriteByteToActiveChannel(u8 b, int totalLen, bool sendStopBit);
u32 I2c_Configure(u32 requested_speed);
int I2c_Read(u8* data, int len, int timeout);


#endif /* USER_APPLICATIONS_DONGLE2_I2C_H_ */
