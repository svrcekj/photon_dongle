/*
 * led.cpp
 *
 *  Created on: 27. 4. 2017
 *      Author: jiri svrcek
 */

#include "led.h"

#include "com-master.h"

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
	static u32 blink_period;

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

	u8 red_color = (dongleState->wifi_active == false) ? 0 : (millis() & 0x00000400) == 0 ? 0 : 255;
	u8 green_color = dongleState->slave_mode == SLAVE_MODE_I2C ? 255 : 0;
	u8 blue_color = dongleState->slave_mode == SLAVE_MODE_SPI ? 255 : 0;

	RGB.color(red_color, green_color, blue_color);

	if ((dongleState->tcp_port_state == PORT_CLOSED) && (dongleState->vcp_port_state == PORT_CLOSED))
	{
		Led_Off(ONBOARD_LED);
	}
	else // at least one port opened
	{
		if (dongleState->command_type == GOOD_COMMAND)
		{
			dongleState->command_type = NO_COMMAND;
			blink_period = BLINK_PERIOD;
			if (millis() >= blink_stop_time) // previous state was iddle
			{
				blink_next_toggle_time = millis() + blink_period;
			}
			blink_stop_time = millis() + BLINK_DURATION;
		}
		else if (dongleState->command_type == BAD_COMMAND)
		{
			dongleState->command_type = NO_COMMAND;
			blink_period = BLINK_PERIOD / 4;
			if (millis() >= blink_stop_time) // previous state was iddle
			{
				blink_next_toggle_time = millis() + blink_period;
			}
			blink_stop_time = millis() + BLINK_DURATION;
		}

		if (millis() < blink_stop_time)
		{
			if (millis() > blink_next_toggle_time)
			{
				Led_Toggle(ONBOARD_LED);
				blink_next_toggle_time += blink_period;
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
