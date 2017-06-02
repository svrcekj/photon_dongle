/*
 * global.h
 *
 *  Created on: 6. 3. 2017
 *      Author: Jiri
 */

#ifndef USER_APPLICATIONS_DONGLE2_GLOBAL_H_
#define USER_APPLICATIONS_DONGLE2_GLOBAL_H_

#include <application.h>

#define PHOTON_CORE_CLK				(120 * 1000 * 1000) // Hz
#define NRST_PIN            		D6 // RESET of MCU
#define MODE_PIN            		D5 // MODE select of FT chip (to select I2C or SPI)
#define INTB_PIN           		 	D4 // INTERRUPT input from FT chip
#define RSTB_PIN           			D3 // RESET of FT chip
#define DEBUG_PIN           		D0


#define SERVER_PORT					23

//#define MAX_READ_CNT 				4096

#define MAX_WRITE_LEN				256
#define MAX_READ_LEN				256

#define DEFAULT_CHUNK_SIZE  		512

#define FORCE_NRST_LOW      		1
#define USE_RSTB_HW_RESET			1
#define DO_NOT_USE_WIFI				0
#define BUTTON_RECOVERS_PHOTON		0

#define DEFAULT_SLAVE_MODE  		SLAVE_MODE_SPI
#define DEFAULT_MASTER_MODE  		MASTER_MODE_USB
#define DEFAULT_MASTER_INTERFACE	MASTER_INTERFACE_USB
#define DEFAULT_MSG_PROTOCOL		MSG_PROTOCOL_STANDARD_COM_MASTER

/*
#define	ROWS						24
#define COLS						14
*/

#define DUMMY_VCP_SPEED				115200

#define NR_OF_DUMMY_BYTES			1

const u8 FIRMWARE_VERSION[] = {0xF0, 0xB0};

typedef enum {
	ADDR_SIZE_8_BITS = 1,
	ADDR_SIZE_16_BITS = 2,
	ADDR_SIZE_24_BITS = 3,
	ADDR_SIZE_32_BITS = 4
} addr_size_t;

typedef enum {
	WIFI_NOT_CONNECTED,
	WIFI_CONNECTING,
	WIFI_CONNECTED,
} wifi_state_t;

typedef enum {
	PORT_CLOSED,
	PORT_OPENED
} port_state_t;

typedef enum {
	GOOD_COMMAND,
	BAD_COMMAND,
	NO_COMMAND
} command_type_t;

typedef enum {
	MASTER_INTERFACE_WIFI,
	MASTER_INTERFACE_USB
} masterInterface_t;

typedef enum {
	MASTER_MODE_USB,
	MASTER_MODE_WIFI
} master_mode_t;

typedef enum {
	SLAVE_MODE_I2C,
	SLAVE_MODE_SPI
} slave_mode_t;

typedef enum {
	MSG_PROTOCOL_GRAY_DONGLE,
	MSG_PROTOCOL_STANDARD_COM_MASTER,
	MSG_PROTOCOL_TEXT_BASED
} msg_protocol_t;

typedef enum {
	ZERO_EVENT = 0,
	NON_ZERO_EVENT = 1
} event_t;

typedef enum {
	STATUS_OK 					= 0x0000U,
	STATUS_ERROR				= 0x0001U,
	STATUS_WRITE_LEN_ERROR		= 0x0002U,
	STATUS_READ_LEN_ERROR		= 0x0003U
} stdReplyStatus_t;

typedef enum {
	PARSER_CMD_EXIT,
	PARSER_CMD_DFU,
	PARSER_CMD_SET_SPEED,
	PARSER_CMD_GET_SPEED,
	PARSER_CMD_SET_CHUNK,
	PARSER_CMD_GET_CHUNK,
	PARSER_CMD_USB_MODE,
	PARSER_CMD_WIFI_MODE,
	PARSER_CMD_I2C_MODE,
	PARSER_CMD_SPI_MODE,
	PARSER_CMD_INT_ON,
	PARSER_CMD_INT_OFF,
	PARSER_CMD_RESET,
	PARSER_CMD_INIT,
	PARSER_CMD_DUMMY,
	PARSER_CMD_FRAME,
	PARSER_CMD_HEATMAP,
	PARSER_CMD_RAW,
	PARSER_CMD_ID,
	PARSER_CMD_GENERAL_READ,
	PARSER_CMD_GENERAL_WRITE,
	PARSER_CMD_UNKNOWN
} cmd_t;

typedef enum {
    ACTION_WRITE        		= (uint16_t)0x0001U,
    ACTION_READ         		= (uint16_t)0x0002U,
    ACTION_WRITE_READ   		= (uint16_t)0x0003U,
    ACTION_GET_VERSION  		= (uint16_t)0x0004U,
	ACTION_GET_I2C_ADDRESS		= (uint16_t)0x0D00U,
	ACTION_SET_I2C_ADDRESS		= (uint16_t)0x0D80U,
	ACTION_GET_I2C_SPEED		= (uint16_t)0x0D01U,
	ACTION_SET_I2C_SPEED		= (uint16_t)0x0D81U,
	ACTION_GET_SPI_PRESCALLER	= (uint16_t)0x0D02U,
	ACTION_SET_SPI_PRESCALLER	= (uint16_t)0x0D82U,
	ACTION_GET_SPI_SPEED		= (uint16_t)0x0D03U,
	ACTION_SET_SPI_SPEED		= (uint16_t)0x0D83U,
	ACTION_SYSTEM_CTRL			= (uint16_t)0x1111U
} ProtocolAction;

typedef enum {
	NUMBER_FORMAT_BIG_ENDIAN,
	NUMBER_FORMAT_LITTLE_ENDIAN
} EndianFormat;

typedef struct {
	wifi_state_t wifi;
	port_state_t tcp_port_state;
	port_state_t vcp_port_state;
	command_type_t command_type;
	bool wifi_active;
	master_mode_t master_mode;
	slave_mode_t slave_mode;
	msg_protocol_t msg_protocol;
	u8 spiPrescaller;
	u32 spiSpeed;
	u32 i2cSpeed;
	u8 i2cAddress;
	u32 dataChunkSize;
} TDongleState;

#endif /* USER_APPLICATIONS_DONGLE2_GLOBAL_H_ */
