/*
 * reply.h
 *
 *  Created on: 22. 5. 2017
 *      Author: jiri svrcek
 */

#ifndef USER_APPLICATIONS_DONGLE2_REPLY_H_
#define USER_APPLICATIONS_DONGLE2_REPLY_H_


#include "global.h"
#include "request.h"
#include <application.h>

#define REPLY_MAX_PAYLOAD_SIZE				1024
#define REPLY_HEADER_SIZE					10
#define REPLY_MAX_PACKET_SIZE						(REPLY_MAX_PAYLOAD_SIZE + REPLY_HEADER_SIZE)

class StdProtocolRequest;

/*****************************************/
class StdProtocolReply
/*****************************************/
{
public:
	void assignRequest(StdProtocolRequest *req) {request = req;}
	void init(masterInterface_t ifaceType, u16 msgCounter);
	void addByte(u8 newByte);
	void addField(u16 field);
	void addField32(u32 field);
	void addData(u8 *newData, int len);
	void setField(u16 position, u16 field);
	bool send(void);
	bool send(u8 *dataToSent, int len);
	void sendErrorCode(u16 status);
	void sendFwVersion(u16 version);
	void setNrOfDummyBytes(u8 dummyBytes) {nrOfDummyBytes = dummyBytes;}
	bool setPayloadData(u8 *payload, int len);
	void setErrorCode(u16 err);
private:
	enum {
		START_BYTE = '{',
		STOP_BYTE = '}'
	};
	enum {
		START_BYTE_POS = 0,
		MSG_SIZE_POS = 1,
		COUNTER_POS = 3,
		ACTION_POS = 5,
		ERROR_POS = 7,
		PAYLOAD_POS = 9
	};
	StdProtocolRequest *request;
	bool finalize(void);
	u8 data[REPLY_MAX_PAYLOAD_SIZE + REPLY_HEADER_SIZE];
	int writeIndex;
	u8 nrOfDummyBytes;
	masterInterface_t masterInterface;
};


#endif /* USER_APPLICATIONS_DONGLE2_REPLY_H_ */
