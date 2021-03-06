/**
 * Snowfox is a modular RTOS with extensive IO support.
 * Copyright (C) 2017 - 2020 Alexander Entinger / LXRobotics GmbH
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
 * This example program is tailored for usage with Arduino Uno
 * and Seedstudio CAN Bus Shield V2.0
 *
 * Electrical interface:
 *   CS   = D10 = PB2
 *   SCK  = D13 = PB5
 *   MISO = D12 = PB4
 *   MOSI = D11 = PB3
 *   INT  = D2  = PD2 = INT0
 *
 * Upload via avrdude
 *   avrdude -p atmega328p -c avrisp2 -e -U flash:w:bin/driver-mcp2515-spi-atmega328p-receiver
 **************************************************************************************/

/**************************************************************************************
 * INCLUDE
 **************************************************************************************/

#include <avr/io.h>

#include <snowfox/hal/avr/ATMEGA328P/Delay.h>
#include <snowfox/hal/avr/ATMEGA328P/DigitalInPin.h>
#include <snowfox/hal/avr/ATMEGA328P/DigitalOutPin.h>
#include <snowfox/hal/avr/ATMEGA328P/CriticalSection.h>
#include <snowfox/hal/avr/ATMEGA328P/InterruptController.h>
#include <snowfox/hal/avr/ATMEGA328P/ExternalInterruptController.h>

#include <snowfox/blox/hal/avr/ATMEGA328P/UART0.h>
#include <snowfox/blox/hal/avr/ATMEGA328P/SpiMaster.h>

#include <snowfox/blox/driver/serial/SerialUart.h>

#include <snowfox/driver/can/Can.h>
#include <snowfox/driver/can/interface/CanFrameBuffer.h>

#include <snowfox/driver/can/MCP2515/MCP2515_IoSpi.h>
#include <snowfox/driver/can/MCP2515/MCP2515_Debug.h>
#include <snowfox/driver/can/MCP2515/MCP2515_Control.h>
#include <snowfox/driver/can/MCP2515/MCP2515_CanControl.h>
#include <snowfox/driver/can/MCP2515/MCP2515_Configuration.h>
#include <snowfox/driver/can/MCP2515/MCP2515_CanConfiguration.h>

#include <snowfox/driver/can/MCP2515/events/MCP2515_EventCallback.h>
#include <snowfox/driver/can/MCP2515/events/MCP2515_onWakeup.h>
#include <snowfox/driver/can/MCP2515/events/MCP2515_onMessageError.h>
#include <snowfox/driver/can/MCP2515/events/MCP2515_onReceiveBufferFull.h>
#include <snowfox/driver/can/MCP2515/events/MCP2515_onTransmitBufferEmpty.h>

#include <snowfox/trace/Trace.h>
#include <snowfox/trace/SerialTraceOutput.h>

/**************************************************************************************
 * NAMESPACES
 **************************************************************************************/

using namespace snowfox;
using namespace snowfox::hal;
using namespace snowfox::driver;

/**************************************************************************************
 * GLOBAL CONSTANTS
 **************************************************************************************/

static uint16_t                    const UART_RX_BUFFER_SIZE      = 0;
static uint16_t                    const UART_TX_BUFFER_SIZE      = 64;

static hal::interface::SpiMode     const MCP2515_SPI_MODE         = hal::interface::SpiMode::MODE_0;
static hal::interface::SpiBitOrder const MCP2515_SPI_BIT_ORDER    = hal::interface::SpiBitOrder::MSB_FIRST;
static uint32_t                    const MCP2515_SPI_PRESCALER    = 16; /* Arduino Uno Clk = 16 MHz -> SPI Clk = 1 MHz                     */
static hal::interface::TriggerMode const MCP2515_INT_TRIGGER_MODE = hal::interface::TriggerMode::FallingEdge;

static uint8_t                     const F_MCP2515_MHz            = 16; /* Seedstudio CAN Bus Shield V2.0 is clocked with a 16 MHz crystal */
static uint16_t                    const CAN_TX_BUFFER_SIZE       = 8;
static uint16_t                    const CAN_RX_BUFFER_SIZE       = 8;

/**************************************************************************************
 * MAIN
 **************************************************************************************/

