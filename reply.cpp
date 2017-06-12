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
	if (writeIndex < (REPLY_MAX_PAYLOAD_SIZE-1))
		data[writeIndex++] = newByte;
}

void StdProtocolReply::addField(u16 field)
{
	if (writeIndex < (REPLY_MAX_PAYLOAD_SIZE-2))
	{
		data[writeIndex++] = field >> 8;
		data[writeIndex++] = field;
	}
}

void StdProtocolReply::addField32(u32 field)
{
	if (writeIndex < (REPLY_MAX_PAYLOAD_SIZE-4))
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

bool StdProtocolReply::send(void)
{
	return send(data, writeIndex+1);
}

bool StdProtocolReply::send(u8 *dataToSent, int len)
{
	if (len > REPLY_MAX_PACKET_SIZE)
		return false;

	if (!finalize())
		return false;

	if (masterInterface == MASTER_INTERFACE_USB)
	{
		USBSerial1.flush();
		USBSerial1.write(dataToSent, len);
		return true;
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
		return true;
	}
	return false;
}

void StdProtocolReply::sendErrorCode(u16 status)
{
	setField(ERROR_POS, status);
	writeIndex = PAYLOAD_POS;
	send();
}

void StdProtocolReply::sendFwVersion(u16 version)
{
	addByte(version >> 8);
	addByte(version);
	send();
}

void StdProtocolReply::setField(u16 position, u16 field)
{
	data[position] = field >> 8;
	data[position + 1] = field;
}

bool StdProtocolReply::finalize(void)
{
	setField(ACTION_POS, (u16) request->getAction());
	if (writeIndex < REPLY_MAX_PACKET_SIZE)
	{
		setField(MSG_SIZE_POS, writeIndex+1);
		data[writeIndex] = STOP_BYTE;
		return true;
	}
	return false;
}

bool StdProtocolReply::setPayloadData(u8* payload, int len)
{
#if USE_SAFE_COPY
	writeIndex = PAYLOAD_POS;
	addData(payload, len);
	return true;
#else
	if (len <= REPLY_MAX_PAYLOAD_SIZE)
	{
		memcpy(data+PAYLOAD_POS, payload, len);
		writeIndex = PAYLOAD_POS + len;
		return true;
	}
#endif
	return false;
}

void StdProtocolReply::setErrorCode(u16 err)
{
	setField(ERROR_POS, err);
}
