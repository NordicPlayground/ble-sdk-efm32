/* Copyright (c) 2014, Nordic Semiconductor ASA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdint.h>
#include <stdio.h>

#include "hal_platform.h"

#include "em_cmu.h"
#include "em_gpio.h"
#include "em_usart.h"
#include "em_int.h"

volatile uint32_t msTicks; /* counts 1ms timeTicks */

void SysTick_Handler(void)
{
  msTicks++;       /* increment counter necessary in delay()*/
}

void delay(uint32_t dlyTicks)
{
  uint32_t curTicks;

  curTicks = msTicks;
  while ((msTicks - curTicks) < dlyTicks) ;
}

/* Setup SWO*/
void setupSWO(void)
{
  /* Enable GPIO Clock. */
  CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_GPIO;
  /* Enable Serial wire output pin */
  GPIO->ROUTE |= GPIO_ROUTE_SWOPEN;
#if defined(_EFM32_GIANT_FAMILY) || defined(_EFM32_WONDER_FAMILY) || defined(_EFM32_LEOPARD_FAMILY)
  /* Set location 0 */
  GPIO->ROUTE = (GPIO->ROUTE & ~(_GPIO_ROUTE_SWLOCATION_MASK)) | GPIO_ROUTE_SWLOCATION_LOC0;

  /* Enable output on pin - GPIO Port F, Pin 2 */
  GPIO->P[5].MODEL &= ~(_GPIO_P_MODEL_MODE2_MASK);
  GPIO->P[5].MODEL |= GPIO_P_MODEL_MODE2_PUSHPULL;
#else
  /* Set location 1 */
  GPIO->ROUTE = (GPIO->ROUTE & ~(_GPIO_ROUTE_SWLOCATION_MASK)) | GPIO_ROUTE_SWLOCATION_LOC1;
  /* Enable output on pin */
  GPIO->P[2].MODEH &= ~(_GPIO_P_MODEH_MODE15_MASK);
  GPIO->P[2].MODEH |= GPIO_P_MODEH_MODE15_PUSHPULL;
#endif
  /* Enable debug clock AUXHFRCO */
  CMU->OSCENCMD = CMU_OSCENCMD_AUXHFRCOEN;

  while(!(CMU->STATUS & CMU_STATUS_AUXHFRCORDY));

  /* Enable trace in core debug */
  CoreDebug->DHCSR |= 1;
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

  /* Enable PC and IRQ sampling output */
  DWT->CTRL = 0x400113FF;
  /* Set TPIU prescaler to 16. */
  TPI->ACPR = 0xf;
  /* Set protocol to NRZ */
  TPI->SPPR = 2;
  /* Disable continuous formatting */
  TPI->FFCR = 0x100;
  /* Unlock ITM and output data */
  ITM->LAR = 0xC5ACCE55;
  ITM->TCR = 0x10009;
}

/* Enable the ARM compiler to send printf commands via the SWO interface*/

struct __FILE { int handle; /* Add whatever you need here */ };

FILE __stdout;
FILE __stdin;

int fputc(int ch, FILE *f)
{
  ITM_SendChar(ch);
  return(ch);
}

void enableClocksForAci(void)
{
  /* Enable clocks*/
  CMU_ClockEnable(cmuClock_HFPER, true);
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_USART1, true);
}

void pinMode(uint8_t pin, uint8_t pinMode)
{
  switch(pinMode)
  {
    case INPUT:
      GPIO_PinModeSet(gpioPortD, pin, gpioModeInput, 0);
    break;
  
    case INPUT_PULLUP:
      GPIO_PinModeSet(gpioPortD, pin, gpioModeInputPull, 0);
    break;
  
    case OUTPUT:
      GPIO_PinModeSet(gpioPortD, pin, gpioModePushPull, 0);
    break;
  }
}

uint8_t digitalRead(uint8_t pin)
{
  return GPIO_PinInGet(gpioPortD, pin);
}

void digitalWrite(uint8_t pin, uint8_t value)
{
  if(value)
  {
    GPIO_PinOutSet(gpioPortD, pin);
  }
  else 
  {
    GPIO_PinOutClear(gpioPortD, pin);
  }
}

uint8_t efm_spi_readwrite(const uint8_t aci_byte)
{
  return USART_SpiTransfer(USART1, aci_byte);
}

void attachInterrupt(uint8_t interruptNumber, void (*handlerPtr)(void), uint8_t mode)
{
  //TODO: attach interrupt
}

void detachInterrupt(uint8_t interruptNumber)
{
  //TODO: detach interrupt
}

void noInterrupts(void)
{
  INT_Disable();
}

void interrupts(void)
{
  INT_Enable();
}
