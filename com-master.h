/*
 * device.hpp
 *
 *  Created on: 27. 4. 2017
 *      Author: jiri svrcek
 */

#ifndef USER_APPLICATIONS_DONGLE2_COM_MASTER_H_
#define USER_APPLICATIONS_DONGLE2_COM_MASTER_H_

#include "global.h"
#include <application.h>

class ComMaster {
public:
	ComMaster(slave_mode_t mode);
	void setMode(slave_mode_t mode);
	void write1(u8 b1);
	void write2(u8 b1, u8 b2);
	void write3(u8 b1, u8 b2, u8 b3);
	void write4(u8 b1, u8 b2, u8 b3, u8 b4);
	void writeN(u8 * data, int len);
	void write1ReadN(u8 writeB1, u8* readData, int readLen);
	void write2ReadN(u8 writeB1, u8 writeB2, u8* readData, int readLen);
	void write3ReadN(u8 writeB1, u8 writeB2, u8 writeB3, u8* readData, int readLen);
	void write5ReadN(u8 b1, u8 b2, u8 b3, u8 b4 , u8 b5, u8* readData, int readLen);
	void writeNReadN(u8 *writeData, int writeLen, u8* readData, int readLen);
	void readN(u8* data, int len);
	/***********************************/
	//void writeReadU8U16(u8 opCode, addr_size_t addrSize, u32 address, size_t count, bool dummyByte);
	//void writeU8UXThenWriteU8UX(byte opCodeW1, AddressSizeEnum addressSizeW1, byte opCodeW2, AddressSizeEnum addressSizeW2, uint address, byte[] data, IProgress<double> progress = null)
	/***********************************/
private:
	slave_mode_t slave_mode;
	int bytes_to_send;
	void beginTx();
	void endTx();
	void beginRx();
	void endRx();
	void writeByteToActiveChannel(u8 b);
};


#endif /* USER_APPLICATIONS_DONGLE2_COM_MASTER_H_ */
