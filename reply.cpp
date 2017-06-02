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
	writeIndex = PAYLOAD_POS;
	masterInterface = ifaceType;
	setField(COUNTER_POS, msgCounter);
	setField(ERROR_POS, 0); // No error by default
}

void StdProtocolReply::addByte(u8 newByte)
{
	if (writeIndex < (REPLY_MSG_MAX_SIZE-1))
		data[writeIndex++] = newByte;
}

void StdProtocolReply::addField(u16 field)
{
	if (writeIndex < (REPLY_MSG_MAX_SIZE-2))
	{
		data[writeIndex++] = field >> 8;
		data[writeIndex++] = field;
	}
}

void StdProtocolReply::addField32(u32 field)
{
	if (writeIndex < (REPLY_MSG_MAX_SIZE-4))
	{
		data[writeIndex++] = field >> 24;
		data[writeIndex++] = field >> 16;
		data[writeIndex++] = field >> 8;
		data[writeIndex++] = field;
	}
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
	//setField(ACTION_POS, (u16) request->getAction());
	setField(ERROR_POS, status);
	writeIndex = PAYLOAD_POS;
	send();
}

void StdProtocolReply::sendFwVersion(u16 version)
{
	//setField(ACTION_POS, (u16) request->getAction());
	//setField(ERROR_POS, 0);
	//writeIndex = PAYLOAD_POS;
	addByte(version >> 8);
	addByte(version);
	send();
}

void StdProtocolReply::setField(u16 position, u16 field)
{
	data[position] = field >> 8;
	data[position + 1] = field;
}

void StdProtocolReply::finalize(void)
{
	setField(ACTION_POS, (u16) request->getAction());
	if (writeIndex < REPLY_MSG_MAX_SIZE)
	{
		setField(MSG_SIZE_POS, writeIndex+1);
		data[writeIndex] = STOP_BYTE;
	}
}

void StdProtocolReply::setPayloadData(u8* payload, int len)
{
#if USE_SAFE_COPY
	writeIndex = PAYLOAD_POS;
	addData(payload, len);
#else
	memcpy(data+PAYLOAD_POS, payload, len);
	writeIndex = PAYLOAD_POS + len;
#endif
}

void StdProtocolReply::setErrorCode(u16 err)
{
	setField(ERROR_POS, err);
}
