/*
 * application.cpp
 *
 *  Created on: 6. 3. 2017
 *      Author: Jiri
 */

/*-----------------------------------------------------------------------------------------------*/

#include "global.h"
#include "led.h"
#include "spi.h"
#include "i2c.h"
#include "help.h"
#include "parser.h"
#include "hw-specific.h"

#include "application.h"

#include "chip-api.h"
#include "com-master.h"
#include "stm32f2xx.h"
#include "stm32f2xx_dma.h"
#include "stm32f2xx_gpio.h"
#include "stm32f2xx_spi.h"
#include "stm32f2xx_rcc.h"
#include "hw_config.h"
#include "message.h"


/*-----------------------------------------------------------------------------------------------*/

const int rows = ROWS;
const int cols = COLS;

TCPServer server = TCPServer(SERVER_PORT);
TCPClient client = TCPClient(SERVER_PORT);

IPAddress myIP;

ComMaster comMaster = ComMaster(DEFAULT_SLAVE_MODE);
TChipApi chipApi = TChipApi(&comMaster);

IncommingMessage wifiRequest = IncommingMessage(256);
RequestMessage request;
ResponseMessage response;
TDongleState dongleState;

u8 readData[8192+1];
//u8 tx_buf[64]; // Serial output buffer

/*-----------------------------------------------------------------------------------------------*/

SYSTEM_MODE(MANUAL);

/*-----------------------------------------------------------------------------------------------*/


void Main_ProcessSetSpeedCmd(u8 readBuf[], int readPtr);
void Main_ProcessSetChunkCmd(u8 readBuf[], int readPtr);
void Main_ProcessSwitchToUsbMode();
void Main_ProcessSwitchToWifiMode();
void Main_ProcessSwitchToSpiMode();
void Main_ProcessSwitchToI2cMode();
slave_mode_t Main_ProcessToggleSlaveMode();
void Main_ProcessEnableInterrupts();
void Main_ProcessDisableInterrupts();
void Main_ProcessGeneralReadCmd(u8 readBuf[], int readPtr);
void Main_ProcessGeneralWriteCmd(u8 readBuf[], int readPtr);

void Main_RecoverPhoton();

void Tcp_SendData(u8 data[], int dataSize);
void Tcp_SendUInt32(u32 data);

void Other_SendDummyData(int len);

void button_handler(system_event_t event, int duration, void* );

/*****************************************************************************
 *
 *
 * 							  P a r s e r
 *
 *
 *****************************************************************************/


/***********************************************/
void Parser_AddToBuffer(u8 * buffer, int count)
/***********************************************/
{
	while (count--)
		wifiRequest.addByte(client.read());
}

/***********************************************/
int Parser_GetChunkSize(u8 val)
/***********************************************/
{
	const int chunkSizeArray[] = {4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048};
	return 	((val - '0') < 0) ? 2 :
			((val - '0') > 9) ? 4096 :
			chunkSizeArray[val - '0'];
}

