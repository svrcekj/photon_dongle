/*
 * application.cpp
 *
 *  Created on: 6. 3. 2017
 *      Author: Jiri
 */

#include "global.h"
#include "led.h"
#include "spi.h"
#include "i2c.h"
#include "help.h"
#include "parser.h"
#include "hw-specific.h"
#include "message.h"
#include "request.h"
#include "reply.h"

#include "application.h"

#include "chip-api.h"
#include "com-master.h"
#include "stm32f2xx.h"
#include "stm32f2xx_dma.h"
#include "stm32f2xx_gpio.h"
#include "stm32f2xx_spi.h"
#include "stm32f2xx_rcc.h"
#include "hw_config.h"

/*-----------------------------------------------------------------------------------------------*/

const int rows = ROWS;
const int cols = COLS;

TCPServer server = TCPServer(SERVER_PORT);
TCPClient client = TCPClient(SERVER_PORT);

IPAddress myIP;

ComMaster comMaster = ComMaster(DEFAULT_SLAVE_MODE);
TChipApi chipApi = TChipApi(&comMaster);

StdProtocolRequest staticUsbRequest;
StdProtocolRequest staticWifiRequest;

StdProtocolRequest *usbRequest = &staticUsbRequest;
StdProtocolRequest *wifiRequest = &staticWifiRequest;

StdProtocolReply staticUsbReply;
StdProtocolReply staticWifiReply;

StdProtocolReply *usbReply = &staticUsbReply;
StdProtocolReply *wifiReply = &staticWifiReply;

TDongleState dongleState;

bool single_click, double_click, tripple_click;

/*-----------------------------------------------------------------------------------------------*/

SYSTEM_MODE(MANUAL);

/*-----------------------------------------------------------------------------------------------*/
void switchToSpiMode();
void switchToI2cMode();
slave_mode_t toggleSlaveMode();
void initObjects();
void WifiOn();
void WifiOff();
void button_handler(system_event_t event, int duration, void* );


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

#if USE_RSTB_HW_RESET
	pinMode(RSTB_PIN, OUTPUT);
	digitalWrite(RSTB_PIN, LOW);
	delayMicroseconds(10000);
	digitalWrite(RSTB_PIN, HIGH);
	delayMicroseconds(10000);
#endif

	Led_Configure();
	Led_Blink(3);

	initObjects();

	USBSerial1.begin(DUMMY_VCP_SPEED);

	WifiOff();

/*
#if DO_NOT_USE_WIFI
	wifiSuspended = true;
	WifiOff();
#else
	wifiSuspended = false;
	WifiOn();
#endif
*/
	System.on(button_status, button_handler);
}

/*********************************************************************/
/**********************/    void loop()     /*************************/
/*********************************************************************/
{
#if !DO_NOT_USE_WIFI
	//---------------------------------
	// Process WiFi incomming message
	//---------------------------------
	if (int dataWaiting = client.available() > 0)
		for (int i = 0; i < dataWaiting; ++i)
			wifiRequest->addByte(client.read());

	if (wifiRequest->isCompleted())
		wifiRequest->processNew(&dongleState);

	if (!client.connected())
		client = server.available();

	dongleState.tcp_port_state = client.connected() ? PORT_OPENED : PORT_CLOSED;
#endif

	//---------------------------------------------
	// Process USB-VCP incomming message
	//---------------------------------------------
	while (USBSerial1.available())
		usbRequest->addByte(USBSerial1.read());

	if (usbRequest->isCompleted())
		usbRequest->processNew(&dongleState);

	dongleState.vcp_port_state = USBSerial1.isConnected() ? PORT_OPENED : PORT_CLOSED;

	//-------------------
	// Button management
	//-------------------
	// NOTE: single, double and tripple click detection
	// is part of a button_handler() routine

	if (tripple_click == true)
	{
		System.dfu();
		tripple_click = false;
	}
	else if (double_click == true)
	{
		delay(1000);
		if (tripple_click == false)
		{
			WifiOn();
			double_click = false;
		}
	}
	else if (single_click == true)
	{
		toggleSlaveMode();
		single_click = false;
	}
	//-------------------
	// LED management
	//-------------------
	Led_ManageAll(&dongleState);
}

