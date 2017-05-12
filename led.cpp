/*
 * led.cpp
 *
 *  Created on: 27. 4. 2017
 *      Author: jiri svrcek
 */

#include "led.h"

#include "com-master.h"

#define ONBOARD_LED					D7
#define BLINK_DURATION				600	// milliseconds
#define BLINK_PERIOD				100	// milliseconds

//system_tick_t com_timeout = 0;
//system_tick_t blink_timeout = 0;
//system_tick_t badcmd_com_timeout = 0;
//system_tick_t badcmd_blink_timeout = 0;

/***********************************************/
void Led_On(u8 led_name)
/***********************************************/
{
	digitalWrite(led_name, HIGH);
}

/***********************************************/
void Led_Off(u8 led_name)
/***********************************************/
{
	digitalWrite(led_name, LOW);
}

/***********************************************/
void Led_Toggle(u8 led_name)
/***********************************************/
{
	digitalWrite(led_name, !digitalRead(led_name));
}

/***********************************************/
void Led_Configure(void)
/***********************************************/
{
	pinMode(ONBOARD_LED, OUTPUT);
}

/***********************************************/
void Led_TurnAllOff(void)
/***********************************************/
{
	digitalWrite(ONBOARD_LED, LOW);
}

/***********************************************/
void Led_TurnAllOn(void)
/***********************************************/
{
	digitalWrite(ONBOARD_LED, HIGH);
}

/***********************************************/
void Led_Blink(int blink_count)
/***********************************************/
{
	for (int i = 0; i < blink_count; i++)
	{
		Led_TurnAllOn();
		delay(50);
		Led_TurnAllOff();
		delay(50);
	}
}

/***********************************************/
void Led_TakeControllOfRgb(void)
/***********************************************/
{
	RGB.control(true);
}

/***********************************************/
void Led_GiveBackControllOfRgb(void)
/***********************************************/
{
	RGB.control(false);
}

/***********************************************/
void Led_ManageAll(TDongleState *dongleState)
/***********************************************/
{
	static system_tick_t blink_stop_time;
	static system_tick_t blink_next_toggle_time;

	//static system_tick_t rgb_timeout;

	// RGB Led:
	//----------------------------------
	// Wifi not connected	: 	blinking
	// Wifi connected		:	steady
	// I2C mode				:	green
	// SPI mode				: 	blue

	// Blue Led:
	//---------------------------------------
	// Host not connected	: 	off
	// Host connected		:	on
	// Host command ok		:	slow blinking
	// Host command error	:	fast blinking


	if (dongleState->slave_mode == SLAVE_MODE_I2C)
	{
		RGB.color(0, 255, 0);
	}
	if (dongleState->slave_mode == SLAVE_MODE_SPI)
	{
		RGB.color(0, 0, 255);
	}

	if ((dongleState->tcp_port_state == PORT_CLOSED) && (dongleState->vcp_port_state == PORT_CLOSED))
	{
		Led_Off(ONBOARD_LED);
	}
	else // at least one port opened
	{
		if (dongleState->command_type == GOOD_COMMAND)
		{
			dongleState->command_type = NO_COMMAND;
			blink_stop_time = millis() + BLINK_DURATION;
			blink_next_toggle_time = millis() + BLINK_PERIOD;
		}
		else if (dongleState->command_type == BAD_COMMAND)
		{
			dongleState->command_type = NO_COMMAND;
			blink_stop_time = millis() + BLINK_DURATION;
			blink_next_toggle_time = millis() + BLINK_PERIOD / 4;
		}

		if (millis() < blink_stop_time)
		{
			if (millis() > blink_next_toggle_time)
			{
				Led_Toggle(ONBOARD_LED);
				blink_next_toggle_time += BLINK_PERIOD;
			}
		}
		else
		{
			Led_On(ONBOARD_LED);
		}
	}
}

/******************************/
void Led_Circus(u32 circusTime)
/******************************/
{
	system_tick_t timeEnd = millis() + circusTime;
	while (timeEnd > millis())
	{
		RGB.color(random(0,255*255*255));
		delay(20);
		RGB.color(0);
		delay(80);
	}
}
