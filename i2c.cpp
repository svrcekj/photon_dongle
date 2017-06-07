/*
 * i2c.cpp
 *
 *  Created on: 27. 4. 2017
 *      Author: jiri svrcek
 */

#include "i2c.h"
#include <application.h>


static int byte_pos = 0;
static int bytes_send = 0;

/***********************************************/
void I2c_BeginTx(void)
/***********************************************/
{
	Wire.beginTransmission(DEV_I2C_ADDRESS);
	byte_pos = 0;
	bytes_send = 0;
}

/***********************************************/
bool I2c_WriteByteToActiveChannel(u8 b, int totalLen)
/***********************************************/
{
	Wire.write(b);

	if (++bytes_send >= totalLen)
	{
		if (Wire.endTransmission(true) != 0) // Terminate transaction with STOP BIT SET
		{
			Wire.reset();
			return false;
		}
	}
	else if (++byte_pos == 32) // Arduino limitation
	{
		byte_pos = 0;
		if (Wire.endTransmission(false) != 0) // Send restart bit to keep connection alive
		{
			Wire.reset();
			return false;
		}
		Wire.beginTransmission(DEV_I2C_ADDRESS);
	}
	return true;
}

/***********************************************/
u32 I2c_Configure(u32 requested_speed)
/***********************************************/
{
	Wire.end();
	u32 safe_speed = (requested_speed < MIN_I2C_SPEED) ? MIN_I2C_SPEED :
					 (requested_speed > MAX_I2C_SPEED) ? MAX_I2C_SPEED :
					 requested_speed;

	Wire.setSpeed(safe_speed);
	if (!Wire.isEnabled())
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
		int chunk = bytes_remaining > WIRING_I2C_CHUNK_LIMIT ? WIRING_I2C_CHUNK_LIMIT : bytes_remaining;
		bool stopBit = bytes_remaining <= WIRING_I2C_CHUNK_LIMIT ? true: false;

		int bytesGot = Wire.requestFrom(DEV_I2C_ADDRESS, chunk, stopBit);

		if (bytesGot == 0)
		{
			Wire.reset();
			return -1;
		}
		while (chunk--)
		{
			if (Wire.available())
			{
				data[wr_index++] = Wire.read();
				bytes_remaining--;
			}
			if (millis() > time_to_end)
			{
				Wire.reset();
				return -1; // exit on timeout
			}
		}
	}
	return wr_index;
}
