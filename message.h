/*
 * serial.hpp
 *
 *  Created on: 27. 4. 2017
 *      Author: jiri svrcek
 */

#ifndef USER_APPLICATIONS_DONGLE2_MESSAGE_H_
#define USER_APPLICATIONS_DONGLE2_MESSAGE_H_

#include <application.h>

#define MSG_MAX_SIZE				256

class AMessage {
public:
	void addByte(u8 newByte);
	void addData(u8 *newData, int len);
private:
	u8 data[MSG_MAX_SIZE];
	int writeIndex;
	int readIndex;
};

#if 0

class AReplyMessage;

/*****************************/
class ARequestMessage
/*****************************/
{
public:
	virtual ~ARequestMessage() = 0;
	void assignReplyMessage(AReplyMessage *replyMsg) {reply = replyMsg; }
	virtual void init(void) = 0;
	virtual void addByte(u8 newByte) = 0;
	u8 dequeue8(void);
	u16 dequeue16(EndianFormat endianFormat);
	u32 dequeue32(EndianFormat endianFormat);
	bool completed() {return _completed;}
	virtual void processNew(TDongleState *dongleState) = 0;
	u16 getReadLength(void);
	u16 getWriteLength(void);
	u8* getDataPointer(void) {return data;}
	u8 getNrOfDummyBytes(void);
protected:
	u8 data[REQUEST_MSG_MAX_SIZE];
	int writeIndex;
	int readIndex;
	int declaredLength;
	int receivedLength;
	bool _completed;
	AReplyMessage *reply;
	masterInterface_t masterInterface;
};

/*******************************************/
class StdProtocolRequest : public ARequestMessage
/*******************************************/
{
public:
	void init(void);
	void addByte(u8 newByte);
	void processNew(TDongleState *dongleState);
	u16 getReadAddressSize(void);
	u8 getReadCommand(void);
	void fillDataToBeWritten(u8 *writeData);
	bool isResetCmd(void);
private:
	typedef enum {
		WAITING_START_BYTE,
		WAITING_MSG_LEN_MSB,
		WAITING_MSG_LEN_LSB,
		WAITING_MSG_CNT_MSB,
		WAITING_MSG_CNT_LSB,
		WAITING_MSG_BODY
	} msgState_t;
	msgState_t msgState;
	int msgCounter;
	//int SerialStartBytesFound;
	//int FoundCommandSize;
	//int CounterBytesFound;
	//int CommandSize;
};

/*******************************************/
class GrayDongleRequest : public ARequestMessage
/*******************************************/
{
public:
	void init(void);
	void addByte(u8 newByte);
	void processNew(TDongleState *dongleState);
	u16 getReadAddressSize(void);
	u8 getReadCommand(void);
	u16 getReadLen(void);
	u16 getWriteLen(void);
	void fillDataToBeWritten(u8 *writeData);
	bool isResetCmd(void);
private:
	int startBytesFound;
	int commandSizeFound;
	u16 getReadAddressSize(void);
	u8 getReadCommand(void);
	u16 getReadLen(void);
	u16 getWriteLen(void);
	void fillDataToBeWritten(u8* writeData);
	bool isResetCmd(void);
};

//============================================================================================


/*********************/
class AReplyMessage
/*********************/
{
public:
	virtual ~AReplyMessage(void);
	virtual void init(masterInterface_t ifaceType, u16 msgCounter) = 0;
	void setNrOdDummyBytes(u8 dummyBytes);
	void addByte(u8 b);
	void addBytes(u8 *data, int len);
	virtual void sendWriteStatus(u16 status) = 0;
	void send(void);
	void send(u8 *dataToBeSent, int len);
	virtual void finalize(void) = 0;
protected:
	u8 data[REPLY_MSG_MAX_SIZE];
	int wrPtr;
	u8 nrOfDummyBytes;
	masterInterface_t masterInterface;
};

/*****************************************/
class StdProtocolReply : public AReplyMessage
/*****************************************/
{
public:
	StdProtocolReply(void);
	void init(masterInterface_t ifaceType, u16 msgCounter);
	void setCounterField(u16 counter);
	void addByte(u8 b);
	void addBytes(u8 *data, int len);
	void send(void);
	void sendWriteStatus(u16 status);
private:
	void finalize(void);
};

/*****************************************/
class GrayDongleReply : public AReplyMessage
/*****************************************/
{
public:
	GrayDongleReply(void);
	void init(masterInterface_t ifaceType, u16 msgCounter);
	void addByte(u8 b);
	void addBytes(u8 *data, int len);
	void send(void);
	void sendWriteStatus(u16 status);
	void sendAck(void);
	void sendNack(void);
	void confirmBurstWrite(void);
	void addBurstReadHeader(bool ack, u16 len);
private:
	void finalize(void);
};
#endif

#if 0
/*********************/
class TextCommand
/*********************/
{
public:
	TextCommand(int maxSize);
	void reset(void);
	void addByte(u8 newByte);
	~TextCommand();
	void terminate(void) {data[bytesRead] = '\0';}
	int length(void) {return bytesRead;}
	u8 *toArray(void) {return data;}
	int getNumber();
private:
	u8 *data;
	int bytesRead;
	int sizeLimit;
};
#endif

#endif /* USER_APPLICATIONS_DONGLE2_MESSAGE_H_ */
