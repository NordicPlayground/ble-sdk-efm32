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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "bsp.h"
#include "bsp_trace.h"

//Nordic includes
#include "lib_aci.h"
#include "hal_platform.h"
#include "hal_aci_tl.h"
#include "aci_setup.h"

/**
Put the nRF8001 setup in the RAM of the nRF8001.
*/
#include "services.h"
/**
Include the services_lock.h to put the setup in the OTP memory of the nRF8001.
This would mean that the setup cannot be changed once put in.
However this removes the need to do the setup of the nRF8001 on every reset.
*/


#ifdef SERVICES_PIPE_TYPE_MAPPING_CONTENT
    static services_pipe_type_mapping_t
        services_pipe_type_mapping[NUMBER_OF_PIPES] = SERVICES_PIPE_TYPE_MAPPING_CONTENT;
#else
    #define NUMBER_OF_PIPES 0
    static services_pipe_type_mapping_t * services_pipe_type_mapping = NULL;
#endif
static hal_aci_data_t setup_msgs[NB_SETUP_MESSAGES] = SETUP_MESSAGES_CONTENT;

//@todo have an aci_struct that will contain
// total initial credits
// current credit
// current state of the aci (setup/standby/active/sleep)
// open remote pipe pending
// close remote pipe pending
// Current pipe available bitmap
// Current pipe closed bitmap
// Current connection interval, slave latency and link supervision timeout
// Current State of the the GATT client (Service Discovery)
// Status of the bond (R) Peer address
static struct aci_state_t aci_state;
static hal_aci_evt_t aci_data;
//static hal_aci_data_t aci_cmd;


/* Define how assert should function in the BLE library */
void __ble_assert(const char *file, uint16_t line)
{
  printf("ERROR ");
  printf("%s", file);
  printf(": ");
  printf("%d", line);
  printf("\n");
  while(1);
}

#define ACI_MOSI  0
#define ACI_MISO  1
#define ACI_SCLK  2
#define ACI_REQN  3
#define ACI_RDYN  5
#define ACI_RESET 6

void setupACI(void)
{ 
  printf("ACI setup\n");

  enableClocksForAci();
  
  /**
  Point ACI data structures to the the setup data that the nRFgo studio generated for the nRF8001
  */
  if (NULL != services_pipe_type_mapping)
  {
    aci_state.aci_setup_info.services_pipe_type_mapping = &services_pipe_type_mapping[0];
  }
  else
  {
    aci_state.aci_setup_info.services_pipe_type_mapping = NULL;
  }
  aci_state.aci_setup_info.number_of_pipes    = NUMBER_OF_PIPES;
  aci_state.aci_setup_info.setup_msgs         = setup_msgs;
  aci_state.aci_setup_info.num_setup_msgs     = NB_SETUP_MESSAGES;

  /*
  Tell the ACI library, the MCU to nRF8001 pin connections.
  The Active pin is optional and can be marked UNUSED
  */
  aci_state.aci_pins.board_name = BOARD_DEFAULT; //See board.h for details
  aci_state.aci_pins.reqn_pin   = ACI_REQN;
  aci_state.aci_pins.rdyn_pin   = ACI_RDYN;
  aci_state.aci_pins.mosi_pin   = ACI_MOSI;
  aci_state.aci_pins.miso_pin   = ACI_MISO;
  aci_state.aci_pins.sck_pin    = ACI_SCLK;

  aci_state.aci_pins.spi_clock_divider     = 0;
  
  aci_state.aci_pins.reset_pin             = ACI_RESET;
  aci_state.aci_pins.active_pin            = UNUSED;
  aci_state.aci_pins.optional_chip_sel_pin = UNUSED;

  aci_state.aci_pins.interface_is_interrupt = false;
  aci_state.aci_pins.interrupt_number       = 1;
  
  /* We initialize the data structures required to setup the nRF8001
  */
  //The second parameter is for turning debug printing on for the ACI Commands and Events 
  //so they be printed on the Serial
  lib_aci_init(&aci_state, true);
  
  printf("nRF8001 Reset done\n");
}

//###############################################################################

/**************************************************************************//**
 * @brief  Main function
 *****************************************************************************/
