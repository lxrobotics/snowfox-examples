##########################################################################

set(SNOWFOX_APPLICATON_TARGET "trace-serial-atmega328p-uart0")
set(SNOWFOX_APPLICATON_SRCS
  examples/trace/trace-serial-atmega328p-uart0/trace-serial-atmega328p-uart0.cpp
)

##########################################################################

set(MCU_ARCH avr)

##########################################################################
# AVR ####################################################################
########################################################################## 

set(MCU_TYPE atmega328p)
set(MCU_SPEED 16000000UL)

##########################################################################
# DRIVER #################################################################
##########################################################################

set(DRIVER_CAN_MCP2515 no)

set(DRIVER_GLCD_RA6963 no)

set(DRIVER_HAPTIC_DRV2605 no)

set(DRIVER_IOEXPANDER_MAX6921 no)
set(DRIVER_IOEXPANDER_MCP23017 no)
set(DRIVER_IOEXPANDER_PCA9547 no)

set(DRIVER_LORA_RFM9x no)

set(DRIVER_MEMORY_AT45DBX no)
set(DRIVER_MEMORY_N25Q256A no)
set(DRIVER_MEMORY_PCF8570 no)

set(DRIVER_SENSOR_AD7151 no)
set(DRIVER_SENSOR_AS5600 no)
set(DRIVER_SENSOR_BMG160 no)
set(DRIVER_SENSOR_BMP388 no)
set(DRIVER_SENSOR_INA220 no)
set(DRIVER_SENSOR_L3GD20 no)
set(DRIVER_SENSOR_LIS2DSH no)
set(DRIVER_SENSOR_LIS3DSH no)
set(DRIVER_SENSOR_LIS3MDL no)
set(DRIVER_SENSOR_LSM6DSM no)

set(DRIVER_SERIAL yes)

set(DRIVER_STEPPER_TMC26x no)

set(DRIVER_TLCD_HD44780 no)

##########################################################################
# COMSTACK ###############################################################
##########################################################################

set(COMSTACK_CANOPEN no)

##########################################################################
