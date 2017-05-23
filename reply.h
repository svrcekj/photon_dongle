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

#define REPLY_MSG_MAX_SIZE					256

class StdProtocolRequest;

/*****************************************/
class StdProtocolReply
/*****************************************/
{
public:
	void assignRequest(StdProtocolRequest *req) {request = req;}
	void init(masterInterface_t ifaceType, u16 msgCounter);
	void setCounterField(u16 counter);
	void addByte(u8 newByte);
	void addData(u8 *newData, int len);
	void addField(u16 position, u16 field);
	void send(void);
	void send(u8 *dataToSent, int len);
	void sendWriteStatus(u16 status);
	void setNrOfDummyBytes(u8 dummyBytes) {nrOfDummyBytes = dummyBytes;}
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
		ERROR_POS = 7
	};
	StdProtocolRequest *request;
	void finalize(void);
	u8 data[REPLY_MSG_MAX_SIZE];
	int writeIndex;
	u8 nrOfDummyBytes;
	masterInterface_t masterInterface;
};


#endif /* USER_APPLICATIONS_DONGLE2_REPLY_H_ */
