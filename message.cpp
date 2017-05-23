/*
 * serial.cpp
 *
 *  Created on: 27. 4. 2017
 *      Author: jiri svrcek
 */

#include "message.h"
#include "application.h"

void AMessage::addByte(u8 newByte)
{
	if (writeIndex < MSG_MAX_SIZE)
		data[writeIndex++] = newByte;
}

void AMessage::addData(u8* newData, int len)
{
	while (len--)
		addByte(*newData++);
}

#if 0
#include "parser.h"
#include "led.h"
#include "com-master.h"


extern TDongleState dongleState;
extern ComMaster comMaster;
extern TCPServer server;

u8 readData[1024 + NR_OF_DUMMY_BYTES];

void StdProtocolRequest::init(void)
{
	writeIndex = 0;
	readIndex = 0;
	completed = false;
	reply = NULL;
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
				msgState = WAITING_START_BYTE;
			}
		}
		break;

	default:
		msgState = WAITING_START_BYTE;
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





StdProtocolReply::StdProtocolReply(void)
{
	data[0] = STD_PROTOCOL_START_BYTE;
}

void StdProtocolReply::init(masterInterface_t ifaceType, u16 replyCounter)
{
	wrPtr = 5; // skip four bytes for payload size
	setCounterField(replyCounter);
	masterInterface = ifaceType;
}

void StdProtocolReply::setCounterField(u16 counter)
{
	data[3] = counter;
	data[4] = counter << 8;
}

void StdProtocolReply::sendWriteStatus(u16 status)
{
	addByte(status);
	addByte(status << 8);
	send();
}

void StdProtocolReply::finalize(void)
{
	// payload size is without 5-byte header
	data[2] = wrPtr - 5;
	data[3] = (wrPtr - 5) >> 8;
	data[wrPtr] = STD_PROTOCOL_END_BYTE;
}
#endif

#if 0
//====================================================================================
//				A b s t r a c t     r e q u e s t     c l a s s
//====================================================================================

void ARequestMessage::init(void)
{
	writeIndex = 0;
	readIndex = 0;
	_completed = false;
	reply = NULL;
}

void ARequestMessage::addByte(u8 newByte)
{
	// DO NOT USE THIS FUNCTION UNLESS YOU DEBUG THE CODE!
	if (writeIndex < REQUEST_MSG_MAX_SIZE)
		data[writeIndex++] = newByte;
	if (writeIndex == REQUEST_MSG_MAX_SIZE)
		_completed = true;
}

u8 ARequestMessage::dequeue8(void)
{
	if (readIndex < REQUEST_MSG_MAX_SIZE)
		return data[readIndex++];
	return 0;
}

u16 ARequestMessage::dequeue16(EndianFormat endianFormat)
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

u32 ARequestMessage::dequeue32(EndianFormat endianFormat)
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

void ARequestMessage::processNew(TDongleState * dongleState)
{
	writeIndex = 0;
	_completed = false;
}


//====================================================================================
//				S t a n d a r d     p r o t o c o l     r e q u e s t
//====================================================================================


void StdProtocolRequest::init(void)
{
	writeIndex = 0;
	readIndex = 0;
	_completed = false;
	reply = NULL;
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
				_completed = true;
			}
			else // no END byte, msg corrupted, restart state machine
			{
				msgState = WAITING_START_BYTE;
			}
		}
		break;

	default:
		msgState = WAITING_START_BYTE;
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
		u8 * writeData = getDataPointer();
		const int writeLen = writeIndex - 2; // payload size without Action size
		comMaster.writeN(writeData, writeLen);
		reply->sendWriteStatus(0);
		break;
	}
	case ACTION_READ:
	{
		const int readLen = getReadLength();
		comMaster.readN(readData, readLen);
		reply->setNrOdDummyBytes(getNrOfDummyBytes());
		reply->send();
		break;
	}
	case ACTION_WRITE_READ:
		break;

	case ACTION_GET_VERSION:
		break;
	}
	_completed = false;
}

u16 StdProtocolRequest::getReadAddressSize(void)
{
	return 0;
}

u8 StdProtocolRequest::getReadCommand(void)
{
	return 0;
}

u16 StdProtocolRequest::getReadLen(void)
{
	return 0;
}

u16 StdProtocolRequest::getWriteLen(void)
{
	return 0;
}

void StdProtocolRequest::fillDataToBeWritten(u8* writeData)
{

}

bool StdProtocolRequest::isResetCmd(void)
{
	return false;
}






//====================================================================================
//				G r a y    d o n g l e     p r o t o c o l     r e q u e s t
//====================================================================================

void GrayDongleRequest::init(void)
{

}

void GrayDongleRequest::addByte(u8 newByte)
{
	if (startBytesFound == 0)
	{
		if (newByte == GRAY_DONGLE_REQUEST_START_BYTE1)
			startBytesFound++;
	}
	else if (startBytesFound == 1)
	{
		if (newByte == GRAY_DONGLE_REQUEST_START_BYTE2)
		{
			startBytesFound++;
			commandSizeFound = 0;
			writeIndex = 0;
		}
		else
		{
			if (newByte != GRAY_DONGLE_REQUEST_START_BYTE2)
				startBytesFound = 0;
		}
	}
	else // startBytesFound == 2
	{
		if (commandSizeFound == 0)
		{
			declaredLength = newByte;
			commandSizeFound++;
		}
		else if (commandSizeFound == 1)
		{
			declaredLength += newByte << 8;
			commandSizeFound++;
			receivedLength = 0;
		}
		else // commandSizeFound == 2
		{
			if (receivedLength < declaredLength)
			{
				if (writeIndex < REQUEST_MSG_MAX_SIZE)
					data[writeIndex++] = newByte;
				else
					startBytesFound = 0;
			}

			if (writeIndex == declaredLength)
			{
				USBSerial1.write(data[0]);
				_completed = true;
				startBytesFound = 0;
				readIndex = 0;
			}
		}
	}
}