/***********************************************/
void Parser_ProcessRequest(void)
/***********************************************/
{
	wifiRequest.terminate();

#if SAFE_PARSER_ENABLED
	cmd_t cmd = Parser_GetCmdCode(wifiRequest.toArray(), wifiRequest.length());
#else
	cmd_t cmd = Parser_DecodeCmd(wifiRequest.toArray(), wifiRequest.length());
#endif

	switch(cmd)
	{
		case PARSER_CMD_EXIT:
		{
			dongleState.command_type = GOOD_COMMAND;
			client.stop();
			wifiRequest.reset();
			break;
		}

		case PARSER_CMD_DFU:
		{
			System.dfu();
			// Here the program will be stopped anyway
			wifiRequest.reset();
			break;
		}

		case PARSER_CMD_SET_SPEED:
		{
			dongleState.command_type = GOOD_COMMAND;
			Main_ProcessSetSpeedCmd(wifiRequest.toArray(), wifiRequest.length());
			wifiRequest.reset();
			break;
		}

		case PARSER_CMD_GET_SPEED:
		{
			dongleState.command_type = GOOD_COMMAND;
			u32 speedToReport = (dongleState.slave_mode == SLAVE_MODE_SPI) ? dongleState.spiSpeed : dongleState.i2cSpeed;
			Tcp_SendUInt32(speedToReport);
			wifiRequest.reset();
			break;
		}

		case PARSER_CMD_SET_CHUNK:
		{
			dongleState.command_type = GOOD_COMMAND;
			Main_ProcessSetChunkCmd(wifiRequest.toArray(), wifiRequest.length());
			wifiRequest.reset();
			break;
		}

		case PARSER_CMD_GET_CHUNK:
		{
			dongleState.command_type = GOOD_COMMAND;
			Tcp_SendUInt32(dongleState.dataChunkSize);
			wifiRequest.reset();
			break;
		}

		case PARSER_CMD_USB_MODE:
		{
			dongleState.command_type = GOOD_COMMAND;
			Main_ProcessSwitchToUsbMode();
			wifiRequest.reset();
			break;
		}
		
		case PARSER_CMD_WIFI_MODE:
		{
			dongleState.command_type = GOOD_COMMAND;
			Main_ProcessSwitchToWifiMode();
			wifiRequest.reset();
			break;
		}
		
		case PARSER_CMD_SPI_MODE:
		{
			dongleState.command_type = GOOD_COMMAND;
			Main_ProcessSwitchToSpiMode();
			wifiRequest.reset();
			break;
		}
		
		case PARSER_CMD_I2C_MODE:
		{
			dongleState.command_type = GOOD_COMMAND;
			Main_ProcessSwitchToI2cMode();
			wifiRequest.reset();
			break;
		}
		
		case PARSER_CMD_INT_ON:
		{
			dongleState.command_type = GOOD_COMMAND;
			Main_ProcessEnableInterrupts();
			wifiRequest.reset();
			break;
		}
		
		case PARSER_CMD_INT_OFF:
		{
			dongleState.command_type = GOOD_COMMAND;
			Main_ProcessDisableInterrupts();
			wifiRequest.reset();
			break;
		}

		case PARSER_CMD_RESET:
		{	
			dongleState.command_type = GOOD_COMMAND;
			chipApi.init(dongleState.slave_mode);
			wifiRequest.reset();
			break;
		}

		case PARSER_CMD_INIT:
		{
			dongleState.command_type = GOOD_COMMAND;
			chipApi.init(dongleState.slave_mode);
			chipApi.readWaitingEvents();
			chipApi.getSysInfoRecord();
			chipApi.autoTune();
			delayMicroseconds(1000);
			chipApi.readWaitingEvents();
			chipApi.startScan();
			chipApi.readWaitingEvents();
			wifiRequest.reset();
			break;
		}

		case PARSER_CMD_ID:
		{
			dongleState.command_type = GOOD_COMMAND;
			u8 data[7];
			comMaster.write3ReadN(0xB4, 0x00, 0x04, data, 6+1);
			Tcp_SendData(&data[1], 6);
			wifiRequest.reset();
			break;
		}

		case PARSER_CMD_DUMMY:
		{
			dongleState.command_type = GOOD_COMMAND;
			int dataSize = wifiRequest.getNumber();
			dataSize = constrain(dataSize, 32, 4096);
			Other_SendDummyData(dataSize);
			wifiRequest.reset();
			break;
		}

		case PARSER_CMD_FRAME:
		{
			dongleState.command_type = GOOD_COMMAND;
			int dataSize = wifiRequest.getNumber();
			if (dataSize <= 0)
				dataSize =  rows * cols * 2;
			chipApi.burstReadByCommandAndAddress(0xD0, 0x47B4, readData, dataSize + 1);
			Tcp_SendData(&readData[1], dataSize);
			wifiRequest.reset();
			break;
		}

		case PARSER_CMD_HEATMAP:
		{
			dongleState.command_type = GOOD_COMMAND;
			const int dataSize = rows * cols * 2;
			chipApi.burstReadByCommandAndAddress(0xD0, 0x5000+32, readData, dataSize+1);
			Tcp_SendData(&readData[1], dataSize);
			comMaster.write1(0xAC);
			wifiRequest.reset();
			break;
		}

		case PARSER_CMD_GENERAL_READ:
		{
			dongleState.command_type = GOOD_COMMAND;
			Main_ProcessGeneralReadCmd(wifiRequest.toArray(), wifiRequest.length());
			wifiRequest.reset();
			break;
		}

		case PARSER_CMD_GENERAL_WRITE:
		{
			dongleState.command_type = GOOD_COMMAND;
			Main_ProcessGeneralWriteCmd(wifiRequest.toArray(), wifiRequest.length());
			wifiRequest.reset();
			break;
		}

		case PARSER_CMD_UNKNOWN:
		{
			dongleState.command_type = BAD_COMMAND;
			wifiRequest.reset();
			break;
		}

		default:
		{
			wifiRequest.reset();
		}
	}
}


