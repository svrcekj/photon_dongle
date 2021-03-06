/*
 * parser.cpp
 *
 *  Created on: 27. 4. 2017
 *      Author: jiri svrcek
 */

#include "parser.h"
#include "led.h"
#include "message.h"
#include "com-master.h"
#include "chip-api.h"

#if 0

extern ResponseMessage response;
extern RequestMessage serialRequest;
extern ComMaster comMaster;
extern TDongleState dongleState;
extern TChipApi chipApi;
extern u8 readData[1024 + NR_OF_DUMMY_BYTES];
u8 writeData[256]; // to be send over SPI / I2C



inline bool charIsDigit(u8 b)
{
	return ((b >= '0') && (b <= 9));
}

#if SAFE_PARSER_ENABLED
/***********************************************/
cmd_t Parser_GetCmdCode(u8 b[], int len)
/***********************************************/
{
	String cmd = String((char *)b).toLowerCase();

	if (cmd.startsWith("exit")) return PARSER_CMD_EXIT;
	if (cmd.startsWith("dfu")) return PARSER_CMD_DFU;
	if (cmd.startsWith("set speed")) return PARSER_CMD_SET_SPEED;
	if (cmd.startsWith("get speed")) return PARSER_CMD_GET_SPEED;
	if (cmd.startsWith("set chunk")) return PARSER_CMD_SET_CHUNK;
	if (cmd.startsWith("get chunk")) return PARSER_CMD_GET_CHUNK;
	if (cmd.startsWith("usb")) return PARSER_CMD_USB_MODE;
	if (cmd.startsWith("wifi")) return PARSER_CMD_WIFI_MODE;
	if (cmd.startsWith("iic")) return PARSER_CMD_I2C_MODE;
	if (cmd.startsWith("i2c")) return PARSER_CMD_I2C_MODE;
	if (cmd.startsWith("spi")) return PARSER_CMD_SPI_MODE;
	if (cmd.startsWith("int on")) return PARSER_CMD_INT_ON;
	if (cmd.startsWith("int off")) return PARSER_CMD_INT_OFF;
	if (cmd.startsWith("reset")) return PARSER_CMD_RESET;
	if (cmd.startsWith("init")) return PARSER_CMD_INIT;
	if (cmd.startsWith("dummy")) return PARSER_CMD_DUMMY;
	if (cmd.startsWith("frame")) return PARSER_CMD_FRAME;
	if (cmd.startsWith("heat")) return PARSER_CMD_HEATMAP;
	if (cmd.startsWith("raw")) return PARSER_CMD_RAW;
	if (cmd.startsWith("id")) return PARSER_CMD_ID;
	if (cmd.startsWith("-r")) return PARSER_CMD_GENERAL_READ;
	if (cmd.startsWith("-w")) return PARSER_CMD_GENERAL_WRITE;

	// For backward compactibility
	if (cmd.equals("speed")) return PARSER_CMD_GET_SPEED;
	if (cmd.equals("chunk")) return PARSER_CMD_GET_CHUNK;

	return PARSER_CMD_UNKNOWN;
}
#else
/*******************************************************/
cmd_t Parser_DecodeCmd(u8 readBuf[], int readPtr)
/*******************************************************/
{
	String got_cmd = String((char *)readBuf).trim().toLowerCase();

	for (int i = 0;; i++)
	{
		String table_cmd = String(cmd_str[i]);
		if (table_cmd.length() == 0)
		{
			return PARSER_CMD_UNKNOWN;
		}
		int k;
		while ((k = table_cmd.indexOf('?')) > -1)
		{
			table_cmd.setCharAt(k, got_cmd.charAt(k));
		}
		if (got_cmd.equalsIgnoreCase(table_cmd.substring(1, table_cmd.length())))
		{
			return (cmd_t) i;
		}
	}
	return PARSER_CMD_UNKNOWN;
}
#endif

/***********************************************/
u32 Parser_GetNumberFromString(u8 str[], int len)
/***********************************************/
{
	int val = 0;
	for (int i = 0; i < len; i++)
	{
		if ((str[i] >= '0') && (str[i] <= '9'))
			val = 10 * val + (str[i] - '0');
	}
	return val;
}

