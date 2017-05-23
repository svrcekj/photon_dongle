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
	setCounterField(msgCounter);
}

void StdProtocolReply::setCounterField(u16 counter)
{
	data[3] = counter;
	data[4] = counter << 8;
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
	send(data+nrOfDummyBytes, writeIndex-nrOfDummyBytes);
}

void StdProtocolReply::send(u8 *dataToSent, int len)
{
	finalize();
	if (masterInterface == MASTER_IFACE_USB)
	{
		USBSerial1.flush();
		USBSerial1.write(dataToSent, len);
	}
	else if (masterInterface == MASTER_IFACE_WIFI)
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
	addField(ACTION_POS, (u16) request->getAction());
	addField(ERROR_POS, status);
	finalize();
	send();
}

void StdProtocolReply::addField(u16 position, u16 field)
{
	data[position] = field;
	data[position + 1] = field >> 8;
}

void StdProtocolReply::finalize(void)
{
	if (writeIndex < REPLY_MSG_MAX_SIZE)
	{
		addField(MSG_SIZE_POS, writeIndex);
		data[writeIndex] = STOP_BYTE;
	}
}

