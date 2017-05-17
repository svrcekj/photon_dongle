/*
 * serial.cpp
 *
 *  Created on: 27. 4. 2017
 *      Author: jiri svrcek
 */

#include "message.h"

#include "global.h"
#include "parser.h"
#include "led.h"
#include <application.h>

//====================================================================================

//						S e r i a l     r e s p o n s e

//====================================================================================

/*******************************************/
ResponseMessage::ResponseMessage(void)
/*******************************************/
{
	data[0] = UART_RESPONSE_START_BYTE1;
	data[1] = UART_RESPONSE_START_BYTE2;
	wrPtr = 4; // skip two bytes for payload size
}


/*******************************************/
void ResponseMessage::init(void)
/*******************************************/
{
	wrPtr = 4; // skip two bytes for payload size
}

/*******************************************/
void ResponseMessage::addByte(u8 b)
/*******************************************/
{
	if (wrPtr < SERIAL_INPUT_BUFFER_SIZE)
		data[wrPtr++] = b;
}

/*******************************************/
void ResponseMessage::addBytes(u8 *data, int len)
/*******************************************/
{
#if 1
	while (len--)
		addByte(*data++);
#else
	for (int i = 0; i < len; ++i)
	{
		addByte(data[i]);
	}
#endif
}

/*******************************************/
void ResponseMessage::finalize(void)
/*******************************************/
{
	// payload size is without 4-byte header
	data[2] = wrPtr - 4;
	data[3] = (wrPtr - 4) >> 8;
}

/*******************************************/
void ResponseMessage::send(void)
/*******************************************/
{
	finalize();
	USBSerial1.flush();
	USBSerial1.write(data, wrPtr);
}

/*******************************************/
void ResponseMessage::sendAck(void)
/*******************************************/
{
	const u8 ACK_BYTE = 0x00;
	init();
	addByte(ACK_BYTE);
	send();
}

/*******************************************/
void ResponseMessage::sendNack(void)
/*******************************************/
{
	const u8 NACK_BYTE = 0x01;
	init();
	addByte(NACK_BYTE);
	send();
}

/*******************************************/
void ResponseMessage::confirmBurstWrite(void)
/*******************************************/
{
	const u8 ACK_BYTE = 0x00;
	init();
	addByte(WRITE_REGISTER_BURST);
	addByte(ACK_BYTE);
	send();
}

/*************************************************************/
void ResponseMessage::addBurstReadHeader(bool ack, u16 len)
/*************************************************************/
{
	addByte(READ_REGISTER_BURST);
	addByte((u8) ack);
	addByte(len);
	addByte(len >> 8);
}

//====================================================================================

//						S e r i a l     r e q u e s t

//====================================================================================



/*******************************************/
void RequestMessage::init()
/*******************************************/
{
	SerialStartBytesFound = 0;
	newCommand = false;
	rdPtr = 0;
}

/*******************************************/
void RequestMessage::addByte(u8 newByte)
/*******************************************/
{
	if (SerialStartBytesFound == 0)
	{
		if (newByte == UART_ANSWER_START_BYTE1)
			SerialStartBytesFound++;
	}
	else if (SerialStartBytesFound == 1)
	{
		if (newByte == UART_ANSWER_START_BYTE2)
		{
			SerialStartBytesFound++;
			FoundCommandSize=0;
			CommandSize=0;
		}
		else
		{
			if (newByte != UART_ANSWER_START_BYTE2)
				SerialStartBytesFound = 0;
		}
	}
	else
	{
		if (FoundCommandSize == 0)
		{
			CommandSize=newByte;
			FoundCommandSize++;
		}
		else if (FoundCommandSize==1)
		{
			CommandSize += newByte << 8;
			FoundCommandSize++;
			ReadCount = 0;
		}
		else
		{
			if (ReadCount < CommandSize)
			{
				if (ReadCount < SERIAL_INPUT_BUFFER_SIZE)
					data[ReadCount++] = newByte;
				else
					SerialStartBytesFound = 0;
			}

			if (ReadCount == CommandSize)
			{
				USBSerial1.write(data[0]);
				newCommand = true;
				SerialStartBytesFound = 0;
				rdPtr = 0;
			}
		}
	}
}