//====================================================================================
//				A b s t r a c t     r e s p o n s e      c l a s s
//====================================================================================

void AReplyMessage::setNrOdDummyBytes(u8 dummyBytes)
{
	nrOfDummyBytes = dummyBytes;
}

void AReplyMessage::addByte(u8 b)
{
	if (wrPtr < REPLY_MSG_MAX_SIZE)
		data[wrPtr++] = b;
}

void AReplyMessage::addBytes(u8 *data, int len)
{
	while (len--)
		addByte(*data++);
}

void AReplyMessage::send(void)
{
	send(data+nrOfDummyBytes, wrPtr-nrOfDummyBytes);
}

void AReplyMessage::send(u8 *dataToBeSent, int len)
{
	finalize();
	if (masterInterface == MASTER_IFACE_USB)
	{
		USBSerial1.flush();
		USBSerial1.write(dataToBeSent, len);
	}
	else if (masterInterface == MASTER_IFACE_WIFI)
	{
		int fullChunks = len / dongleState.dataChunkSize;
		for (int i = 0; i < fullChunks; i++)
		{
			server.write(&dataToBeSent[i * dongleState.dataChunkSize], dongleState.dataChunkSize);
		}
		int lastChunkSize = len % dongleState.dataChunkSize;
		if (lastChunkSize > 0)
		{
			server.write(&dataToBeSent[fullChunks * dongleState.dataChunkSize], lastChunkSize);
		}
	}
}

//====================================================================================
//				S t a n d a r d     p r o t o c o l     r e s p o n s e
//====================================================================================

StdProtocolReply::StdProtocolReply(void)
{
	data[0] = STD_PROTOCOL_START_BYTE;
}

void StdProtocolReply::init(masterInterface_t ifaceType, u16 replyCounter)
{
	wrPtr = 5; // skip four bytes for payload size
	setCounterField(replyCounter);
	masterInterface = ifaceType;
}

void StdProtocolReply::setCounterField(u16 counter)
{
	data[3] = counter;
	data[4] = counter << 8;
}

void StdProtocolReply::sendWriteStatus(u16 status)
{
	addByte(status);
	addByte(status << 8);
	send();
}

void StdProtocolReply::finalize(void)
{
	// payload size is without 5-byte header
	data[2] = wrPtr - 5;
	data[3] = (wrPtr - 5) >> 8;
	data[wrPtr] = STD_PROTOCOL_END_BYTE;
}

//====================================================================================
//					G r a y     d o n g l e     r e s p o n s e
//====================================================================================

GrayDongleReply::GrayDongleReply(void)
{
	data[0] = GRAY_DONGLE_REPLY_START_BYTE1;
	data[1] = GRAY_DONGLE_REPLY_START_BYTE2;
	wrPtr = 4; // skip two bytes for payload size
}


void GrayDongleReply::init(masterInterface_t ifaceType, u16 msgCounter)
{
	masterInterface = ifaceType;
	(void) msgCounter; // not used
	wrPtr = 4; // skip two bytes for payload size
}

void GrayDongleReply::finalize(void)
{
	// payload size is without 4-byte header
	data[2] = wrPtr - 4;
	data[3] = (wrPtr - 4) >> 8;
}

void GrayDongleReply::send(void)
{
	finalize();
	USBSerial1.flush();
	USBSerial1.write(data, wrPtr);
}

void GrayDongleReply::sendAck(void)
{
	const u8 ACK_BYTE = 0x00;
	init(masterInterface, 0x0000);
	addByte(ACK_BYTE);
	send();
}

void GrayDongleReply::sendNack(void)
{
	const u8 NACK_BYTE = 0x01;
	init(masterInterface, 0x0000);
	addByte(NACK_BYTE);
	send();
}

void GrayDongleReply::confirmBurstWrite(void)
{
	const u8 ACK_BYTE = 0x00;
	init(masterInterface, 0x0000);
	addByte(WRITE_REGISTER_BURST);
	addByte(ACK_BYTE);
	send();
}

void GrayDongleReply::addBurstReadHeader(bool ack, u16 len)
{
	addByte(READ_REGISTER_BURST);
	addByte((u8) ack);
	addByte(len);
	addByte(len >> 8);
}

void GrayDongleReply::sendWriteStatus(u16 status)
{
	confirmBurstWrite();
}
#endif

#if 0
//====================================================================================
//							T e x t    C o m m a n d
//====================================================================================

TextCommand::TextCommand(int maxSize)
{
	sizeLimit = maxSize;
	data = new u8[maxSize];
	reset();
}

TextCommand::~TextCommand()
{
	delete [] data;
}

void TextCommand::reset(void)
{
	bytesRead = 0;
}

void TextCommand::addByte(u8 newByte)
{
	if (sizeLimit < bytesRead)
		data[bytesRead++] = newByte;
}

int TextCommand::getNumber(void)
{
	u8 *lastByte = &data[bytesRead-1];
	u8 *firstByte = lastByte;

	while (charIsDigit(*firstByte--)) // finds first digit in the string
		;

	String tmp = String((char *)firstByte);
	return tmp.toInt();
}
#endif
