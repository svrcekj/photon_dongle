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
#define NRST_PIN            		D6
#define MODE_PIN            		D5
#define INTB_PIN           		 	D4
#define DEBUG_PIN           		D0

#define SERVER_PORT					23

#define MAX_READ_CNT 				4096

#define DEFAULT_CHUNK_SIZE  		512

#define FORCE_NRST_LOW      		1

#define DO_NOT_USE_WIFI				1
#define BUTTON_RECOVERS_PHOTON		0

#define DEFAULT_SLAVE_MODE  		SLAVE_MODE_SPI
#define DEFAULT_MASTER_MODE  		MASTER_MODE_USB

#define	ROWS						24
#define COLS						14

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
	MASTER_MODE_USB,
	MASTER_MODE_WIFI
} master_mode_t;

typedef enum {
	SLAVE_MODE_I2C,
	SLAVE_MODE_SPI
} slave_mode_t;

typedef enum {
	ZERO_EVENT = 0,
	NON_ZERO_EVENT = 1
} event_t;

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

typedef struct {
	wifi_state_t wifi;
	port_state_t tcp_port_state;
	port_state_t vcp_port_state;
	command_type_t command_type;
	master_mode_t master_mode;
	slave_mode_t slave_mode;
	u8 spiPrescaller;
	u32 spiSpeed;
	u32 i2cSpeed;
	u32 dataChunkSize;
} TDongleState;

#endif /* USER_APPLICATIONS_DONGLE2_GLOBAL_H_ */