//==============================================================================================================================
//==================================================   E N D   O F   L O O P   =================================================
//==============================================================================================================================

void button_handler(system_event_t event, int duration, void* )
{
	static system_tick_t press1;
	static system_tick_t delta01, delta12;

	if (duration == 0) // button just pressed
	{
		delta01 = millis() - press1;
		press1 = millis();

/*
		single_click = false;
		double_click = false;
		tripple_click = false;
*/

		if ((delta01 < 200) && (delta12 < 200))
			tripple_click = true;
		else if (delta01 < 200)
			double_click = true;
		else
			single_click = true;

		delta12 = delta01;
	}
}


/***********************************************/
void initObjects(void) // Software initialization
/***********************************************/
{
	usbRequest->assignReply(usbReply);
	wifiRequest->assignReply(wifiReply);

	usbReply->assignRequest(usbRequest);
	wifiReply->assignRequest(wifiRequest);

	dongleState.master_mode = DEFAULT_MASTER_MODE;
	dongleState.slave_mode = DEFAULT_SLAVE_MODE;
	dongleState.msg_protocol = DEFAULT_MSG_PROTOCOL;
	dongleState.i2cSpeed = DEFAULT_I2C_SPEED;
	dongleState.spiSpeed = DEFAULT_SPI_SPEED;
	dongleState.dataChunkSize = DEFAULT_CHUNK_SIZE;
	dongleState.command_type = NO_COMMAND;
	dongleState.wifi = WIFI_NOT_CONNECTED;
	dongleState.tcp_port_state = PORT_CLOSED;
	dongleState.vcp_port_state = PORT_CLOSED;

	if (dongleState.msg_protocol == MSG_PROTOCOL_STANDARD_COM_MASTER)
	{
		//usbRequest = usbStdProtocolRequest;
	}
	else //if (dongleState.msg_protocol == MSG_PROTOCOL_GRAY_DONGLE)
	{
		//usbRequest = usbGrayDongleRequest;
	}

	if (dongleState.slave_mode == SLAVE_MODE_SPI)
	{
		dongleState.spiSpeed = Spi_Configure(DEFAULT_SPI_SPEED, &dongleState);
	}
	else
	{
		dongleState.i2cSpeed = I2c_Configure(DEFAULT_I2C_SPEED);
	}

	chipApi.init(dongleState.slave_mode);
}

/***********************************************/
void switchToSpiMode()
/***********************************************/
{
	dongleState.slave_mode = SLAVE_MODE_SPI;
	Spi_Configure(dongleState.spiSpeed, &dongleState);
	chipApi.init(dongleState.slave_mode);
}

/***********************************************/
void switchToI2cMode()
/***********************************************/
{
	dongleState.slave_mode = SLAVE_MODE_I2C;
	I2c_Configure(dongleState.i2cSpeed);
	chipApi.init(dongleState.slave_mode);
}

/***********************************************/
slave_mode_t toggleSlaveMode()
/***********************************************/
{
	if (dongleState.slave_mode == SLAVE_MODE_SPI)
	{
		switchToI2cMode();
		return SLAVE_MODE_I2C;
	}
	else
	{
		switchToSpiMode();
		return SLAVE_MODE_SPI;
	}
}

/***********************************************/
void WifiOn()
/***********************************************/
{
	Led_GiveBackControllOfRgb();
	WiFi.on();
	WiFiAccessPoint ap[5];

	int nrOfRecords = WiFi.getCredentials(ap, 5);

	bool ssid_found = false;

	for (int i = 0; i < nrOfRecords; ++i)
		if (String(ap[i].ssid).compareTo("PhotonSpace") == 0)
			ssid_found = true;

	if (!ssid_found)
		WiFi.setCredentials("PhotonSpace", "universe", WPA2, WLAN_CIPHER_AES);

	WiFi.connect(WIFI_CONNECT_SKIP_LISTEN);
	WiFi.useDynamicIP();
	myIP = WiFi.localIP();

}

/***********************************************/
void WifiOff()
/***********************************************/
{
	WiFi.off();
	Led_TakeControllOfRgb();
}
