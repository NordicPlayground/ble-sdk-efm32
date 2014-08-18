/***************************************************************************//**
 * @file displaypalconfig.h
 * @brief Configuration file for PAL (Platform Abstraction Layer)
 * @author Energy Micro AS
 * @version 3.20.3
 *******************************************************************************
 * @section License
 * <b>(C) Copyright 2014 Silicon Labs, http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silicon Labs Software License Agreement. See 
 * "http://developer.silabs.com/legal/version/v11/Silicon_Labs_Software_License_Agreement.txt"  
 * for details. Before using this software for any purpose, you must agree to the 
 * terms of that agreement.
 *
 ******************************************************************************/


#ifndef _DISPLAY_PAL_CONFIG_H_
#define _DISPLAY_PAL_CONFIG_H_

/*
 * PAL SPI / USART configuration for the EFM32ZG_STK3200.
 * Select which USART and location is connected to the device via SPI.
 */
#define PAL_SPI_USART_UNIT     (USART1)
#define PAL_SPI_USART_CLOCK    (cmuClock_USART1)
#define PAL_SPI_USART_LOCATION (USART_ROUTE_LOCATION_LOC1)

#endif /* _DISPLAY_PAL_CONFIG_H_ */
