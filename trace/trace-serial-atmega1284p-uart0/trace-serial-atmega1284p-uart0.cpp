/**
 * Snowfox is a modular RTOS with extensive IO support.
 * Copyright (C) 2017 - 2019 Alexander Entinger / LXRobotics GmbH
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**************************************************************************************
 * This example program is tailored for usage with Moteino-Mega-USB
 *
 * Upload via avrdude
 *   avrdude -p atmega1284p -c arduino -P /dev/ttyUSB0 -e -U flash:w:debug-serial-atmega1284p-uart0
 **************************************************************************************/

/**************************************************************************************
 * INCLUDE
 **************************************************************************************/

#include <avr/io.h>

#include <snowfox/hal/avr/ATMEGA1284P/CriticalSection.h>
#include <snowfox/hal/avr/ATMEGA1284P/InterruptController.h>

#include <snowfox/blox/hal/avr/ATMEGA1284P/UART0.h>

#include <snowfox/blox/driver/serial/SerialUart.h>

#include <snowfox/trace/Trace.h>
#include <snowfox/trace/SerialTraceOutput.h>

/**************************************************************************************
 * NAMESPACES
 **************************************************************************************/

using namespace snowfox;
using namespace snowfox::hal;
using namespace snowfox::driver;

/**************************************************************************************
 * CONSTANTS
 **************************************************************************************/

static uint16_t const UART_RX_BUFFER_SIZE =  0;
static uint16_t const UART_TX_BUFFER_SIZE = 16;

/**************************************************************************************
 * MAIN
 **************************************************************************************/

int main()
{
  /* HAL ******************************************************************************/

  ATMEGA1284P::InterruptController int_ctrl(&EIMSK, &PCICR, &PCMSK0, &PCMSK1, &PCMSK2, &PCMSK3, &WDTCSR, &TIMSK0, &TIMSK1, &TIMSK2, &UCSR0B, &UCSR1B, &SPCR, &TWCR, &EECR, &SPMCSR, &ACSR, &ADCSRA);
  ATMEGA1284P::CriticalSection     crit_sec(&SREG);

  blox::ATMEGA1284P::UART0         uart0   (&UDR0, &UCSR0A, &UCSR0B, &UCSR0C, &UBRR0, int_ctrl, F_CPU);


  /* DRIVER ***************************************************************************/

  blox::SerialUart serial(crit_sec,
                          uart0(),
                          UART_RX_BUFFER_SIZE,
                          UART_TX_BUFFER_SIZE,
                          serial::interface::SerialBaudRate::B115200,
                          serial::interface::SerialParity::None,
                          serial::interface::SerialStopBit::_1);


  /* GLOBAL INTERRUPT *****************************************************************/

  int_ctrl.enableInterrupt(ATMEGA164P_324P_644P_1284P::toIntNum(ATMEGA1284P::Interrupt::GLOBAL));


  /* APPLICATION **********************************************************************/

  trace::SerialTraceOutput serial_trace_output(serial());
  trace::Trace             trace              (serial_trace_output,trace::Level::Debug);

  for(uint32_t cnt = 0;; cnt++)
  {
    trace.print(trace::Level::Debug, "( %08X ) Hello ATMEGA1284P\r\n", cnt);
  }

  return 0;
}
