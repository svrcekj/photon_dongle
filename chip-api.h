/*
 * slave.h
 *
 *  Created on: 28. 4. 2017
 *      Author: jiri svrcek
 */

#ifndef USER_APPLICATIONS_DONGLE2_CHIP_API_H_
#define USER_APPLICATIONS_DONGLE2_CHIP_API_H_

#include "global.h"
#include <application.h>
#include "com-master.h"

#define CMD_AUTO_TUNE				0xA5
#define CMD_SENSE_ON				0x93

class TChipApi {
public:
	TChipApi(ComMaster *slaveDevice);
	//void assignSlaveDevice(TSlaveDevice *slDev);
	void reset(void);
	void configureDataPin(slave_mode_t slave_mode);
	void init(slave_mode_t slave_mode);
	event_t readOneEvent(u8* event);
	void readWaitingEvents(void);
	void autoTune(void);
	void startScan(void);
	void readChipId(u16* hwVer, u8 *hwRev, u16* fwVer);
	bool chipAnsweredByNonZeroId(void);
	void getSysInfoRecord(void);
	void getRawFrame(u16 address, u8 *frameData, int rows, int cols);
	void burstReadByCommandAndAddress(u8 cmd, u16 address, u8 *data, int len);
private:
	ComMaster *slDev;
};



#endif /* USER_APPLICATIONS_DONGLE2_CHIP_API_H_ */
