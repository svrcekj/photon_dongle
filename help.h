/*
 * help.hpp
 *
 *  Created on: 27. 4. 2017
 *      Author: jiri svrcek
 */

#ifndef USER_APPLICATIONS_DONGLE2_HELP_H_
#define USER_APPLICATIONS_DONGLE2_HELP_H_

#include <application.h>

int help_getReadLenFromCommand(int digits, u8* readBuf);
u32 help_convertCharsToNumber(u8 ptr[],  int digitCount);


#endif /* USER_APPLICATIONS_DONGLE2_HELP_H_ */
