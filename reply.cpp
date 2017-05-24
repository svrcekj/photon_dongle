/*
 * reply.cpp
 *
 *  Created on: 22. 5. 2017
 *      Author: jiri svrcek
 */

#include "global.h"
#include "reply.h"
#include "application.h"

extern TCPServer server;
extern TDongleState dongleState;

void StdProtocolReply::init(masterInterface_t ifaceType, u16 msgCounter)
{
	data[0] = START_BYTE;;
	masterInterface = ifaceType;
	setField(COUNTER_POS, msgCounter);
}

void StdProtocolReply::addByte(u8 newByte)
{
	if (writeIndex < (REPLY_MSG_MAX_SIZE-1))
		data[writeIndex++] = newByte;
}

void StdProtocolReply::addData(u8* newData, int len)
{
	while (len--)
		addByte(*newData++);
}

void StdProtocolReply::send(void)
{
	send(data, writeIndex+1);
}

void StdProtocolReply::send(u8 *dataToSent, int len)
{
	finalize();
	if (masterInterface == MASTER_INTERFACE_USB)
	{
		USBSerial1.flush();
		USBSerial1.write(dataToSent, len);
	}
	else if (masterInterface == MASTER_INTERFACE_WIFI)
	{
		int fullChunks = len / dongleState.dataChunkSize;
		for (int i = 0; i < fullChunks; i++)
		{
			server.write(&dataToSent[i * dongleState.dataChunkSize], dongleState.dataChunkSize);
		}
		int lastChunkSize = len % dongleState.dataChunkSize;
		if (lastChunkSize > 0)
		{
			server.write(&dataToSent[fullChunks * dongleState.dataChunkSize], lastChunkSize);
		}
	}
}

void StdProtocolReply::sendWriteStatus(u16 status)
{
	setField(ACTION_POS, (u16) request->getAction());
	setField(ERROR_POS, status);
	writeIndex = PAYLOAD_POS;
	send();
}

void StdProtocolReply::setField(u16 position, u16 field)
{
	data[position] = field >> 8;
	data[position + 1] = field;
}

void StdProtocolReply::finalize(void)
{
	if (writeIndex < REPLY_MSG_MAX_SIZE)
	{
		setField(MSG_SIZE_POS, writeIndex);
		data[writeIndex] = STOP_BYTE;
	}
}

