/*
 * led.hpp
 *
 *  Created on: 27. 4. 2017
 *      Author: jiri svrcek
 */

#ifndef USER_APPLICATIONS_DONGLE2_LED_H_
#define USER_APPLICATIONS_DONGLE2_LED_H_

#include "global.h"
#include <application.h>

#define ONBOARD_LED					D7

void Led_On(u8 led_name);
void Led_Off(u8 led_name);
void Led_Toggle(u8 led_name);
void Led_Configure(void);
void Led_TurnAllOff(void);
void Led_TurnAllOn(void);
void Led_Blink(int blink_count);
void Led_TakeControllOfRgb(void);
void Led_GiveBackControllOfRgb(void);
void Led_ManageAll(TDongleState *dongleState);
void Led_Circus(u32 cirsusTime);

#endif /* USER_APPLICATIONS_DONGLE2_LED_H_ */
