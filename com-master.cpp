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
		Wire.endTransmission();
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
void ComMaster::writeByteToActiveChannel(u8 b)
{
	if (slave_mode == SLAVE_MODE_SPI)
	{
		Spi_WriteByteToActiveChannel(b);
	}
	else // I2C
	{
		I2c_WriteByteToActiveChannel(b);
	}
}

/***********************************************
* Send 1 byte over serial link (SPI or I2C)
************************************************/
void ComMaster::write1(u8 b1)
{
	bytes_to_send = 1;
	beginTx();
	writeByteToActiveChannel(b1);
	endTx();
}

/***********************************************
* Send 2 bytes over serial link (SPI or I2C)
************************************************/
void ComMaster::write2(u8 b1, u8 b2)
{
	bytes_to_send = 2;
	beginTx();
	writeByteToActiveChannel(b1);
	writeByteToActiveChannel(b2);
	endTx();
}

/***********************************************
* Send 3 bytes over serial link (SPI or I2C)
************************************************/
void ComMaster::write3(u8 b1, u8 b2, u8 b3)
{
	bytes_to_send = 3;
	beginTx();
	writeByteToActiveChannel(b1);
	writeByteToActiveChannel(b2);
	writeByteToActiveChannel(b3);
	endTx();
}

/***********************************************
* Send 4 bytes over serial link (SPI or I2C)
************************************************/
void ComMaster::write4(u8 b1, u8 b2, u8 b3, u8 b4)
{
	bytes_to_send = 4;
	beginTx();
	writeByteToActiveChannel(b1);
	writeByteToActiveChannel(b2);
	writeByteToActiveChannel(b3);
	writeByteToActiveChannel(b4);
	endTx();
}

/***********************************************
*
* Send N bytes over serial link (SPI or I2C)
*
************************************************/
void ComMaster::writeN(u8 * data, int len)
{
	bytes_to_send = len;
	beginTx();
	for (int i = 0; i < len; i++)
	{
		writeByteToActiveChannel(data[i]);
	}
	endTx();
}

/***********************************************
*
* Send 1 byte end receives n bytes
*
************************************************/
void ComMaster::write1ReadN(u8 b1, u8* readData, int readLen)
{
	bytes_to_send = 1;
	beginTx();
	writeByteToActiveChannel(b1);
	beginRx();
	ReadAfterWrite(readData, readLen);
	endRx();
}

/***********************************************
*
* Send 2 bytes end receives n bytes
*
************************************************/
void ComMaster::write2ReadN(u8 b1, u8 b2, u8* readData, int readLen)
{
	bytes_to_send = 2;
	beginTx();
	writeByteToActiveChannel(b1);
	writeByteToActiveChannel(b2);
	beginRx();
	ReadAfterWrite(readData, readLen);
	endRx();
}

/***********************************************
*
* Send 3 bytes end receives n bytes
*
************************************************/
void ComMaster::write3ReadN(u8 b1, u8 b2, u8 b3, u8* readData, int readLen)
{
	bytes_to_send = 3;
	beginTx();
	writeByteToActiveChannel(b1);
	writeByteToActiveChannel(b2);
	writeByteToActiveChannel(b3);
	beginRx();
	ReadAfterWrite(readData, readLen);
	endRx();
}

/***********************************************
*
* Send 5 bytes end receives n bytes
*
************************************************/
void ComMaster::write5ReadN(u8 b1, u8 b2, u8 b3, u8 b4, u8 b5, u8* readData, int readLen)
{
	bytes_to_send = 5;
	beginTx();
	writeByteToActiveChannel(b1);
	writeByteToActiveChannel(b2);
	writeByteToActiveChannel(b3);
	writeByteToActiveChannel(b4);
	writeByteToActiveChannel(b5);
	beginRx();
	ReadAfterWrite(readData, readLen);
	endRx();
}

/***********************************************
*
* Send a write command and then receives data back
*
************************************************/
void ComMaster::writeNReadN(u8 *writeData, int writeLen, u8* readData, int readLen)
{
	bytes_to_send = writeLen;
	beginTx();
	for (int i = 0; i < writeLen; i++)
	{
		writeByteToActiveChannel(writeData[i]);
	}
	beginRx();
	ReadAfterWrite(readData, readLen);
	endRx();
}

/***********************************************
* Receives data from serial link
* (even if no write operation called before)
************************************************/
void ComMaster::readN(u8* data, int len)
{
	if (slave_mode == SLAVE_MODE_SPI)
	{
		Spi_EnableAndBeginRx();
	}
	else
	{
		// no action needed
	}
	ReadAfterWrite(data, len);
	endRx();
}

/***********************************************
* Receives data from serial link
* (write operation must be issued before calling this funtion)
************************************************/
void ComMaster::ReadAfterWrite(u8* data, int len)
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
		I2c_Read(data, len, DEFAULT_I2C_TIMEOUT);
	}
}
