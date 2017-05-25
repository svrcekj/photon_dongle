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

bool wifiSuspended = true;


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

/*-----------------------------------------------------------------------------------------------*/

SYSTEM_MODE(MANUAL);

/*-----------------------------------------------------------------------------------------------*/
void Main_ProcessSwitchToUsbMode();
void Main_ProcessSwitchToWifiMode();
void Main_ProcessSwitchToSpiMode();
void Main_ProcessSwitchToI2cMode();
slave_mode_t Main_ProcessToggleSlaveMode();
void Main_ProcessToggleWifiOnOff();
void Main_ProcessEnableInterrupts();
void Main_ProcessDisableInterrupts();
void Main_RecoverPhoton();
void initObjects();

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


	if (wifiSuspended)
	{
		WiFi.off();
		Led_TakeControllOfRgb();
	}
	else
	{
#if DO_NOT_USE_WIFI
		WiFi.off();
#else
		WiFi.on();
		WiFi.connect();
		WiFi.useDynamicIP();
		myIP = WiFi.localIP();
		server.begin();
#endif
		Led_GiveBackControllOfRgb();
	}

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
	// NOTE: Part of button_handler()
	if (System.buttonPushed() > 2000)
	{
		RGB.color(255, 255, 0); // Yellow
		Led_Toggle(ONBOARD_LED);
		delay(50);
		if (System.buttonPushed() > 3000)
			System.dfu();

		while (System.buttonPushed() > 1000)
		{
			; // do nothing, waiting for button release
		}
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

void button_handler(system_event_t event, int duration, void* )
{
	static system_tick_t prev_release;
	system_tick_t last_release;
	system_tick_t time_between;
	bool double_click;

    if (!duration) // just pressed
    {
        RGB.color(0, 0, 0); // Gray
    }
    else // just released
    {
    	last_release = millis();
    	time_between = last_release - prev_release;
    	prev_release = last_release;

   		double_click = time_between > 200 ? false : true;

   		if (double_click)
   		{
   			Main_ProcessToggleWifiOnOff();
   		}
   		else
   		{
   			Main_ProcessToggleSlaveMode();
   		}
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
void Main_ProcessSwitchToUsbMode()
/***********************************************/
{
	USBSerial1.begin(DUMMY_VCP_SPEED);
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
	Spi_Configure(dongleState.spiSpeed, &dongleState);
	chipApi.init(dongleState.slave_mode);
}

/***********************************************/
void Main_ProcessSwitchToI2cMode()
/***********************************************/
{
	dongleState.slave_mode = SLAVE_MODE_I2C;
	I2c_Configure(dongleState.i2cSpeed);
	chipApi.init(dongleState.slave_mode);
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
void Main_ProcessToggleWifiOnOff()
/***********************************************/
{
	if (wifiSuspended)
	{
		Led_GiveBackControllOfRgb();
		WiFi.on();
		WiFi.connect();
		WiFi.useDynamicIP();
		myIP = WiFi.localIP();
		wifiSuspended = false;
	}
	else
	{
		WiFi.off();
		Led_TakeControllOfRgb();
		wifiSuspended = true;
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
	USBSerial1.begin(DUMMY_VCP_SPEED);
}
