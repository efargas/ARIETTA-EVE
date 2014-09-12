#ifndef _FT_PLATFORM_H_
#define _FT_PLATFORM_H_

#define ARDUINO_PLATFORM
//#define MSVC_PLATFORM
#define  FT_ATMEGA_328P

//#define ARDUINO_PRO_328

#define SAMAPP_DISPLAY_WQVGA
//#define SAMAPP_DISPLAY_QVGA

#ifdef  ARDUINO_PLATFORM

#define Absolute Dial
//#define Relative Dial

#ifdef ARDUINO_PRO_328
#define FT800_CS (10)
#define FT_SDCARD_CS (5)       // Which pin is sdcard enable connected to?
#define FT800_INT (3)
#define FT800_PD_N (4)
#define FT_ARDUINO_PRO_SPI_CS FT800_CS
#define ARDUINO_PLATFORM_SPI
#define ARDUINO_PLATFORM_COCMD_BURST
#define FT_ARDUINO_ATMEGA328P_I2C
#endif

#ifdef  FT_ATMEGA_328P
#define FT800_CS (9)
#define FT_SDCARD_CS (8)       // Which pin is sdcard enable connected to?
#define FT800_INT (3)
#define FT800_PD_N (4)
#define FT_ARDUINO_PRO_SPI_CS FT800_CS
#define ARDUINO_PLATFORM_SPI
#define ARDUINO_PLATFORM_COCMD_BURST
#define FT_ARDUINO_ATMEGA328P_I2C
#endif
#endif

#ifdef ARDUINO_PLATFORM
#include <stdio.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>
#include <avr/pgmspace.h>
#include <Wire.h>
#include "..\Wire\Wire.h"
#endif

#ifdef MSVC_PLATFORM
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <Windows.h>

#include "ftd2xx.h"
#include "LibMPSSE_spi.h"
#endif


#include "FT_DataTypes.h"
#include "FT_Gpu_Hal.h"
#include "FT_Gpu.h"
#include "FT_CoPro_Cmds.h"



#ifdef MSVC_PLATFORM
#define BUFFER_OPTIMIZATION
#define MSVC_PLATFORM_SPI
/* Compile time switch for enabling sample app api sets - please cross check the same in SampleApp_RawData.cpp file as well */

#define SAMAPP_ENABLE_APIS_SET0
#define SAMAPP_ENABLE_APIS_SET1
#define SAMAPP_ENABLE_APIS_SET2
#define SAMAPP_ENABLE_APIS_SET3
#define SAMAPP_ENABLE_APIS_SET4
#endif

#ifdef ARDUINO_PLATFORM
/* Compile time switch for enabling sample app api sets - please cross check the same in SampleApp_RawData.cpp file as well */
/*
#define SAMAPP_ENABLE_APIS_SET0
#define SAMAPP_ENABLE_APIS_SET1
#define SAMAPP_ENABLE_APIS_SET2
#define SAMAPP_ENABLE_APIS_SET3
*/
#define SAMAPP_ENABLE_APIS_SET4

#endif
#endif /*_FT_PLATFORM_H_*/
/* Nothing beyond this*/









