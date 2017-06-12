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
extern const u8 FIRMWARE_VERSION[];

void WifiOn();
void WifiOff();

u8 readData[1024 + NR_OF_DUMMY_BYTES];

void blink_led(int count)
{
	while (count--)
	{
		RGB.color(0,0,255);
		delay(50);
		RGB.color(255,0,0);
		delay(50);
		RGB.color(255,255,255);
		delay(50);
		RGB.color(0);
		delay(200);
	}
	delay(1500);
}

StdProtocolRequest::StdProtocolRequest()
{
	writeIndex = 0;
	readIndex = 0;
	completed = false;
	msgState = WAITING_START_BYTE;
	msgCounter = 0;
	declaredLength = 0;
	receivedLength = 0;
	reply = NULL;
	masterInterface = DEFAULT_MASTER_INTERFACE;
	nrOfDummyBytes = 0;
	byteToByteDelay = 0;
	lastByteArrival = 0;
	msgAction = 0;
}

void StdProtocolRequest::clear(void)
{
	writeIndex = 0;
	readIndex = 0;
	completed = false;
	msgState = WAITING_START_BYTE;
}

void StdProtocolRequest::addByte(u8 newByte)
{
	// check the delay between received bytes only when not waiting the start byte
	if (msgState != WAITING_START_BYTE)
	{
		byteToByteDelay = millis() - lastByteArrival;
		if (byteToByteDelay > BYTE_TO_BYTE_MAX_DELAY)
			clear();
	}

	lastByteArrival = millis();

	switch (msgState)
	{
	case WAITING_START_BYTE:
		if (newByte == STD_PROTOCOL_START_BYTE)
			msgState = WAITING_MSG_LEN_1;
		break;

	case WAITING_MSG_LEN_1:
		declaredLength = newByte << 8;
		msgState = WAITING_MSG_LEN_2;
		break;

	case WAITING_MSG_LEN_2:
		declaredLength += newByte;
		if (declaredLength < 8)
			msgState = WAITING_START_BYTE; // the shortest message can by 8 bytes
		else
			msgState = WAITING_MSG_CNT_1;
		break;

	case WAITING_MSG_CNT_1:
		msgCounter = newByte << 8;
		msgState = WAITING_MSG_CNT_2;
		break;

	case WAITING_MSG_CNT_2:
		msgCounter += newByte;
		msgState = WAITING_ACTION_1;
		break;

	case WAITING_ACTION_1:
		msgAction = newByte << 8;
		msgState = WAITING_ACTION_2;
		break;

	case WAITING_ACTION_2:
		msgAction += newByte;
		msgState = WAITING_PAYLOAD;
		receivedLength = 7;
		writeIndex = 0;
		break;

	case WAITING_PAYLOAD:
		if (++receivedLength < declaredLength)
		{
			if (writeIndex < REQUEST_MSG_MAX_SIZE)
			{
				data[writeIndex++] = newByte;
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
/*
	<START> <SIZE> <COUNTER> <ACTION> <PAYLOAD ..... PAYLOAD> <END>
	   0    1   2   3    4    5    6    7  ......... LEN-1     LEN

	Write: (12 bytes = 8 + 4)

             	 	   data[0]
	                     |
	                     V
	7B 00 0C FF FF 00 01 B6 00 23 01 7D
	7B 00 0A FF FF 00 01 00 00 7D .......... reply

	Read: (10 bytes)
	7B 00 0B FF FF 00 02 00 04 00 7D
	7B 00 0E FF FF 00 02 00 00 AA BB CC DD 7D ..... reply

	WriteRead: (14 bytes)
	7B 00 0E FF FF 00 03 B4 00 07 00 02 01 7D
	7B 00 0B FF FF 00 02 AA BB CC 7D  .... reply


	GetVersion: (8 bytes)
	7B 00 08 FF FF 00 04 7D
	7B 00 0C FF FF 00 04 00 00 12 34 7D .... reply
*/

	completed = false;
	reply->init(masterInterface, msgCounter);
	dongleState->command_type = GOOD_COMMAND;

	switch (msgAction)
	{
		case ACTION_WRITE:
		{
			u16 writeLength = receivedLength - 8;
			if ((writeLength > 0) && (writeLength <= MAX_WRITE_LEN))
			{
				for (int i = 0; i < NR_OF_ACTION_ATTEMPTS; ++i)
				{
					if (comMaster.writeN(data, writeLength))
					{
						reply->sendErrorCode(STATUS_OK);
						return;
					}
				}
				reply->sendErrorCode(STATUS_ERROR);
			}
			else
			{
				reply->sendErrorCode(STATUS_WRITE_LEN_ERROR);
				dongleState->command_type = BAD_COMMAND;
			}
			break;
		}
		case ACTION_READ:
		{
			u16 readLength = getField(0); // READ_LEN_POS
			u8 nrOfDummies = (u8) (data[2] > 0);
			if ((readLength > 0) && (readLength <= REPLY_MAX_PAYLOAD_SIZE))
			{
				for (int i = 0; i < NR_OF_ACTION_ATTEMPTS; ++i)
				{
					if (comMaster.readN(readData, readLength + nrOfDummies))
					{
						if (!reply->setPayloadData(readData + nrOfDummies, readLength))
						{
							reply->sendErrorCode(STATUS_MSG_SIZE_LIMIT);
							return;
						}
						if (!reply->send())
						{
							reply->sendErrorCode(STATUS_MSG_SIZE_LIMIT);
							return;
						}
						return;
					}
				}
				reply->sendErrorCode(STATUS_ERROR);
			}
			else
			{
				reply->sendErrorCode(STATUS_READ_LEN_ERROR);
				dongleState->command_type = BAD_COMMAND;
			}
			break;
		}
		case ACTION_WRITE_READ:
		{
			u16 writeLength = receivedLength - 11;
			if ((writeLength < 1) || (writeLength > MAX_WRITE_LEN))
			{
				reply->sendErrorCode(STATUS_WRITE_LEN_ERROR);
				dongleState->command_type = BAD_COMMAND;
				return;
			}

			u16 readLength = getField(writeLength);
			u16 nrOfDummies = (u8) (data[writeLength + 2] > 0);

			if ((readLength < 1) || (readLength > REPLY_MAX_PAYLOAD_SIZE))
			{
				reply->sendErrorCode(STATUS_READ_LEN_ERROR);
				dongleState->command_type = BAD_COMMAND;
				return;
			}

			for (int i = 0; i < NR_OF_ACTION_ATTEMPTS; ++i)
			{
				if (comMaster.writeNReadN(data, writeLength, readData, readLength + nrOfDummies))
				{
					if (!reply->setPayloadData(readData + nrOfDummies, readLength))
					{
						reply->sendErrorCode(STATUS_MSG_SIZE_LIMIT);
						return;
					}
					if (!reply->send())
					{
						reply->sendErrorCode(STATUS_MSG_SIZE_LIMIT);
						return;
					}
					return;
				}
			}
			reply->sendErrorCode(STATUS_ERROR);
			break;
		}
		case ACTION_GET_VERSION:
		{
			u16 version = FIRMWARE_VERSION[0] << 8;
			version += FIRMWARE_VERSION[1];
			reply->sendFwVersion(version);
			break;
		}
		case ACTION_GET_I2C_ADDRESS:
		{
			reply->addByte(dongleState->i2cAddress);
			reply->send();
			break;
		}
		case ACTION_SET_I2C_ADDRESS:
		{
			dongleState->i2cAddress = dequeue8();
			reply->sendErrorCode(STATUS_OK);
			break;
		}
		case ACTION_GET_I2C_SPEED:
		{
			reply->addField32(dongleState->i2cSpeed);
			reply->send();
			break;
		}
		case ACTION_SET_I2C_SPEED:
		{
			dongleState->i2cSpeed = dequeue32(NUMBER_FORMAT_BIG_ENDIAN);
			dongleState->command_type = GOOD_COMMAND;
			break;
		}
		case ACTION_GET_SPI_PRESCALLER:
		{
			reply->addByte((u8)(dongleState->slave_mode == SLAVE_MODE_SPI));
			reply->addByte(dongleState->spiPrescaller);
			reply->send();
			break;
		}
		case ACTION_SET_SPI_PRESCALLER:
		{
			dongleState->spiPrescaller = dequeue8();
			reply->sendErrorCode(STATUS_OK);
			break;
		}
		case ACTION_SYSTEM_CTRL: // 7B 00 0E FF FF 11 11 <CTRL_CMD> 7D
		{
			switch (data[0])
			{
			case 0x00:
				WifiOn();
				break;
			case 0x01:
				WifiOff();
				break;
			case 0x02:
				System.dfu();
				break;
			case 0x03:
				System.reset();
				break;
			default:
				break;
			}
			break;
		}
		default:
			dongleState->command_type = BAD_COMMAND;
			break;
	}
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

/*
u16 StdProtocolRequest::getReadLength(void)
{
	return getField(READ_LEN_POS);
}
*/

/*
u16 StdProtocolRequest::getWriteLength(void)
{
	return receivedLength - 11;
}
*/

ProtocolAction StdProtocolRequest::getAction(void)
{
	return (ProtocolAction) msgAction;
}

u16 StdProtocolRequest::getField(u16 position)
{
	if (position < REQUEST_MSG_MAX_SIZE - 2)
		return (data[position] << 8) + data[position+1];
	else
		return 0;
}

/*void StdProtocolRequest::prepareDataToSend(u8* srcData, u16 len)
{
	memcpy(data, srcData, len);
}*/