int snowfox_main()
{
  /************************************************************************************
   * HAL
   ************************************************************************************/

  ATMEGA328P::Delay                       delay;
  ATMEGA328P::InterruptController         int_ctrl    (&EIMSK, &PCICR, &PCMSK0, &PCMSK1, &PCMSK2, &WDTCSR, &TIMSK0, &TIMSK1, &TIMSK2, &UCSR0B, &SPCR, &TWCR, &EECR, &SPMCSR, &ACSR, &ADCSRA);
  ATMEGA328P::CriticalSection             crit_sec;
  ATMEGA328P::ExternalInterruptController ext_int_ctrl(&EICRA,
                                                       int_ctrl);

  ATMEGA328P::DigitalOutPin       mcp2515_cs  (&DDRB, &PORTB,        2); /* CS   = D10 = PB2 */
  ATMEGA328P::DigitalOutPin       mcp2515_sck (&DDRB, &PORTB,        5); /* SCK  = D13 = PB5 */
  ATMEGA328P::DigitalInPin        mcp2515_miso(&DDRB, &PORTB, &PINB, 4); /* MISO = D12 = PB4 */
  ATMEGA328P::DigitalOutPin       mcp2515_mosi(&DDRB, &PORTB,        3); /* MOSI = D11 = PB3 */

  mcp2515_cs.set();
  mcp2515_miso.setPullUpMode(hal::interface::PullUpMode::PULL_UP);

  blox::ATMEGA328P::UART0                       uart0       (&UDR0,
                                                             &UCSR0A,
                                                             &UCSR0B,
                                                             &UCSR0C,
                                                             &UBRR0,
                                                             int_ctrl,
                                                             F_CPU);

  blox::ATMEGA328P::SpiMaster                   spi_master  (&SPCR,
                                                             &SPSR,
                                                             &SPDR,
                                                             int_ctrl,
                                                             MCP2515_SPI_MODE,
                                                             MCP2515_SPI_BIT_ORDER,
                                                             MCP2515_SPI_PRESCALER);

  /* EXT INT #0 for notifications by MCP2515 ******************************************/
  ATMEGA328P::DigitalInPin mcp2515_int_pin              (&DDRD, &PORTD, &PIND, 2); /* D2 = PD2 = INT0 */
                           mcp2515_int_pin.setPullUpMode(hal::interface::PullUpMode::PULL_UP);

  ext_int_ctrl.setTriggerMode(ATMEGA328P::toExtIntNum(ATMEGA328P::ExternalInterrupt::EXTERNAL_INT0), MCP2515_INT_TRIGGER_MODE);
  ext_int_ctrl.enable        (ATMEGA328P::toExtIntNum(ATMEGA328P::ExternalInterrupt::EXTERNAL_INT0)                          );

  /* GLOBAL INTERRUPT *****************************************************************/
  int_ctrl.enableInterrupt(ATMEGA328P::toIntNum(ATMEGA328P::Interrupt::GLOBAL));


  /************************************************************************************
   * DRIVER
   ************************************************************************************/

  /* SERIAL ***************************************************************************/
  blox::SerialUart   serial(crit_sec,
                            uart0(),
                            UART_RX_BUFFER_SIZE,
                            UART_TX_BUFFER_SIZE,
                            serial::interface::SerialBaudRate::B115200,
                            serial::interface::SerialParity::None,
                            serial::interface::SerialStopBit::_1);

  trace::SerialTraceOutput serial_trace_output(serial());
  trace::Trace             trace              (serial_trace_output,trace::Level::Debug);

  /* MCP2515 **************************************************************************/
  can::interface::CanFrameBuffer              mcp2515_can_tx_buf                (CAN_TX_BUFFER_SIZE);
  can::interface::CanFrameBuffer              mcp2515_can_rx_buf                (CAN_RX_BUFFER_SIZE);

  can::MCP2515::MCP2515_IoSpi                 mcp2515_io_spi                    (spi_master(), mcp2515_cs);
  can::MCP2515::MCP2515_Control               mcp2515_ctrl                      (mcp2515_io_spi);
  can::MCP2515::MCP2515_Configuration         mcp2515_config                    (mcp2515_io_spi, delay, F_MCP2515_MHz);
  can::MCP2515::MCP2515_CanConfiguration      mcp2515_can_config                (mcp2515_config);
  can::MCP2515::MCP2515_CanControl            mcp2515_can_control               (mcp2515_can_tx_buf, mcp2515_can_rx_buf, mcp2515_ctrl);

  can::MCP2515::MCP2515_onMessageError        mcp2515_on_message_error;
  can::MCP2515::MCP2515_onWakeup              mcp2515_on_wakeup;
  can::MCP2515::MCP2515_onTransmitBufferEmpty mcp2515_on_transmit_buffer_2_empty(mcp2515_can_tx_buf, mcp2515_ctrl);
  can::MCP2515::MCP2515_onTransmitBufferEmpty mcp2515_on_transmit_buffer_1_empty(mcp2515_can_tx_buf, mcp2515_ctrl);
  can::MCP2515::MCP2515_onTransmitBufferEmpty mcp2515_on_transmit_buffer_0_empty(mcp2515_can_tx_buf, mcp2515_ctrl);
  can::MCP2515::MCP2515_onReceiveBufferFull   mcp2515_on_receive_buffer_1_full  (mcp2515_can_rx_buf, mcp2515_ctrl);
  can::MCP2515::MCP2515_onReceiveBufferFull   mcp2515_on_receive_buffer_0_full  (mcp2515_can_rx_buf, mcp2515_ctrl);
  can::MCP2515::MCP2515_EventCallback         mcp2515_event_callback            (mcp2515_ctrl, mcp2515_on_message_error, mcp2515_on_wakeup, mcp2515_on_transmit_buffer_2_empty, mcp2515_on_transmit_buffer_1_empty, mcp2515_on_transmit_buffer_0_empty, mcp2515_on_receive_buffer_1_full, mcp2515_on_receive_buffer_0_full);

  can::Can                                    can                               (mcp2515_can_config, mcp2515_can_control);

  ext_int_ctrl.registerInterruptCallback(ATMEGA328P::toExtIntNum(ATMEGA328P::ExternalInterrupt::EXTERNAL_INT0), &mcp2515_event_callback);


  uint8_t bitrate = static_cast<uint8_t>(can::interface::CanBitRate::BR_250kBPS);

  can.open();

  can.ioctl(can::IOCTL_SET_BITRATE, static_cast<void *>(&bitrate));


  /************************************************************************************
   * APPLICATION
   ************************************************************************************/

  for(;;)
  {
    util::type::CanFrame frame;

    bool const success = can.read(reinterpret_cast<uint8_t* >(&frame), sizeof(frame)) == sizeof(frame);
    if(success)
    {
      trace.print(trace::Level::Debug, "%04X %02i ", frame.id, frame.dlc);
      for(uint8_t b = 0; b < frame.dlc; b++)
      {
        trace.print(trace::Level::Debug, "%02X ", frame.data[b]);
      }
      trace.println(trace::Level::Debug);
    }
  }

  can.close();

  return 0;
}