/*-----------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/

/*************************************************************************************************/
void Main_ProcessSetSpeedCmd(u8 readBuf[], int readPtr)
/*************************************************************************************************/
{
	int newSpeed = Parser_GetNumberFromString(&readBuf[10], readPtr - 10);
	if (dongleState.slave_mode == SLAVE_MODE_SPI)
	{
		dongleState.spiSpeed = Spi_Configure(newSpeed, &dongleState);
	}
	else
	{
		dongleState.i2cSpeed = I2c_Configure(newSpeed);
	}
}

/*************************************************************************************************/
void Main_ProcessSetChunkCmd(u8 readBuf[], int readPtr)
/*************************************************************************************************/
{
	dongleState.dataChunkSize = Parser_GetNumberFromString(&readBuf[10], readPtr - 10);
}

/***********************************************/
void Main_ProcessSwitchToUsbMode()
/***********************************************/
{
	//USBSerial1.begin(9600);
	WiFi.off();
	Led_TakeControllOfRgb();
	RGB.color(255,255,255);
}

/***********************************************/
void Main_ProcessSwitchToWifiMode()
/***********************************************/
{
	USBSerial1.end();
	Led_GiveBackControllOfRgb();

	WiFi.on();
	WiFi.connect();
	WiFi.useDynamicIP();
	myIP = WiFi.localIP();
}

/***********************************************/
void Main_ProcessSwitchToSpiMode()
/***********************************************/
{
	dongleState.slave_mode = SLAVE_MODE_SPI;
	I2c_Configure(dongleState.spiSpeed);
}

/***********************************************/
void Main_ProcessSwitchToI2cMode()
/***********************************************/
{
	dongleState.slave_mode = SLAVE_MODE_I2C;
	I2c_Configure(dongleState.i2cSpeed);
}

/***********************************************/
slave_mode_t Main_ProcessToggleSlaveMode()
/***********************************************/
{
	if (dongleState.slave_mode == SLAVE_MODE_SPI)
	{
		Main_ProcessSwitchToI2cMode();
		return SLAVE_MODE_I2C;
	}
	else
	{
		Main_ProcessSwitchToSpiMode();
		return SLAVE_MODE_SPI;
	}
}

/***********************************************/
void Main_ProcessEnableInterrupts()
/***********************************************/
{

}

/***********************************************/
void Main_ProcessDisableInterrupts()
/***********************************************/
{

}


/************************************************************************************************/
void Main_ProcessGeneralReadCmd(u8 readBuf[], int readPtr)
/************************************************************************************************/
{
	//-------------------------------------------------------------------------------------------------
	// The possible syntax can be one of the following:
	//
	// -r <command> <number>
	// -r <command><address> <number>
	//
	// <command> can be any two digit hexadecimal number from 00 to FF without preceding 0x characters
	// <address> can be any two, four or eight digit hexadecimal number without preceding 0x characters
	// note: between command and address there is NO SPACE
	// <number> can be any decimal number (one, two, three or four digits long)
	//-------------------------------------------------------------------------------------------------

	// checking for "-r CC N" syntax:
	if ((readPtr > 6) && (readBuf[5] == ' '))
	{
		const u8 cmdByte = help_convertCharsToNumber(&readBuf[3], 2);
		const int readLen = help_getReadLenFromCommand(readPtr - 6, &readBuf[6]);
		if (readLen > 0)
		{
			comMaster.write1ReadN(cmdByte, readData, readLen);
			Tcp_SendData(readData, readLen);
		}
	}
	// checking for "-r CCAA N" syntax:
	else if ((readPtr > 8) && (readBuf[7] == ' '))
	{
		//TODO:
	}
	// checking for "-r CCAAAA N" syntax:
	else if ((readPtr > 10) && (readBuf[9] == ' '))
	{	
		const u8 cmdByte = help_convertCharsToNumber(&readBuf[3], 2);
		const u8 aHighByte = help_convertCharsToNumber(&readBuf[5], 2);
		const u8 aLowByte = help_convertCharsToNumber(&readBuf[7], 2);
		const int readLen = help_getReadLenFromCommand(readPtr - 10, &readBuf[10]);
		if (readLen > 0)
		{
			comMaster.write3ReadN(cmdByte, aHighByte, aLowByte, readData, readLen);
			Tcp_SendData(readData, readLen);
		}
	}
	// checking for "-r CCAAAAAAAA N" syntax:
	else if ((readPtr > 12) && (readBuf[11] == ' '))
	{
		const u8 cmdByte = help_convertCharsToNumber(&readBuf[3], 2);
		const u8 a0Byte = help_convertCharsToNumber(&readBuf[5], 2);
		const u8 a1Byte = help_convertCharsToNumber(&readBuf[7], 2);
		const u8 a2Byte = help_convertCharsToNumber(&readBuf[9], 2);
		const u8 a3Byte = help_convertCharsToNumber(&readBuf[11], 2);
		const int readLen = help_getReadLenFromCommand(readPtr - 14, &readBuf[14]);
		if (readLen > 0)
		{
			comMaster.write5ReadN(cmdByte, a0Byte, a1Byte, a2Byte, a3Byte, readData, readLen);
			Tcp_SendData(readData, readLen);
		}
	}
}

