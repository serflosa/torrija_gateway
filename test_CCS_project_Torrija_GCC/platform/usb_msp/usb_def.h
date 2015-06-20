// (c)2009 by Texas Instruments Incorporated, All Rights Reserved.
/*----------------------------------------------------------------------------+
|                                                                             |
|                              Texas Instruments                              |
|                                                                             |
|                          MSP430 USB-Example (CDC/HID Driver)                |
|                                                                             |
+-----------------------------------------------------------------------------+
|  Source: usb_def.h, v1.18 2009/06/11                                   |
|  Author: RSTO                                                               |
|                                                                             |
|  Description:                                                               |
|  Contains USB Constants, Type Definitions & Macros                          |
|                                                                             |
|  WHO          WHEN         WHAT                                             |
|  ---          ----------   ------------------------------------------------ |
|  RSTO         2008/09/03   born                                             |
+----------------------------------------------------------------------------*/

#include "device.h"
#ifndef _defMSP430USB_H
#define _defMSP430USB_H


#ifndef _TYPES_H_
#define _TYPES_H_
#endif

#ifdef __TI_COMPILER_VERSION__
#define __no_init
#define __data16
#define __ACCESS_20BIT_REG__  __SFR_FARPTR
#endif

/*----------------------------------------------------------------------------+
| Constant Definitions                                                        |
+----------------------------------------------------------------------------*/
#define YES         1
#define NO          0

#define TRUE        1
#define FALSE       0

#define NOERR       0
#define ERR         1

#define NO_ERROR    0
#define ERROR       1

#define DISABLE     0
#define ENABLE      1


/*----------------------------------------------------------------------------+
| USB Constants, Type Definition & Macro                                      |
+----------------------------------------------------------------------------*/

// USB related Constant
#define MAX_ENDPOINT_NUMBER     0x07    // A maximum of 7 endpoints is available
#define EP0_MAX_PACKET_SIZE     0x08
#define EP0_PACKET_SIZE         0x08
#define EP_MAX_PACKET_SIZE      0x40

// Base addresses of transmit and receive buffers
#define OEP1_X_BUFFER_ADDRESS   0x1C00  // Input  Endpoint 1 X Buffer Base-address
#define OEP1_Y_BUFFER_ADDRESS   0x1C40  // Input  Endpoint 1 Y Buffer Base-address
#define IEP1_X_BUFFER_ADDRESS   0x1C80  // Output Endpoint 1 X Buffer Base-address
#define IEP1_Y_BUFFER_ADDRESS   0x1CC0  // Output Endpoint 1 Y Buffer Base-address

#define OEP2_X_BUFFER_ADDRESS   0x1D00  // Input  Endpoint 2 X Buffer Base-address
#define OEP2_Y_BUFFER_ADDRESS   0x1D40  // Input  Endpoint 2 Y Buffer Base-address
#define IEP2_X_BUFFER_ADDRESS   0x1D80  // Output Endpoint 2 X Buffer Base-address
#define IEP2_Y_BUFFER_ADDRESS   0x1DC0  // Output Endpoint 2 Y Buffer Base-address

#define OEP3_X_BUFFER_ADDRESS   0x1E00  // Input  Endpoint 2 X Buffer Base-address
#define OEP3_Y_BUFFER_ADDRESS   0x1E40  // Input  Endpoint 2 Y Buffer Base-address
#define IEP3_X_BUFFER_ADDRESS   0x1E80  // Output Endpoint 2 X Buffer Base-address
#define IEP3_Y_BUFFER_ADDRESS   0x1EC0  // Output Endpoint 2 Y Buffer Base-address

#define OEP4_X_BUFFER_ADDRESS   0x1F00  // Input  Endpoint 2 X Buffer Base-address
#define OEP4_Y_BUFFER_ADDRESS   0x1F40  // Input  Endpoint 2 Y Buffer Base-address
#define IEP4_X_BUFFER_ADDRESS   0x1F80  // Output Endpoint 2 X Buffer Base-address
#define IEP4_Y_BUFFER_ADDRESS   0x1FC0  // Output Endpoint 2 Y Buffer Base-address

#define OEP5_X_BUFFER_ADDRESS   0x2000  // Input  Endpoint 2 X Buffer Base-address
#define OEP5_Y_BUFFER_ADDRESS   0x2040  // Input  Endpoint 2 Y Buffer Base-address
#define IEP5_X_BUFFER_ADDRESS   0x2080  // Output Endpoint 2 X Buffer Base-address
#define IEP5_Y_BUFFER_ADDRESS   0x20C0  // Output Endpoint 2 Y Buffer Base-address

#define OEP6_X_BUFFER_ADDRESS   0x2100  // Input  Endpoint 2 X Buffer Base-address
#define OEP6_Y_BUFFER_ADDRESS   0x2140  // Input  Endpoint 2 Y Buffer Base-address
#define IEP6_X_BUFFER_ADDRESS   0x2180  // Output Endpoint 2 X Buffer Base-address
#define IEP6_Y_BUFFER_ADDRESS   0x21C0  // Output Endpoint 2 Y Buffer Base-address

