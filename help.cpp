/*
 * help.cpp
 *
 *  Created on: 27. 4. 2017
 *      Author: jiri svrcek
 */

#include "help.h"


/******************************************************************/
int help_getReadLenFromCommand(int digits, u8* readBuf)
/******************************************************************/
{
	int number = 0;

	if ((digits < 1) || (digits > 4))
	{
		return 0;
	}
	else if (digits == 1)
	{
		return readBuf[0] - '0';
	}
	else if (digits == 2)
	{
		number += (readBuf[0] - '0') * 10;
		number += (readBuf[1] - '0') * 1;
	}
	else if (digits == 3)
	{
		number += (readBuf[0] - '0') * 100;
		number += (readBuf[1] - '0') * 10;
		number += (readBuf[2] - '0') * 1;
	}
	else if (digits == 4)
	{
		number += (readBuf[0] - '0') * 1000;
		number += (readBuf[1] - '0') * 100;
		number += (readBuf[2] - '0') * 10;
		number += (readBuf[3] - '0') * 1;
	}
	return number;
}

/******************************************************************/
u32 help_convertCharsToNumber(u8 ptr[],  int digitCount)
/******************************************************************/
{
	u32 val = 0;

	for (int i = 0; i < digitCount; ++i)
	{
		val <<= 4;

		u8 b = ptr[i];

		val += ((b >= '0') && (b <= '9')) ? b - '0' :
			   ((b >= 'a') && (b <= 'f')) ? b - 'a' + 10:
			   ((b >= 'A') && (b <= 'A')) ? b - 'A' + 10:
			0;
	}
	return val;
}
