/*
 * i2c.cpp
 *
 *  Created on: 27. 4. 2017
 *      Author: jiri svrcek
 */

#include "i2c.h"
#include <application.h>


static int i2c_byte_pos = 0;
static int serial_bytes_to_send = 0;

/***********************************************/
void I2c_BeginTx(void)
/***********************************************/
{
	Wire.beginTransmission(DEV_I2C_ADDRESS);
	i2c_byte_pos = 0;
}

/***********************************************/
void I2c_WriteByteToActiveChannel(u8 b)
/***********************************************/
{
	Wire.write(b);
	serial_bytes_to_send--;
	i2c_byte_pos++;
	if (serial_bytes_to_send <= 0) // Arduino limitation
	{
		Wire.endTransmission(true); // Stop bit set to terminate transfer
		return;
	}
	if (i2c_byte_pos >= 32) // Arduino limitation
	{
		Wire.endTransmission(false); // Send restart bit to keep connection alive
		i2c_byte_pos = 0;
	}

}

/***********************************************/
u32 I2c_Configure(u32 requested_speed)
/***********************************************/
{
	u32 safe_speed = (requested_speed < MIN_I2C_SPEED) ? MIN_I2C_SPEED :
					 (requested_speed > MAX_I2C_SPEED) ? MAX_I2C_SPEED :
					 requested_speed;

	Wire.setSpeed(safe_speed);
	Wire.begin();
	return safe_speed;
}

/***********************************************/
int I2c_Read(u8* data, int len, int timeout)
/***********************************************/
{
	int wr_index = 0;
	int bytes_remaining = len;
	system_tick_t time_to_end = millis() + timeout;

	while (bytes_remaining > 0)
	{
		int chunk = min(bytes_remaining, WIRING_I2C_CHUNK_LIMIT);
		bool stopBit = (bytes_remaining == WIRING_I2C_CHUNK_LIMIT) ? true: false;
		Wire.requestFrom(DEV_I2C_ADDRESS, chunk, stopBit);

		while (chunk--)
		{
			if (Wire.available())
			{
				data[wr_index++] = Wire.read();
				bytes_remaining--;
			}
			if (millis() > time_to_end)
				return -1; // exit on timeout
		}
	}
	return wr_index;
}