/************************************************************************************************/
void Main_ProcessGeneralWriteCmd(u8 readBuf[], int readPtr)
/************************************************************************************************/
{
	//limit the numbers of byte to be written to 32
	const int maxBytesToSend = 32;
	u8 arrayToSend[maxBytesToSend];
	int index = 0;
	for (int i = 2; i < readPtr; ++i)
	{
		if (readBuf[i] == ' ')
			continue;
		if (i < readPtr) // check if the next byte is within the array size
		{
			arrayToSend[index++] = help_convertCharsToNumber(&readBuf[i], 2);
			i++;
			if (index == maxBytesToSend)
				break;
		}
	}
	if (index > 0)
	{
		comMaster.writeN(arrayToSend, index);
	}
}

/***********************************************/
void Main_RecoverPhoton()
/***********************************************/
{
	Led_TakeControllOfRgb();
	USBSerial1.end();

#if FORCE_NRST_LOW
	HwSpecific_ForceNrstPinLow();
#endif
	HwSpecific_ForceModePin(dongleState.slave_mode);

	Led_Configure();
	Led_Blink(10);
	Led_GiveBackControllOfRgb();

	dongleState.slave_mode = SLAVE_MODE_SPI;
	dongleState.spiSpeed = Spi_Configure(DEFAULT_SPI_SPEED, &dongleState);

	WiFi.on();
	WiFi.connect();
	WiFi.useDynamicIP();
	myIP = WiFi.localIP();
}

/***********************************************/
void Other_SendDummyData(int len)
/***********************************************/
{
	for (int i = 0; i < len/2; i++)
	{
		readData[2*i] = i >> 8;
		readData[2*i+1] = i;
	}
	Tcp_SendData(readData, len);
}

/***********************************************/
void Tcp_SendData(u8 dataPtr[], int dataSize)
/***********************************************/
{
	int fullChunks = dataSize / dongleState.dataChunkSize;
	for (int i = 0; i < fullChunks; i++)
	{
		server.write(&dataPtr[i * dongleState.dataChunkSize], dongleState.dataChunkSize);
	}
	int lastChunkSize = dataSize % dongleState.dataChunkSize;
	if (lastChunkSize > 0)
	{
		server.write(&dataPtr[fullChunks * dongleState.dataChunkSize], lastChunkSize);
	}
}

/***********************************************/
void Tcp_SendUInt32(u32 data)
/***********************************************/
{
	u8 tmp[4];
	for (int i = 0; i < 4; i++)
	{
		tmp[i] = data >> ((3-i)*8);
	}
	Tcp_SendData(tmp, 4);
}


//==============================================================================================================================
//===========================================   M A I N    A N D    L O O P    =================================================
//==============================================================================================================================


