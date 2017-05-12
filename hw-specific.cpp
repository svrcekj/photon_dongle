/*
 * hw-specific.cpp
 *
 *  Created on: 2. 5. 2017
 *      Author: jiri svrcek
 */


#include "hw-specific.h"


/*****************************************************/
void HwSpecific_ForceModePin(slave_mode_t slave_mode)
/*****************************************************/
{
	pinMode(MODE_PIN, OUTPUT);
	if (slave_mode == SLAVE_MODE_I2C)
	{
		digitalWrite(MODE_PIN, HIGH);
	}
	else
	{
		digitalWrite(MODE_PIN, LOW);
	}
}

/***********************************************/
void HwSpecific_ReleaseModePin(void)
/***********************************************/
{
	pinMode(MODE_PIN, INPUT);
}

/***********************************************/
void HwSpecific_ForceNrstPinLow(void)
/***********************************************/
{
	pinMode(NRST_PIN, OUTPUT);
	digitalWrite(NRST_PIN, LOW);
}
