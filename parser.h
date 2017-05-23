/*
 * parser.hpp
 *
 *  Created on: 27. 4. 2017
 *      Author: jiri svrcek
 */

#ifndef USER_APPLICATIONS_DONGLE2_PARSER_H_
#define USER_APPLICATIONS_DONGLE2_PARSER_H_

#include "global.h"
#include <application.h>

#if 0
#define	SAFE_PARSER_ENABLED				1
#define TRANSLATE_SPI_TO_I2C_COMMANDS	1

#define GET_UART_SPEED					0x06
#define GET_SPI_INFO					0xA0
#define SET_SPI_SPEED					0xA1 // Set the baud prescaler
#define SET_IIC_SPEED					0x07
#define GET_IIC_SPEED					0x08
#define SET_IIC_SUB_ADDRESS				0x09
#define GET_IIC_SUB_ADDRESS				0x0a
#define FIND_IIC_SUB_ADDRESS			0x0b
#define GET_FIRMWARE_VERSION			0x0c
#define READ_REGISTER_BURST				0x11
#define WRITE_REGISTER_BURST			0x12
#define UART_MAGIC_BYTE					0x77 // Magic byte for FTSTool (application by Alex Hong)

#define BURST_ACK						0x00
#define BURST_NACK						0x01

inline bool charIsDigit(u8 b);

void SerialParser_ProcessGrayDongleCommand(u8 cmd, TDongleState *dongleState);
void SerialParser_ReportFw(u8 *version);
void SerialParser_AddByte(u8 buf);
void SerialParser_ReportMagicSequence(void);
void SerialParser_ReportDongleVersion(const u8 *version);
void SerialParser_ReportFixedAddress(u8 addr);
void SerialParser_ReportAddress(void);
void SerialParser_ReportSpeed(u32 speed);
void SerialParser_ReportSpiInfo(bool spiSelected, u8 spiPrescaller);
void SerialParser_SetSpeed(void);
void SerialParser_SetSpiSpeed(void);
void SerialParser_SetAddress(void);
void SerialParser_BurstRead(u8 *readData, slave_mode_t slave_mode);
void SerialParser_BurstWrite(void);

#if SAFE_PARSER_ENABLED
cmd_t Parser_GetCmdCode(u8 *b, int len);
#else
cmd_t Parser_DecodeCmd(u8 readBuf[], int readPtr);
#endif

//void Parser_AddToBuffer(u8 * buffer, int count);
void Parser_Reset(void);
int Parser_GetChunkSize(u8 val);
//u32 Parser_GetSpiSpeedFromCmd(u8 readBuf[], int readPtr);
//u32 Parser_GetChunkSizeFromCmd(u8 readBuf[], int readPtr);
u32 Parser_GetNumberFromString(u8 str[], int len);

#endif

#endif /* USER_APPLICATIONS_DONGLE2_PARSER_H_ */
