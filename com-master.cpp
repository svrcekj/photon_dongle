/*
 * device.cpp
 *
 *  Created on: 27. 4. 2017
 *      Author: jiri svrcek
 */

#include "com-master.h"

#include "global.h"
#include "spi.h"
#include "i2c.h"
#include <application.h>

enum {
	NO_STOP_BIT = false
};

/***********************************************/
ComMaster::ComMaster(slave_mode_t mode)
/***********************************************/
{
	slave_mode = mode;
	bytes_to_send = 0;
}

/***********************************************/
void ComMaster::beginTx()
/***********************************************/
{
	if (slave_mode == SLAVE_MODE_SPI)
	{
		Spi_BeginTx();
	}
	else
	{
		I2c_BeginTx();
	}
}

/***********************************************/
void ComMaster::endTx()
/***********************************************/
{
	if (slave_mode == SLAVE_MODE_SPI)
	{
		Spi_EndTx();
	}
	else
	{
		//Wire.endTransmission();
	}
}

/***********************************************/
void ComMaster::beginRx()
/***********************************************/
{
	if (slave_mode == SLAVE_MODE_SPI)
	{
		Spi_BeginRx();
	}
	else
	{
		// no action needed
	}
}

/***********************************************/
void ComMaster::endRx()
/***********************************************/
{
	if (slave_mode == SLAVE_MODE_SPI)
	{
		Spi_EndRx();
	}
	else
	{
		// no action needed
	}
}

/************************************************
* Send one byte to active channel
************************************************/
bool ComMaster::writeByteToActiveChannel(u8 b, bool sendStopBit)
{
	if (slave_mode == SLAVE_MODE_SPI)
	{
		Spi_WriteByteToActiveChannel(b);
		return true;
	}
	else // I2C
	{
		return I2c_WriteByteToActiveChannel(b, bytes_to_send, sendStopBit);
	}
}


/***********************************************
*
* Send N bytes over serial link (SPI or I2C)
*
************************************************/
bool ComMaster::writeN(u8 * data, int len)
{
	bytes_to_send = len;
	beginTx();
	for (int i = 0; i < len; i++)
	{
		if (!writeByteToActiveChannel(data[i]))
			return false;
	}
	endTx();
	return true;
}

/***********************************************
*
* Send a write command and then receives data back
*
************************************************/
bool ComMaster::writeNReadN(u8 *writeData, int writeLen, u8* readData, int readLen)
{
	bytes_to_send = writeLen;
	beginTx();
	for (int i = 0; i < writeLen; i++)
	{
		if (!writeByteToActiveChannel(writeData[i], NO_STOP_BIT))
			return false;
	}
	beginRx();
	ReadAfterWrite(readData, readLen);
	endRx();
	return true;
}

/***********************************************
* Receives data from serial link
* (even if no write operation called before)
************************************************/
bool ComMaster::readN(u8* data, int len)
{
	if (slave_mode == SLAVE_MODE_SPI)
	{
		Spi_EnableAndBeginRx();
	}
	else
	{
		// no action needed
	}
	if (!ReadAfterWrite(data, len))
		return false;
	endRx();
	return true;
}

/***********************************************
* Receives data from serial link
* (write operation must be issued before calling this funtion)
************************************************/
bool ComMaster::ReadAfterWrite(u8* data, int len)
{
	if (slave_mode == SLAVE_MODE_SPI)
	{
#if !USE_DMA
		for (int i = 0; i < len; i++)
		{
			data[i] = Spi_ReadByteFromActiveChannel();
		}
#else
		Spi_ReadByDma(data, len);
#endif
	}
	else // I2C
	{
		if (I2c_Read(data, len, DEFAULT_I2C_TIMEOUT) < 0)
			return false;
	}
	return true;
}