/**********************************************************************/
/************************/  void setup()    /**************************/
/**********************************************************************/
{
	Led_TakeControllOfRgb();
	RGB.color(255,0,0);

	// Configure DEBUG_PIN as output to possibly indicate some events on logic analyzer
	pinMode(DEBUG_PIN, OUTPUT);
	digitalWrite(DEBUG_PIN, LOW);

#if FORCE_NRST_LOW
	HwSpecific_ForceNrstPinLow();
#endif

	HwSpecific_ForceModePin(dongleState.slave_mode);

	Led_Configure();
	Led_Blink(3);

	dongleState.master_mode = DEFAULT_MASTER_MODE;
	dongleState.slave_mode = DEFAULT_SLAVE_MODE;
	dongleState.i2cSpeed = DEFAULT_I2C_SPEED;
	dongleState.spiSpeed = DEFAULT_SPI_SPEED;
	dongleState.dataChunkSize = DEFAULT_CHUNK_SIZE;
	dongleState.command_type = NO_COMMAND;
	dongleState.wifi = WIFI_NOT_CONNECTED;
	dongleState.tcp_port_state = PORT_CLOSED;
	dongleState.vcp_port_state = PORT_CLOSED;

	if (dongleState.slave_mode == SLAVE_MODE_SPI)
	{
		dongleState.spiSpeed = Spi_Configure(DEFAULT_SPI_SPEED, &dongleState);
	}
	else
	{
		dongleState.i2cSpeed = I2c_Configure(DEFAULT_I2C_SPEED);
	}

	USBSerial1.begin(115200);
	RGB.color(255,255,255);

#if DO_NOT_USE_WIFI
	WiFi.off();
#else
	WiFi.on();
	WiFi.connect();
	WiFi.useDynamicIP();
	myIP = WiFi.localIP();
	server.begin();
	//udp.begin(8888);
#endif
	System.on(button_status, button_handler);
}


/************************/
void usbSerialEvent1()
/************************/
{
	while (USBSerial1.available())
		request.addByte(USBSerial1.read());
}


/*********************************************************************/
/**********************/    void loop()     /*************************/
/*********************************************************************/
{
#if !DO_NOT_USE_WIFI

	static int noDataCnt;

	//--------------------
	// WiFi incomming data
	//--------------------
	int dataWaiting = client.available();
	if (dataWaiting > 0)
	{
		Parser_AddToBuffer(wifiRequest.toArray(), wifiRequest.length());
	}
	else
	{
		if (wifiRequest.length() > 0)
		{
			Parser_ProcessRequest();
		}
		else
		{
			noDataCnt++;
			if (noDataCnt == 1000)
			{
				wifiRequest.reset();
				noDataCnt = 0;
			}
		}
	}
	if (!client.connected())
	{
		client = server.available();
		dongleState.tcp_port_state = PORT_CLOSED;
	}
	else
	{
		dongleState.tcp_port_state = PORT_OPENED;
	}
#endif

	//--------------------
	// USB incomming data
	//--------------------
	if (request.newCommandAvailable())
	{
		request.processNewCommand(&dongleState);
	}

	if (USBSerial1.isConnected())
	{
		dongleState.vcp_port_state = PORT_OPENED;
	}
	else
	{
		dongleState.vcp_port_state = PORT_CLOSED;
	}

	//-------------------
	// Button management
	//-------------------
	// NOTE: Part of button_handler()
	if (System.buttonPushed() > 1000)
	{
		RGB.color(255, 255, 0); // Gray
	}
	else if (System.buttonPushed() > 0)
	{
		RGB.color(0, 0, 0); // Gray
	}

	else
	{
		//-------------------
		// LED management
		//-------------------
		Led_ManageAll(&dongleState);
	}
}

//==============================================================================================================================
//==================================================   E N D   O F   L O O P   =================================================
//==============================================================================================================================


//================================ FROM PARTICLE FW DOC ==================================
// EXAMPLE USAGE
//========================================================================================

void button_handler(system_event_t event, int duration, void* )
{
#if 1
    if (!duration) // just pressed
    {
        RGB.color(10, 5, 10); // Gray
    }
    else // just released
    {
    	if (System.buttonPushed() > 1000)
    	{
    		System.dfu();
    	}
    	else if (System.buttonPushed() > 500)
    	{
#if BUTTON_RECOVERS_PHOTON
    		Main_RecoverPhoton();
#endif
    	}
    	else if (System.buttonPushed() > 10)
    	{
    		Main_ProcessToggleSlaveMode();
    	}
    }
#endif
}
