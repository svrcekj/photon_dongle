/*
 * spi.cpp
 *
 *  Created on: 27. 4. 2017
 *      Author: jiri svrcek
 */

#include "spi.h"

#include "application.h"
#include "com-master.h"
#include "global.h"
#include "stm32f2xx.h"
#include "stm32f2xx_dma.h"
#include "stm32f2xx_gpio.h"
#include "stm32f2xx_spi.h"
#include "stm32f2xx_rcc.h"
#include "hw_config.h"

extern u8 *readData;

#define CS_UP				GPIO_SetBits(GPIOC, GPIO_Pin_2)
#define CS_DOWN				GPIO_ResetBits(GPIOC, GPIO_Pin_2)

#define START_SPI			CS_DOWN
#define STOP_SPI			CS_UP

#define WAIT_FOR_TX_DONE	while (SPI_I2S_GetFlagStatus(hspi, SPI_I2S_FLAG_TXE) == 0)
#define WAIT_FOR_RX_DONE	while (SPI_I2S_GetFlagStatus(hspi, SPI_I2S_FLAG_RXNE) == 0)
#define WAIT_FOR_SPI_READY	while (SPI_I2S_GetFlagStatus(hspi, SPI_I2S_FLAG_BSY) == 1)
#define WAIT_FOR_DMA_DONE	while (!DMA_GetFlagStatus(DMA2_Stream0, DMA_FLAG_TCIF0))
//#define FLUSH_RX_BUFF		while (SPI_I2S_GetFlagStatus(hspi, SPI_I2S_FLAG_RXNE) == 1) void(hspi->DR)

#define TOGGLE_PIN			{pinSetFast(DEBUG_PIN); pinResetFast(DEBUG_PIN);}

#define SET_SPI_RX			hspi->CR1 &= SPI_Direction_Rx
#define SET_SPI_TX			hspi->CR1 |= SPI_Direction_Tx

#define ENABLE_SPI			hspi->CR1 |= SPI_CR1_SPE
#define DISABLE_SPI			hspi->CR1 &= (uint16_t)~((uint16_t)SPI_CR1_SPE)


static SPI_InitTypeDef 		SPI_InitStructure;
static GPIO_InitTypeDef 	GPIO_InitStructure;
static DMA_InitTypeDef		DMA_InitStructure;
static SPI_TypeDef 			*hspi = (SPI_TypeDef *) SPI1_BASE;

/***********************************************************/
u8 Spi_GetPrescallerFromSpeed(u32 speed)
/***********************************************************/
{
	u32 divider = 4;
	u32 min_dist = PHOTON_CORE_CLK;
	int index = 0;

	for (int i = 0; i < 8; i++)
	{
		u32 current_speed = PHOTON_CORE_CLK / divider;
		int delta = current_speed - speed;
		u32 abs_delta = abs(delta);
		if (abs(delta) < min_dist)
		{
			min_dist = abs_delta;
			index = i;
		}
		divider <<= 1;
	}

	return index;
}

/****************************************************************/
u32 Spi_Configure(u32 requested_speed, TDongleState *dongleState)
/****************************************************************/
// GPIO configuration (MOSI, SCK, SS pin)
{
#if USE_DMA
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
	SPI_I2S_DMACmd(hspi, SPI_I2S_DMAReq_Rx, DISABLE);
#endif

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	// MOSI and SCK
	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_ResetBits(GPIOA, GPIO_Pin_5 | GPIO_Pin_7);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);

	//CS pin
	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_SetBits(GPIOC, GPIO_Pin_2);

	/* SPI configuration */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

	SPI_I2S_DeInit(hspi);

	SPI_StructInit(&SPI_InitStructure);
	SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;

	//Default: SPI_MODE0
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;

	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;

	u8 prescaller = Spi_GetPrescallerFromSpeed(requested_speed);
	SPI_InitStructure.SPI_BaudRatePrescaler = prescaller * 8;
	dongleState->spiPrescaller = prescaller;
	int retSpeed = PHOTON_CORE_CLK / (4 << prescaller);

	//Default: MSBFIRST
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;

	SPI_InitStructure.SPI_CRCPolynomial = 7;

	SPI_Init(hspi, &SPI_InitStructure);

