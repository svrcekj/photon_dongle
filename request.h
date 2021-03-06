/*
 * request.h
 *
 *  Created on: 22. 5. 2017
 *      Author: jiri svrcek
 */

#ifndef USER_APPLICATIONS_DONGLE2_REQUEST_H_
#define USER_APPLICATIONS_DONGLE2_REQUEST_H_


#include "global.h"
#include "reply.h"
#include <application.h>

#define REQUEST_MSG_MAX_SIZE				256

#define GRAY_DONGLE_REQUEST_START_BYTE1		0x07
#define GRAY_DONGLE_REQUEST_START_BYTE2		0x01

#define STD_PROTOCOL_START_BYTE				'{'
#define STD_PROTOCOL_END_BYTE				'}'

#define BYTE_TO_BYTE_MAX_DELAY				50 // milliseconds

#define NR_OF_ACTION_ATTEMPTS				3

class StdProtocolReply;

/*******************************************/
class StdProtocolRequest
/*******************************************/
{
public:
	StdProtocolRequest();
	void assignReply(StdProtocolReply *assignedReply) { reply = assignedReply; }
	void clear(void);
	void addByte(u8 newByte);
	bool isCompleted(void) {return completed;}
	void processNew(TDongleState *dongleState);
	ProtocolAction getAction(void);
	u16 getField(u16 position);
	//void prepareDataToSend(u8 *srcData, u16 len);
	//u16 getReadAddressSize(void);
	//u8 getReadCommand(void);
	//void fillDataToBeWritten(u8 *writeData);
	//bool isResetCmd(void);

	u8 dequeue8(void);
	u16 dequeue16(EndianFormat endianFormat);
	u32 dequeue32(EndianFormat endianFormat);
	//u16 getReadLength(void);
	//u16 getWriteLength(void);
	void setNrOfDummyBytes(u8 dummyBytes) {nrOfDummyBytes = dummyBytes;}
private:
	enum {
		START_BYTE_POS = 0,
		MSG_SIZE_POS = 1,
		COUNTER_POS = 3,
		ACTION_POS = 5,
		ERROR_POS = 7
	};
	enum {
		READ_LEN_POS = 0
	};
	typedef enum {
		WAITING_START_BYTE,
		WAITING_MSG_LEN_1,
		WAITING_MSG_LEN_2,
		WAITING_MSG_CNT_1,
		WAITING_MSG_CNT_2,
		WAITING_ACTION_1,
		WAITING_ACTION_2,
		WAITING_PAYLOAD,
		WAITING_END_BYTE
	} msgState_t;
	msgState_t msgState;
	u16 msgCounter;
	u16 msgAction;
	//int SerialStartBytesFound;
	//int FoundCommandSize;
	//int CounterBytesFound;
	//int CommandSize;
	u8 data[REQUEST_MSG_MAX_SIZE];
	int writeIndex;
	int readIndex;
	int declaredLength;
	int receivedLength;
	bool completed;
	StdProtocolReply *reply;
	masterInterface_t masterInterface;
	u8 nrOfDummyBytes;
	system_tick_t byteToByteDelay;
	system_tick_t lastByteArrival;
};



#endif /* USER_APPLICATIONS_DONGLE2_REQUEST_H_ */