/*******************************************************************/
void RequestMessage::processNewCommand(TDongleState *dongleState)
/*******************************************************************/
{
	u8 cmd = dequeue8();
	SerialParser_ProcessNewCommand(cmd, dongleState);
	newCommand = false;
}

/*******************************************/
u8 RequestMessage::dequeue8(void)
/*******************************************/
{
	if (rdPtr < SERIAL_INPUT_BUFFER_SIZE)
		return data[rdPtr++];
	return 0;
}

/*******************************************/
u16 RequestMessage::dequeue16(void)
/*******************************************/
{
	if (rdPtr > SERIAL_INPUT_BUFFER_SIZE)
		return 0;
	rdPtr += 2;
	return data[rdPtr-2] << 8 || data[rdPtr-1];
}

/*******************************************/
u32 RequestMessage::dequeue32(void)
/*******************************************/
{
	if (rdPtr+2 > SERIAL_INPUT_BUFFER_SIZE)
		return 0;
	rdPtr += 4;
	return (data[rdPtr-4] << 24) || (data[rdPtr-3] << 16) || (data[rdPtr-2] << 8) || (data[rdPtr-1]);

}

//---------------------------------------------------------------------------------------
// Packet structure for READ_REGISTER_BURST
//  --0- --1- --2- --3- --4-  -5-  -6-  -7-  -8-  -9-
// |0x11|....|....|....|....|....|....|....|....|..
//      aSize|burstSize|b[4] b[5] b[6] b[7] b[8]   dongle packet
//           |low  high|
//                       CMD  ADH  ADL  SH   SL
//  0x11 0x03 0x00 0x01 0xB6 0x40 0x00 0x00 0x01	example of GUI command B64000 256
//    11   03   07   00   b6   00   04				example of GUI command B60004 7
//----------------------------------------------------------------------------------------
// CMD...command,
// ADH...addr (MSB), ADL...addr (LSB),
// SH...size (MSB), SL...size (LSB)
//----------------------------------------------------------------------------------------

/*******************************************/
u16 RequestMessage::getReadAddressSize(void)
/*******************************************/
{
	return data[1];
}

/*******************************************/
u8 RequestMessage::getReadCommand(void)
/*******************************************/
{
	return data[4];
}

/*******************************************/
u16 RequestMessage::getReadLen(void)
/*******************************************/
{
	return data[2] + (data[3] << 8);
}

/*******************************************/
u16 RequestMessage::getWriteLen(void)
/*******************************************/
{
	u8 addressType = data[1];
	u8 burstSize = data[2];
	return addressType == 0 ? burstSize + 1 : burstSize + 2;
}

/*******************************************************/
void RequestMessage::fillDataToBeWritten(u8 *writeData)
/*******************************************************/
{
	//---------------------------------------------------------------------------------------
	// Packet structure for READ_REGISTER_WRITE
	//  --0- --1- --2- --3- --4-  -5-  -6-  -7-  -8-  -9-
	// |0x12|....|....|....|....|....|....|....|....|..
	//      aType|bSiz| address | ---data--------------   dongle packet
	//       AT    BS   ADH  ADL
	//  0x12 0x00 0x02 0xF7 0x00 0x52 0x34				  example of GUI command F7 52 34
	//----------------------------------------------------------------------------------------
	// AT ... address type,
	// BS ... burst size (length),
	// ADH...addr (MSB), ADL...addr (LSB),
	//----------------------------------------------------------------------------------------

	u8 addressType = data[1];
	u8 burstSize = data[2];
	u8 *payloadStart = &data[5];

	if (addressType == 0)
	{
		writeData[0] = data[3];
		memcpy(&writeData[1], payloadStart, burstSize);
	}
	else
	{
		memcpy(&writeData[0], &data[3], burstSize+2);
	}
}

/*******************************************/
bool RequestMessage::isResetCmd(void)
/*******************************************/
{
	if ((data[3] == 0xF7) && (data[5] == 0x52) && (data[6] == 0x34))
		return true;
	if (data[3] == 0xA0)
		return true;
	if ((data[3] == 0xB6) && (data[5] == 0x00) && (data[6] == 0x21) && (data[7] == 0x01))
		return true;
	return false;
}