/**********************************************************************/
void SerialParser_ProcessGrayDongleCommand(u8 cmd, TDongleState *dongleState)
/**********************************************************************/
{
	switch (cmd)
	{
	case UART_MAGIC_BYTE:
		dongleState->command_type = GOOD_COMMAND;
		SerialParser_ReportMagicSequence();
		break;

	case GET_FIRMWARE_VERSION:
		dongleState->command_type = GOOD_COMMAND;
		SerialParser_ReportDongleVersion(FIRMWARE_VERSION);
		break;

	case FIND_IIC_SUB_ADDRESS:
		dongleState->command_type = GOOD_COMMAND;
		SerialParser_ReportFixedAddress(0x92);
		break;

	case SET_IIC_SPEED:
		dongleState->command_type = GOOD_COMMAND;
		SerialParser_SetSpeed();
		break;

	case GET_IIC_SPEED:
		dongleState->command_type = GOOD_COMMAND;
		SerialParser_ReportSpeed(dongleState->i2cSpeed);
		break;

	case SET_IIC_SUB_ADDRESS:
		dongleState->command_type = GOOD_COMMAND;
		SerialParser_SetAddress();
		break;

	case GET_IIC_SUB_ADDRESS:
		dongleState->command_type = GOOD_COMMAND;
		SerialParser_ReportAddress();
		break;

	case GET_SPI_INFO:
		dongleState->command_type = GOOD_COMMAND;
		SerialParser_ReportSpiInfo(
			dongleState->slave_mode == SLAVE_MODE_SPI ? true : false,
			dongleState->spiPrescaller);
		break;

	case SET_SPI_SPEED:
		dongleState->command_type = GOOD_COMMAND;
		SerialParser_SetSpiSpeed();
		break;

	case READ_REGISTER_BURST:
		dongleState->command_type = GOOD_COMMAND;
		SerialParser_BurstRead((u8 *)readData, dongleState->slave_mode);
		break;

	case WRITE_REGISTER_BURST:
		dongleState->command_type = GOOD_COMMAND;
		SerialParser_BurstWrite();
		break;

	default:
		dongleState->command_type = BAD_COMMAND;
		break;
	}
}

/*******************************************/
void SerialParser_ReportMagicSequence(void)
/*******************************************/
{
	response.init();
	response.addByte(0x4f);
	response.addByte(0x4b);
	response.send();
}

/****************************************************/
void SerialParser_ReportDongleVersion(const u8 *version)
/****************************************************/
{
	response.init();
	response.addByte(GET_FIRMWARE_VERSION);
	response.addByte(version[0]);
	response.addByte(version[1]);
	response.send();
}

/**********************************************/
void SerialParser_ReportFixedAddress(u8 addr)
/**********************************************/
{
	response.init();
	response.addByte(FIND_IIC_SUB_ADDRESS);
	response.addByte(1);
	response.addByte(addr);
	response.send();
}

/******************************************/
void SerialParser_ReportSpeed(u32 speed)
/******************************************/
{
	response.init();
	response.addByte(GET_IIC_SPEED);
	response.addByte(speed);
	response.addByte(speed >> 8);
	response.addByte(speed >> 16);
	response.addByte(speed >> 24);
	response.send();
}

/***********************************/
void SerialParser_SetSpeed(void)
/***********************************/
{
	//TODO: To be implemented
	//u32 serialSpeed = 0;
	//serialSpeed = serialRequest.pop32();
}

/***********************************/
void SerialParser_ReportAddress()
/***********************************/
{
	response.init();
	response.addByte(GET_IIC_SUB_ADDRESS);
	response.addByte(0x92);
	response.send();
}

/***********************************/
void SerialParser_SetAddress()
/***********************************/
{
	//TODO: To be implemented
}

/******************************************************************/
void SerialParser_ReportSpiInfo(bool spiSelected, u8 spiPrescaller)
/******************************************************************/
{
	response.init();
	response.addByte(GET_SPI_INFO);
	response.addByte((u8)spiSelected);
	response.addByte(spiPrescaller);
	response.send();
}

/***********************************/
void SerialParser_SetSpiSpeed(void)
/***********************************/
{
	// NOT IMPLEMENTED
}

/***************************************************************/
void SerialParser_BurstRead(u8 *readData, slave_mode_t slave_mode)
/***************************************************************/
{
	u8 wrData[5];
	u8 addrSize = request.getReadAddressSize();
	u8 cmd = request.getReadCommand();
	u16 len = request.getReadLen();
	u8 *b = request.getDataPointer();

#if TRANSLATE_SPI_TO_I2C_COMMANDS
	// SPI read commands translation
	if (slave_mode == SLAVE_MODE_SPI)
	{
		if (cmd == 0xb6)
			cmd = 0xb4;
		if (cmd == 0xb1)
			cmd = 0xb5;
	}
#endif

	wrData[0] = cmd;

	for (int i = 0; i < addrSize-1; i++)
		wrData[1 + i] = b[5+i];

#define LIMIT_ANSWER_LENGTH 1
#if LIMIT_ANSWER_LENGTH
	len = len > 256 ? 256 : len;
#endif
	comMaster.writeNReadN(wrData, addrSize, readData, len);

	response.init();
	response.addBurstReadHeader(BURST_ACK, len);
	response.addBytes(readData, len);
	response.send();
}

/***********************************/
void SerialParser_BurstWrite(void)
/***********************************/
{
	if (serialRequest.isResetCmd())
	{
		chipApi.init(dongleState.slave_mode);
	}
	else // normal write command processing
	{
		u16 len = serialRequest.getWriteLen();
		serialRequest.fillDataToBeWritten(writeData);
		comMaster.writeN(writeData, len);
	}
	response.confirmBurstWrite();
}

#endif