#if USE_DMA
	Dma_Configure(readData);
#endif

	return retSpeed;
}

#if USE_DMA
/**********************************/
void Dma_Configure(u8 *destPtr)
/**********************************/
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

	DMA_DeInit(DMA2_Stream0);

	DMA_InitStructure.DMA_Channel = DMA_Channel_3;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(hspi->DR) ;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t) destPtr;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_BufferSize = 7;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

	DMA_Init(DMA2_Stream0, (DMA_InitTypeDef *) &DMA_InitStructure);

	SPI_I2S_DMACmd(hspi, SPI_I2S_DMAReq_Rx, ENABLE);
}
#endif

/******************/
void Spi_BeginTx()
/******************/
{
	//WAIT_FOR_TX_DONE;
	SET_SPI_TX; // MOSI set as output
	ENABLE_SPI;
	CS_DOWN; // SPI_CS low starts the SPI transaction
	//WAIT_FOR_SPI_READY;
	delayMicroseconds(5);
}

/******************/
void Spi_EndTx()
/******************/
{
	WAIT_FOR_TX_DONE;
	WAIT_FOR_SPI_READY; // wait for last byte fully transmitted
	delayMicroseconds(5);
	CS_UP; // SPI_CS high stops the SPI transaction
	DISABLE_SPI;
	delayMicroseconds(5);
}

/******************/
void Spi_BeginRx()
/******************/
{
	WAIT_FOR_TX_DONE;
	WAIT_FOR_SPI_READY; // wait for last byte fully transmitted
	Spi_FlushRxBuffer();
	delayMicroseconds(1);
	SET_SPI_RX;
}

/*********************************/
void Spi_EnableAndBeginRx()
/*********************************/
{
	//WAIT_FOR_TX_DONE;
	//WAIT_FOR_SPI_READY; // wait for last byte fully transmitted
	//Spi_FlushRxBuffer();
	delayMicroseconds(1);
	SET_SPI_RX;
	CS_DOWN; // SPI_CS low starts the SPI transaction
	delayMicroseconds(5);
	ENABLE_SPI;
}

/******************/
void Spi_EndRx()
/******************/
{
	//WAIT_FOR_RX_DONE;
	//WAIT_FOR_SPI_READY;
	CS_UP; // SPI_CS high stops the SPI transaction
	//delayMicroseconds(5);
	DISABLE_SPI;
	//SET_SPI_TX; // MOSI set as output
	delayMicroseconds(5);
}

/***********************************************
* Receives one byte from active SPI channel
************************************************/
u8 Spi_ReadByteFromActiveChannel(void)
{
	WAIT_FOR_RX_DONE;
	TOGGLE_PIN;
	return hspi->DR;
}

/***********************************************
* Sends one byte from active SPI channel
************************************************/
void Spi_WriteByteToActiveChannel(u8 b)
{
	WAIT_FOR_TX_DONE;
	hspi->DR = b;
}

/***********************************************************/
void Spi_ReadByDma(u8 *data, int len)
/***********************************************************/
{
	DMA2_Stream0->NDTR = len;
	DMA2_Stream0->M0AR = (uint32_t) data;
	DMA_Cmd(DMA2_Stream0, ENABLE);

	Spi_BeginRx();
	system_tick_t time_to_end = millis() + DEFAULT_SPI_TIMEOUT;
	while (!DMA_GetFlagStatus(DMA2_Stream0, DMA_FLAG_TCIF0))
	{
		if (millis() > time_to_end)
			break;
	}
	//TODO: Add safety timer to allow this wait to timeout
	Spi_EndRx();

	DMA_Cmd(DMA2_Stream0, DISABLE);
	DMA_ClearFlag(DMA2_Stream0, DMA_FLAG_TCIF0);
}

/***********************************************
* Flush the incoming SPI buffer
************************************************/
inline void Spi_FlushRxBuffer(void)
{
	uint16_t dummy;
	while (SPI_I2S_GetFlagStatus(hspi, SPI_I2S_FLAG_RXNE) == 1)
	{
		dummy = hspi->DR;
		(void) dummy;
	}
}
