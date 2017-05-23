/*
 * request.cpp
 *
 *  Created on: 22. 5. 2017
 *      Author: jiri svrcek
 */


#include "request.h"
#include "global.h"
#include "parser.h"
#include "led.h"
#include "com-master.h"
#include <application.h>

//extern TDongleState dongleState;
extern ComMaster comMaster;

u8 readData[1024 + NR_OF_DUMMY_BYTES];

void StdProtocolRequest::clear(void)
{
	writeIndex = 0;
	readIndex = 0;
	completed = false;
	msgState = WAITING_START_BYTE;
}

void StdProtocolRequest::addByte(u8 newByte)
{
	switch (msgState)
	{
	case WAITING_START_BYTE:
		if (newByte == STD_PROTOCOL_START_BYTE)
			msgState = WAITING_MSG_LEN_LSB;
		break;

	case WAITING_MSG_LEN_MSB:
		declaredLength = newByte << 8;
		msgState = WAITING_MSG_LEN_LSB;
		break;

	case WAITING_MSG_LEN_LSB:
		declaredLength += newByte;
		msgState = WAITING_MSG_CNT_MSB;
		break;

	case WAITING_MSG_CNT_MSB:
		msgCounter = newByte << 8;
		msgState = WAITING_MSG_CNT_LSB;
		break;

	case WAITING_MSG_CNT_LSB:
		msgCounter += newByte;
		msgState = WAITING_MSG_BODY;
		receivedLength = 5;
		writeIndex = 0;
		break;

	case WAITING_MSG_BODY:
		if (receivedLength < declaredLength)
		{
			if (writeIndex < REQUEST_MSG_MAX_SIZE)
			{
				data[writeIndex++] = newByte;
				receivedLength++;
			}
		}
		else // waiting END byte
		{
			if (newByte == STD_PROTOCOL_END_BYTE)
			{
				msgState = WAITING_START_BYTE;
				completed = true;
			}
			else // no END byte, msg corrupted, restart state machine
			{
				clear();
				//msgState = WAITING_START_BYTE;
			}
		}
		break;

	default:
		clear();
		//msgState = WAITING_START_BYTE;
		break;
	}
}

void StdProtocolRequest::processNew(TDongleState* dongleState)
{
	ProtocolAction action = (ProtocolAction)dequeue16(NUMBER_FORMAT_BIG_ENDIAN);
	reply->init(masterInterface, msgCounter);

	switch ((u16)action)
	{
	case ACTION_WRITE:
	{
		u8 * writeData = data;
		const int writeLen = writeIndex - 2; // payload size without Action size
		comMaster.writeN(writeData, writeLen);
		reply->sendWriteStatus(0);
		break;
	}
	case ACTION_READ:
	{
		const int readLen = getReadLength();
		comMaster.readN(readData, readLen);
		reply->setNrOfDummyBytes(nrOfDummyBytes);
		reply->send();
		break;
	}
	case ACTION_WRITE_READ:
		break;

	case ACTION_GET_VERSION:
		break;
	}
	completed = false;
}

u8 StdProtocolRequest::dequeue8(void)
{
	if (readIndex < REQUEST_MSG_MAX_SIZE)
		return data[readIndex++];
	return 0;
}

u16 StdProtocolRequest::dequeue16(EndianFormat endianFormat)
{
	if (readIndex > REQUEST_MSG_MAX_SIZE)
		return 0;
	readIndex += 2;
	if (endianFormat == NUMBER_FORMAT_LITTLE_ENDIAN)
	{
		return (data[readIndex-2] << 8 ) || data[readIndex-1];
	}
	else
	{
		return data[readIndex-2] || (data[readIndex-1] << 8);
	}
}

u32 StdProtocolRequest::dequeue32(EndianFormat endianFormat)
{
	if (readIndex+2 > REQUEST_MSG_MAX_SIZE)
		return 0;
	readIndex += 4;
	if (endianFormat == NUMBER_FORMAT_LITTLE_ENDIAN)
	{
		return (data[readIndex-4] << 24) || (data[readIndex-3] << 16) || (data[readIndex-2] << 8) || (data[readIndex-1]);
	}
	else
	{
		return (data[readIndex-4]) || (data[readIndex-3] << 8) || (data[readIndex-2] << 16) || (data[readIndex-1] << 24);
	}
}

u16 StdProtocolRequest::getReadLength(void)
{
	return 0;
}

ProtocolAction StdProtocolRequest::getAction(void)
{
	return (ProtocolAction) (data[ACTION_POS] + (data[ACTION_POS+1] >> 8));
}
