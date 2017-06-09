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
	void setMode(slave_mode_t mode) {slave_mode = mode;}
	bool writeN(u8 * data, int len);
	bool writeNReadN(u8 *writeData, int writeLen, u8* readData, int readLen);
	bool readN(u8* data, int len);
private:
	slave_mode_t slave_mode;
	int bytes_to_send;
	void beginTx();
	void endTx();
	void beginRx();
	void endRx();
	bool writeByteToActiveChannel(u8 b, bool sendStopBit = true);
	bool ReadAfterWrite(u8* data, int len);
};

#endif /* USER_APPLICATIONS_DONGLE2_COM_MASTER_H_ */