int main(void)
{
  /* Chip errata */
  CHIP_Init();

  /* If first word of user data page is non-zero, enable eA Profiler trace */
  BSP_TraceProfilerSetup();

  /*Setup SWO output for printing*/
  setupSWO();

  /* Setup SysTick Timer for 1 msec interrupts  */
  if (SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000)) while (1) ;

  /* Enable ACI lib */
  setupACI();

  /* Infinite blink loop */
  while (1)
  {
static bool setup_required = false;

  // We enter the if statement only when there is a ACI event available to be processed
  if (lib_aci_event_get(&aci_state, &aci_data))
  {
    aci_evt_t * aci_evt;

    aci_evt = &aci_data.evt;
    switch(aci_evt->evt_opcode)
    {
      /**
      As soon as you reset the nRF8001 you will get an ACI Device Started Event
      */
      case ACI_EVT_DEVICE_STARTED:
      {
        aci_state.data_credit_available = aci_evt->params.device_started.credit_available;
        switch(aci_evt->params.device_started.device_mode)
        {
          case ACI_DEVICE_SETUP:
            /**
            When the device is in the setup mode
            */
            printf("Evt Device Started: Setup\n");
            setup_required = true;
            break;

          case ACI_DEVICE_STANDBY:
            printf("Evt Device Started: Standby\n");
            if (aci_evt->params.device_started.hw_error)
            {
              delay(20); //Magic number used to make sure the HW error event is handled correctly.
            }
            else
            {
            lib_aci_connect(180/* in seconds */, 0x0100 /* advertising interval 100ms*/);
            printf("Advertising started\n");
            }
            break;
        }
      }
        break; //ACI Device Started Event

      case ACI_EVT_CMD_RSP:
        //If an ACI command response event comes with an error -> stop
        if (ACI_STATUS_SUCCESS != aci_evt->params.cmd_rsp.cmd_status)
        {
          //ACI ReadDynamicData and ACI WriteDynamicData will have status codes of
          //TRANSACTION_CONTINUE and TRANSACTION_COMPLETE
          //all other ACI commands will have status code of ACI_STATUS_SCUCCESS for 
          //a successful command
          printf("ACI Command ");
          printf("%x", aci_evt->params.cmd_rsp.cmd_opcode);
          printf("Evt Cmd respone: Error. Arduino is in an while(1); loop\n");
          while (1);
        }
        break;

      case ACI_EVT_CONNECTED:
        printf("Evt Connected\n");
        break;

      case ACI_EVT_PIPE_STATUS:
        printf("Evt Pipe Status\n");
        break;

      case ACI_EVT_DISCONNECTED:
        printf("Evt Disconnected/Advertising timed out\n");
        lib_aci_connect(180/* in seconds */, 0x0100 /* advertising interval 100ms*/);
        printf("Advertising started\n");
        break;

      case ACI_EVT_PIPE_ERROR:
        //See the appendix in the nRF8001 Product Specication for details on the error codes
        printf("ACI Evt Pipe Error: Pipe #:");
        printf("%d", aci_evt->params.pipe_error.pipe_number);
        printf("  Pipe Error Code: 0x");
        printf("%x\n", aci_evt->params.pipe_error.error_code);

        //Increment the credit available as the data packet was not sent.
        //The pipe error also represents the Attribute protocol Error Response sent from the peer 
        //and that should not be counted for the credit.
        if (ACI_STATUS_ERROR_PEER_ATT_ERROR != aci_evt->params.pipe_error.error_code)
        {
          aci_state.data_credit_available++;
        }
        break;

      case ACI_EVT_DATA_RECEIVED:
        printf("Pipe #: 0x");
        printf("%x", aci_evt->params.data_received.rx_data.pipe_number);
        {
          int i=0;
          printf(" Data(Hex) : ");
          for(i=0; i<aci_evt->len - 2; i++)
          {
            printf("%x", aci_evt->params.data_received.rx_data.aci_data[i]);
            printf(" ");
          }
        }
        printf("\n");
        break;

      case ACI_EVT_HW_ERROR:
        printf("HW error: ");
        printf("%d", aci_evt->params.hw_error.line_num);

        for(uint8_t counter = 0; counter <= (aci_evt->len - 3); counter++)
        {
        //Serial.write(aci_evt->params.hw_error.file_name[counter]); //uint8_t file_name[20];
        }
        printf("\n");
        lib_aci_connect(180/* in seconds */, 0x0050 /* advertising interval 50ms*/);
        printf("Advertising started");
        break;
    }
  }
  else
  {
    //Serial.println(F("No ACI Events available"));
    // No event in the ACI Event queue
    // Arduino can go to sleep now
    // Wakeup from sleep from the RDYN line
  }

  /* setup_required is set to true when the device starts up and enters setup mode.
   * It indicates that do_aci_setup() should be called. The flag should be cleared if
   * do_aci_setup() returns ACI_STATUS_TRANSACTION_COMPLETE.
   */
  if(setup_required)
  {
    if (SETUP_SUCCESS == do_aci_setup(&aci_state))
    {
      setup_required = false;
    }
  }
  }
}