#define OEP7_X_BUFFER_ADDRESS   0x2200  // Input  Endpoint 2 X Buffer Base-address
#define OEP7_Y_BUFFER_ADDRESS   0x2240  // Input  Endpoint 2 Y Buffer Base-address
#define IEP7_X_BUFFER_ADDRESS   0x2280  // Output Endpoint 2 X Buffer Base-address
#define IEP7_Y_BUFFER_ADDRESS   0x22C0  // Output Endpoint 2 Y Buffer Base-address

#define X_BUFFER 0
#define Y_BUFFER 1

// addresses of pipes for endpoints
#define EP1_ADDR          0x01    //address for endpoint 1
#define EP2_ADDR          0x02    //address for endpoint 2
#define EP3_ADDR          0x03    //address for endpoint 3
#define EP4_ADDR          0x04    //address for endpoint 4
#define EP5_ADDR          0x05    //address for endpoint 5
#define EP6_ADDR          0x06    //address for endpoint 6
#define EP7_ADDR          0x07    //address for endpoint 7

// EDB Data Structure
typedef struct _tEDB
{
    uint8_t    bEPCNF;             // Endpoint Configuration
    uint8_t    bEPBBAX;            // Endpoint X Buffer Base Address
    uint8_t    bEPBCTX;            // Endpoint X Buffer byte Count
    uint8_t    bSPARE0;            // no used
    uint8_t    bSPARE1;            // no used
    uint8_t    bEPBBAY;            // Endpoint Y Buffer Base Address
    uint8_t    bEPBCTY;            // Endpoint Y Buffer byte Count
    uint8_t    bEPSIZXY;           // Endpoint XY Buffer Size
} tEDB, *tpEDB;

typedef struct _tEDB0
{
    uint8_t    bIEPCNFG;           // Input Endpoint 0 Configuration Register
    uint8_t    bIEPBCNT;           // Input Endpoint 0 Buffer Byte Count
    uint8_t    bOEPCNFG;           // Output Endpoint 0 Configuration Register
    uint8_t    bOEPBCNT;           // Output Endpoint 0 Buffer Byte Count
} tEDB0, *tpEDB0;

// EndPoint Desciptor Block Bits
#define EPCNF_USBIE     0x04    // USB Interrupt on Transaction Completion. Set By MCU
                                // 0:No Interrupt, 1:Interrupt on completion
#define EPCNF_STALL     0x08    // USB Stall Condition Indication. Set by UBM
                                // 0: No Stall, 1:USB Install Condition
#define EPCNF_DBUF      0x10    // Double Buffer Enable. Set by MCU
                                // 0: Primary Buffer Only(x-buffer only), 1:Toggle Bit Selects Buffer

#define EPCNF_TOGGLE     0x20   // USB Toggle bit. This bit reflects the toggle sequence bit of DATA0 and DATA1.

#define EPCNF_UBME      0x80    // UBM Enable or Disable bit. Set or Clear by MCU.
                                // 0:UBM can't use this endpoint
                                // 1:UBM can use this endpoint
#define EPBCNT_uint8_tCNT_MASK 0x7F // MASK for Buffer Byte Count
#define EPBCNT_NAK       0x80    // NAK, 0:No Valid in buffer, 1:Valid packet in buffer

//definitions for MSP430 USB-module
#define START_OF_USB_BUFFER   0x1C00

// input and output buffers for EP0
#define USBIEP0BUF 0x2378
#define USBOEP0BUF 0x2370

// DEVICE_REQUEST Structure
typedef struct _tDEVICE_REQUEST
{
    uint8_t    bmRequestType;              // See bit definitions below
    uint8_t    bRequest;                   // See value definitions below
    uint16_t    wValue;                     // Meaning varies with request type
    uint16_t    wIndex;                     // Meaning varies with request type
    uint16_t    wLength;                    // Number of bytes of data to transfer
} tDEVICE_REQUEST, *ptDEVICE_REQUEST;

typedef struct _tDEVICE_REQUEST_COMPARE
{
    uint8_t    bmRequestType;              // See bit definitions below
    uint8_t    bRequest;                   // See value definitions below
    uint8_t    bValueL;                    // Meaning varies with request type
    uint8_t    bValueH;                    // Meaning varies with request type
    uint8_t    bIndexL;                    // Meaning varies with request type
    uint8_t    bIndexH;                    // Meaning varies with request type
    uint8_t    bLengthL;                   // Number of bytes of data to transfer (LSByte)
    uint8_t    bLengthH;                   // Number of bytes of data to transfer (MSByte)
    uint8_t    bCompareMask;               // MSB is bRequest, if set 1, bRequest should be matched
    void    (*pUsbFunction)(void);      // function pointer
} tDEVICE_REQUEST_COMPARE, *ptDEVICE_REQUEST_COMPARE;

//----------------------------------------------------------------------------
typedef enum
{
    STATUS_ACTION_NOTHING,
    STATUS_ACTION_DATA_IN,
    STATUS_ACTION_DATA_OUT
} tSTATUS_ACTION_LIST;

#endif /* _TYPES_H_ */
/*------------------------ Nothing Below This Line --------------------------*/
