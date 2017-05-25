/*
 * spi.hpp
 *
 *  Created on: 27. 4. 2017
 *      Author: jiri svrcek
 */

#ifndef USER_APPLICATIONS_DONGLE2_SPI_H_
#define USER_APPLICATIONS_DONGLE2_SPI_H_

#include <application.h>
#include "com-master.h"

#define USE_DMA						1

#define DEFAULT_SPI_SPEED   		3750000

#define DEFAULT_SPI_TIMEOUT			50 // milliseconds

u32 Spi_Configure(u32 requested_speed, TDongleState *dongleState);
u8 Spi_GetPrescallerFromSpeed(u32 speed);

#if USE_DMA
void Dma_Configure(u8 *destPtr);
#endif

void Spi_BeginTx(void);
void Spi_EndTx(void);
void Spi_BeginRx(void);
void Spi_EndRx(void);
void Spi_EnableAndBeginRx(void);
u8 Spi_ReadByteFromActiveChannel(void);
void Spi_WriteByteToActiveChannel(u8 b);
void Spi_ReadByDma(u8* data, int len);
inline void Spi_FlushRxBuffer(void);

#endif /* USER_APPLICATIONS_DONGLE2_SPI_H_ */
