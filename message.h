/*
 * serial.hpp
 *
 *  Created on: 27. 4. 2017
 *      Author: jiri svrcek
 */

#ifndef USER_APPLICATIONS_DONGLE2_MESSAGE_H_
#define USER_APPLICATIONS_DONGLE2_MESSAGE_H_

#include "global.h"
#include <application.h>

#define SERIAL_INPUT_BUFFER_SIZE		256
#define SERIAL_OUTPUT_BUFFER_SIZE		256

#define UART_RESPONSE_START_BYTE1		0x06
#define UART_RESPONSE_START_BYTE2		0x01

#define UART_ANSWER_START_BYTE1			0x07
#define UART_ANSWER_START_BYTE2			0x01

/*********************/
class ResponseMessage
/*********************/
{
public:
	ResponseMessage(void);
	void init(void);
	void addByte(u8 b);
	void addBytes(u8 *data, int len);
	void send(void);
	void sendAck(void);
	void sendNack(void);
	void confirmBurstWrite(void);
	void addBurstReadHeader(bool ack, u16 len);
private:
	int wrPtr;
	u8 data[SERIAL_OUTPUT_BUFFER_SIZE];
	void finalize(void);
};

/*********************/
class RequestMessage
/*********************/
{
public:
	void init(void);
	void addByte(u8 b);
	u8 dequeue8(void);
	u16 dequeue16(void);
	u32 dequeue32(void);
	bool newCommandAvailable() {return newCommand;}
	void processNewCommand(TDongleState *dongleState);
	u16 getReadAddressSize(void);
	u8 getReadCommand(void);
	u16 getReadLen(void);
	u16 getWriteLen(void);
	u8* getDataPointer(void) {return data;}
	void fillDataToBeWritten(u8 *writeData);
	bool isResetCmd(void);
private:
	int rdPtr;
	bool newCommand;
	u8 data[SERIAL_INPUT_BUFFER_SIZE];
	int SerialStartBytesFound;
	int FoundCommandSize;
	int CommandSize;
	int ReadCount;
};




#endif /* USER_APPLICATIONS_DONGLE2_MESSAGE_H_ */
