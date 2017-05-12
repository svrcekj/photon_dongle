/*
 * slave.cpp
 *
 *  Created on: 28. 4. 2017
 *      Author: jiri svrcek
 */


#include "chip-api.h"

#include "com-master.h"
#include "hw-specific.h"

/*****************************************************************************
 *
 *
 *  S l a v e    D e v i c e    S p e c i f i c    A P I    f u n c t i o n s
 *
 *
 *****************************************************************************/

TChipApi::TChipApi(ComMaster *slaveDevice)
{
	slDev = slaveDevice;
}

/*
void TChipApi::assignSlaveDevice(TSlaveDevice *slDev)
{
	dev = slDev;
}
*/

/***********************************************/
void TChipApi::configureDataPin(void)
/***********************************************/
{
	slDev->write4(0xB6, 0x00, 0x3C, 0x80);
	delayMicroseconds(20);
	slDev->write4(0xB6, 0x00, 0x31, 0x40);
	delayMicroseconds(20);
}

/***********************************************/
void TChipApi::init(slave_mode_t slave_mode)
/***********************************************/
{
#if FORCE_NRST_LOW
	HwSpecific_ForceNrstPinLow();
	delayMicroseconds(50);
#endif
	HwSpecific_ForceModePin(slave_mode);
	delayMicroseconds(50);
	reset();
	delayMicroseconds(10000);
	HwSpecific_ReleaseModePin();
	configureDataPin();
	delayMicroseconds(100);
}

/***********************************************/
void TChipApi::reset(void)
/***********************************************/
{
	slDev->write3(0xF7, 0x52, 0x34);
}

/***********************************************/
event_t TChipApi::readOneEvent(u8* event)
/***********************************************/
{
	const int replySize = 8 + 1;
	int zero_cnt = 0;
	u8 data[replySize];
	slDev->write1ReadN(0x85, data, replySize);
	for (int i = 0; i < 8; i++)
	{
		event[i] = data[i+1];
		if (event[i] == 0)
			zero_cnt++;
	}
	return (zero_cnt == 8) ? ZERO_EVENT: NON_ZERO_EVENT;
}

/***********************************************/
void TChipApi::readWaitingEvents(void)
/***********************************************/
{
	int safety_cnt = 0;
	u8 event[8];
	while (readOneEvent(event) == NON_ZERO_EVENT)
	{
		if (safety_cnt++ > 128)
			return;
	}
}

/***********************************************/
void TChipApi::autoTune(void)
/***********************************************/
{
	slDev->write1(CMD_AUTO_TUNE);
}

/***********************************************/
void TChipApi::startScan(void)
/***********************************************/
{
	slDev->write1(CMD_SENSE_ON);
}

/***********************************************/
void TChipApi::readChipId(u16* hwVer, u8* hwRev, u16* fwVer)
/***********************************************/
{
	const int replySize = 6 + 1;
	u8 data[replySize];

	slDev->write3ReadN(0xB4, 0x00, 0x04, data, replySize);

	*hwVer = (data[1] << 8) + data[2];
	*hwRev = data[3];
	*fwVer = (data[5] << 8) + data[6];

}

/***********************************************/
void TChipApi::getSysInfoRecord(void)
/***********************************************/
{
	const int replySize = 2 + 1;
	u8 data[replySize];
	slDev->write3ReadN(0xD0, 0x00, 0x00, data, replySize);
}

/***********************************************/
void TChipApi::getRawFrame(u16 address,
		u8* frameData, int rows, int cols)
/***********************************************/
{
	burstReadByCommandAndAddress(0xD0, address, frameData, 2*rows*cols + 1);

}

/***********************************************/
void TChipApi::burstReadByCommandAndAddress(u8 cmd,
		u16 address, u8* data, int len)
/***********************************************/
{
	slDev->write3ReadN(cmd, address >> 8, address & 0x00ff, data, len);

}


