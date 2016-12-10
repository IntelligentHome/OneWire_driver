
set(CMAKE_SYSTEM_NAME Generic)
cmake_minimum_required(VERSION 2.8)

project(OneWire_driver)

set(MCU atmega8)
set(FCPU 7372800UL)

find_program(AVR_AR avr-ar)
find_program(AVR_RANLIB avr-ranlib)

set(SOURCES
    OneWireDriver.cpp
    )

include_directories(
    ../External/include/
    )

set(CMAKE_CXX_COMPILER avr-g++)
set(CMAKE_AR ${AVR_AR})
set(CMAKE_RANLIB ${AVR_RANLIB})
set(CMAKE_CXX_FLAGS "-std=c++0x -Wall -Os -mmcu=${MCU} -DF_CPU=${FCPU}")
set(FLASH_FLAGS "-R .eeprom -R .fuse -R .lock -R .signature")

add_library(${PROJECT_NAME} STATIC ${SOURCES})
