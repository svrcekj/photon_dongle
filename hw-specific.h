/*
 * hw-specific.h
 *
 *  Created on: 2. 5. 2017
 *      Author: jiri svrcek
 */

#ifndef USER_APPLICATIONS_DONGLE2_HW_SPECIFIC_H_
#define USER_APPLICATIONS_DONGLE2_HW_SPECIFIC_H_

#include "global.h"
#include <application.h>

void HwSpecific_ForceModePin(slave_mode_t slave_mode);
void HwSpecific_ReleaseModePin(void);
void HwSpecific_ForceNrstPinLow(void);



#endif /* USER_APPLICATIONS_DONGLE2_HW_SPECIFIC_H_ */
