// (c)2009 by Texas Instruments Incorporated, All Rights Reserved.
/*----------------------------------------------------------------------------+
|                                                                             |
|                              Texas Instruments                              |
|                                                                             |
|                          MSP430 USB-Example (CDC Driver)                    |
|                                                                             |
+-----------------------------------------------------------------------------+
|  Source: UsbCdc.c, v1.18 2009/06/11                                         |
|  Author: RSTO                                                               |
|                                                                             |
|  WHO          WHEN         WHAT                                             |
|  ---          ----------   ------------------------------------------------ |
|  RSTO         2008/09/03   born                                             |
|  RSTO         2008/09/19   Changed USBCDC_sendData to send more then 64bytes|
|  RSTO         2008/12/23   enhancements of CDC API                          |
|  RSTO         2008/05/19   updated USBCDC_intfStatus()                      |
|  RSTO         2009/05/26   added USBCDC_bytesInUSBBuffer()                  |
|  RSTO         2009/05/28   changed USBCDC_sendData()                        |
|  RSTO         2009/07/17   updated USBCDC_bytesInUSBBuffer()                |
|  RSTO         2009/10/21   move __disable_interrupt() before                |
|                            checking for suspend                             |
+----------------------------------------------------------------------------*/

#include "descriptors.h"

#ifdef _CDC_
// Error:
//#include <intrinsics.h>
#include <string.h>
#include "device.h"
#include "usb_hardware.h"
#include "usb_cdc.h"
#include "usb.h"              // USB-specific Data Structures
#include "usb_def.h"          // Basic Type declarations
#include "descriptors.h"



static unsigned long lBaudrate = 0;
static uint8_t bDataBits = 8;
static uint8_t bStopBits = 0;
static uint8_t bParity = 0;

//*********************************************************************************************
// Please see the MSP430 USB CDC API Programmer's Guide Sec. 9 for a full description of these 
// functions, how they work, and how to use them.  
//*********************************************************************************************

// This call assumes no previous send operation is underway; also assumes size is non-zero.  
// Returns zero if send completed; non-zero if it failed, with 1 = timeout and 2 = bus is gone.  
uint8_t sendData_waitTilDone(uint8_t* dataBuf, uint16_t size, uint8_t intfNum, unsigned long ulTimeout)
{
  unsigned long sendCounter = 0;
  uint16_t bytesSent, bytesReceived;
  uint8_t ret;

  ret = USBCDC_sendData(dataBuf,size,intfNum);  // ret is either sendStarted or busNotAvailable
  if(ret == kUSBCDC_busNotAvailable)
    return 2;
  
  while(1)                                      // ret was sendStarted
  {
    ret = USBCDC_intfStatus(intfNum,&bytesSent,&bytesReceived);
    if(ret & kUSBCDC_busNotAvailable)           // This may happen at any time
      return 2;
    else if(ret & kUSBCDC_waitingForSend)
    {
      if(ulTimeout && (sendCounter++ >= ulTimeout))  // Incr counter & try again
        return(1);                                   // Timed out
    }
    else return 0;                              // If neither busNotAvailable nor waitingForSend, it succeeded
  }
}



// This call assumes a previous send operation might be underway; also assumes size is non-zero.  
// Returns zero if send completed; non-zero if it failed, with 1 = timeout and 2 = bus is gone.  
uint8_t sendData_inBackground(uint8_t* dataBuf, uint16_t size, uint8_t intfNum, unsigned long ulTimeout)
{
  unsigned long sendCounter = 0; 
  uint16_t bytesSent, bytesReceived;
  
  while(USBCDC_intfStatus(intfNum,&bytesSent,&bytesReceived) & kUSBCDC_waitingForSend)
  {
    if(ulTimeout && ((sendCounter++)>ulTimeout))  // A send operation is underway; incr counter & try again
      return 1;                                   // Timed out               
  }
  
  // At this point, a return from sendData() has to be either busNotAvailable or sendStarted
  if(USBCDC_sendData(dataBuf,size,intfNum) == kUSBCDC_busNotAvailable)    // This may happen at any time
    return 2;
  else return 0;                                  // Indicate success            
}                                  



// This call assumes a previous receive operation is NOT underway; also assumes size is non-zero.  
// Returns zero if receive completed; non-zero if it failed, with 1 = timeout and 2 = bus is gone
uint8_t receiveData_waitTilDone(uint8_t* dataBuf, uint16_t size, uint8_t intfNum, unsigned long ulTimeout)
{
  unsigned long rcvCounter = 0;
  uint8_t ret;
  uint16_t bytesSent, bytesReceived;
  
  ret = USBCDC_receiveData(dataBuf,size,intfNum); 
  if(ret == kUSBCDC_busNotAvailable)
    return 2;       // Indicate bus is gone
  if(ret == kUSBCDC_receiveCompleted)
    return 0;       // Indicate success
  
  while(1)
  {
    ret = USBCDC_intfStatus(intfNum,&bytesSent,&bytesReceived);
    if(ret & kUSBCDC_busNotAvailable)
      return 2;
    else if(ret & kUSBCDC_waitingForReceive)
    {
      if(ulTimeout && (rcvCounter++ >= ulTimeout))
         return 1;   // Indicate timed out 
    }
    else return 0;   // Indicate success
  }
}
                         
                         
// This call assumes a prevoius receive operation is NOT underway.  It only retrieves what data is waiting in the buffer
// It doesn't check for kUSBCDC_busNotAvailable, b/c it doesn't matter if it's not.  size is the maximum that
// is allowed to be received before exiting; i.e., it is the size allotted to dataBuf.  
// Returns the number of bytes received.  
uint16_t receiveDataInBuffer(uint8_t* dataBuf, uint16_t size, uint8_t intfNum)
{
  uint16_t bytesInBuf;
  uint8_t* currentPos=dataBuf;
 
  while(bytesInBuf = USBCDC_bytesInUSBBuffer(intfNum))
  {
    if((uint16_t)(currentPos-dataBuf+bytesInBuf) > size) 
      break;
 
    USBCDC_receiveData(currentPos,bytesInBuf,intfNum); 
    currentPos += bytesInBuf;
  } 
  return (currentPos-dataBuf);
}


static struct _CdcWrite
{
    uint16_t nCdcBytesToSend;        // holds counter of bytes to be sent
    uint16_t nCdcBytesToSendLeft;    // holds counter how many bytes is still to be sent
    const uint8_t* pBufferToSend;   // holds the buffer with data to be sent
    uint8_t bCurrentBufferXY;       // is 0 if current buffer to write data is X, or 1 if current buffer is Y
} CdcWriteCtrl;

static struct _CdcRead
{
    uint8_t *pUserBuffer;     // holds the current position of user's receiving buffer. If NULL- no receiving operation started
    uint8_t *pCurrentEpPos;   // current positon to read of received data from curent EP
    uint16_t nBytesToReceive;  // holds how many bytes was requested by receiveData() to receive
    uint16_t nBytesToReceiveLeft;        // holds how many bytes is still requested by receiveData() to receive
    uint8_t * pCT1;           // holds current EPBCTxx register
    uint8_t * pCT2;           // holds next EPBCTxx register
    uint8_t * pEP2;           // holds addr of the next EP buffer
    uint8_t nBytesInEp;       // how many received bytes still available in current EP
    uint8_t bCurrentBufferXY; // indicates which buffer is used by host to transmit data via OUT endpoint3
} CdcReadCtrl;


// extern uint16_t wUsbEventMask;

//function pointers
extern void *(*USB_TX_memcpy)(void * dest, const void * source, size_t count);
extern void *(*USB_RX_memcpy)(void * dest, const void * source, size_t count);


/*----------------------------------------------------------------------------+
| Global Variables                                                            |
+----------------------------------------------------------------------------*/

//extern __no_init tEDB __data16 tInputEndPointDescriptorBlock[];
//extern __no_init tEDB __data16 tOutputEndPointDescriptorBlock[];
extern tEDB tInputEndPointDescriptorBlock[];
extern tEDB tOutputEndPointDescriptorBlock[];

void CdcResetData()
{
    // indicates which buffer is used by host to transmit data via OUT endpoint3 - X buffer is first
    //CdcReadCtrl.bCurrentBufferXY = X_BUFFER;

    memset(&CdcWriteCtrl, 0, sizeof(CdcWriteCtrl));
    memset(&CdcReadCtrl, 0, sizeof(CdcReadCtrl));
}

/*
Sends data over interface intfNum, of size size and starting at address data.
Returns: kUSBCDC_sendStarted
         kUSBCDC_sendComplete
         kUSBCDC_intfBusyError
*/
uint8_t USBCDC_sendData(const uint8_t* data, uint16_t size, uint8_t intfNum)
{
    if (size == 0)
    {
        return kUSBCDC_generalError;
    }

    // atomic operation - disable interrupts
    __disable_interrupt();                   // Disable global interrupts
    
    // do not access USB memory if suspended (PLL off). It may produce BUS_ERROR
    if ((bFunctionSuspended) ||
        (bEnumerationStatus != ENUMERATION_COMPLETE))
    {
        // data can not be read because of USB suspended
        __enable_interrupt();                // enable global interrupts
        return kUSBCDC_busNotAvailable;
    }

    if (CdcWriteCtrl.nCdcBytesToSendLeft != 0)
    {
        // the USB still sends previous data, we have to wait
        __enable_interrupt();                // enable global interrupts
        return kUSBCDC_intfBusyError;
    }

    //This function generate the USB interrupt. The data will be sent out from interrupt

    CdcWriteCtrl.nCdcBytesToSend = size;
    CdcWriteCtrl.nCdcBytesToSendLeft = size;
    CdcWriteCtrl.pBufferToSend = data;

    //trigger Endpoint Interrupt - to start send operation
    USBIEPIFG |= 1<<(EDB(CDC_INEP_ADDR)+1);  //IEPIFGx;

    __enable_interrupt();                // enable global interrupts

    return kUSBCDC_sendStarted;
}

//workaround for CDC windows driver: it doesn't give data to Application if was sent 64 byte
#define EP_MAX_PACKET_SIZE_CDC      0x3F

//this function is used only by USB interrupt
uint16_t CdcToHostFromBuffer()
{
    uint8_t byte_count, nTmp2;
    uint8_t * pEP1;
    uint8_t * pEP2;
    uint8_t * pCT1;
    uint8_t * pCT2;
    uint8_t bWakeUp = FALSE; //TRUE for wake up after interrupt
    static uint8_t bZeroPacketSent; // = FALSE;

    if (CdcWriteCtrl.nCdcBytesToSendLeft == 0)           // do we have somtething to send?
    {
        if (!bZeroPacketSent)               // zero packet was not yet sent
        {
            bZeroPacketSent = TRUE;

            CdcWriteCtrl.nCdcBytesToSend = 0;   // nothing to send
            //call event callback function
            bWakeUp = usb_data_send_completed(1);
            // if(wUsbEventMask & kUSB_sendCompletedEvent){ bWakeUp = USBCDC_handleSendCompleted(1);}

        } // if (!bSentZeroPacket)

        return bWakeUp;
    }

    bZeroPacketSent = FALSE;    // zero packet will be not sent: we have data

    if (CdcWriteCtrl.bCurrentBufferXY == X_BUFFER)
    {
        //this is the active EP buffer
        pEP1 = (uint8_t*)IEP3_X_BUFFER_ADDRESS;
        pCT1 = &tInputEndPointDescriptorBlock[EDB(CDC_INEP_ADDR)].bEPBCTX;

        //second EP buffer
        pEP2 = (uint8_t*)IEP3_Y_BUFFER_ADDRESS;
        pCT2 = &tInputEndPointDescriptorBlock[EDB(CDC_INEP_ADDR)].bEPBCTY;
    }
    else
    {
        //this is the active EP buffer
        pEP1 = (uint8_t*)IEP3_Y_BUFFER_ADDRESS;
        pCT1 = &tInputEndPointDescriptorBlock[EDB(CDC_INEP_ADDR)].bEPBCTY;

        //second EP buffer
        pEP2 = (uint8_t*)IEP3_X_BUFFER_ADDRESS;
        pCT2 = &tInputEndPointDescriptorBlock[EDB(CDC_INEP_ADDR)].bEPBCTX;
    }

    // how many byte we can send over one endpoint buffer
    byte_count = (CdcWriteCtrl.nCdcBytesToSendLeft > EP_MAX_PACKET_SIZE_CDC) ? EP_MAX_PACKET_SIZE_CDC : CdcWriteCtrl.nCdcBytesToSendLeft;
    nTmp2 = *pCT1;

    if(nTmp2 & EPBCNT_NAK)
    {
        USB_TX_memcpy(pEP1, CdcWriteCtrl.pBufferToSend, byte_count); // copy data into IEP3 X or Y buffer
        *pCT1 = byte_count;                      // Set counter for usb In-Transaction
        CdcWriteCtrl.bCurrentBufferXY = (CdcWriteCtrl.bCurrentBufferXY+1)&0x01; //switch buffer
        CdcWriteCtrl.nCdcBytesToSendLeft -= byte_count;
        CdcWriteCtrl.pBufferToSend += byte_count;             // move buffer pointer

        //try to send data over second buffer
        nTmp2 = *pCT2;
        if ((CdcWriteCtrl.nCdcBytesToSendLeft > 0) &&                                  // do we have more data to send?
            (nTmp2 & EPBCNT_NAK)) // if the second buffer is free?
        {
            // how many byte we can send over one endpoint buffer
            byte_count = (CdcWriteCtrl.nCdcBytesToSendLeft > EP_MAX_PACKET_SIZE_CDC) ? EP_MAX_PACKET_SIZE_CDC : CdcWriteCtrl.nCdcBytesToSendLeft;

            USB_TX_memcpy(pEP2, CdcWriteCtrl.pBufferToSend, byte_count); // copy data into IEP3 X or Y buffer
            *pCT2 = byte_count;                      // Set counter for usb In-Transaction
            CdcWriteCtrl.bCurrentBufferXY = (CdcWriteCtrl.bCurrentBufferXY+1)&0x01; //switch buffer
            CdcWriteCtrl.nCdcBytesToSendLeft -= byte_count;
            CdcWriteCtrl.pBufferToSend += byte_count;            //move buffer pointer
        }
    }
    return bWakeUp;
}

/*
Aborts an active send operation on interface intfNum.
Returns the number of bytes that were sent prior to the abort, in size.
*/
uint8_t USBCDC_abortSend(uint16_t* size, uint8_t intfNum)
{
    __disable_interrupt(); //disable interrupts - atomic operation

    *size = (CdcWriteCtrl.nCdcBytesToSend - CdcWriteCtrl.nCdcBytesToSendLeft);
    CdcWriteCtrl.nCdcBytesToSend = 0;
    CdcWriteCtrl.nCdcBytesToSendLeft = 0;

    __enable_interrupt();   //enable interrupts
    return kUSB_succeed;
}

// This function copies data from OUT endpoint into user's buffer
// Arguments:
//    pEP - pointer to EP to copy from
//    pCT - pointer to pCT control reg
//
void CopyUsbToBuff(uint8_t* pEP, uint8_t* pCT)
{
    uint8_t nCount;

    // how many byte we can get from one endpoint buffer
    nCount = (CdcReadCtrl.nBytesToReceiveLeft > CdcReadCtrl.nBytesInEp) ? CdcReadCtrl.nBytesInEp : CdcReadCtrl.nBytesToReceiveLeft;

    USB_RX_memcpy(CdcReadCtrl.pUserBuffer, pEP, nCount); // copy data from OEP3 X or Y buffer
    CdcReadCtrl.nBytesToReceiveLeft -= nCount;
    CdcReadCtrl.pUserBuffer += nCount;          // move buffer pointer
                                                // to read rest of data next time from this place

    if (nCount == CdcReadCtrl.nBytesInEp)       // all bytes are copied from receive buffer?
    {
        //switch current buffer
        CdcReadCtrl.bCurrentBufferXY = (CdcReadCtrl.bCurrentBufferXY+1) &0x01;

        CdcReadCtrl.nBytesInEp = 0;

        //clear NAK, EP ready to receive data
        *pCT = 0x00;
    }
    else
    {
        // set correct rest_data available in EP buffer
        if (CdcReadCtrl.nBytesInEp > nCount)
        {
            CdcReadCtrl.nBytesInEp -= nCount;
            CdcReadCtrl.pCurrentEpPos = pEP + nCount;
        }
        else
        {
            CdcReadCtrl.nBytesInEp = 0;
        }
    }
}

/*
Receives data over interface intfNum, of size size, into memory starting at address data.
Returns:
    kUSBCDC_receiveStarted  if the receiving process started.
    kUSBCDC_receiveCompleted  all requested date are received.
    kUSBCDC_receiveInProgress  previous receive opereation is in progress. The requested receive operation can be not started.
    kUSBCDC_generalError  error occurred.
*/
uint8_t USBCDC_receiveData(uint8_t* data, uint16_t size, uint8_t intfNum)
{
    uint8_t nTmp1;

    if ((size == 0) ||                          // read size is 0
        (data == NULL))
    {
        return kUSBCDC_generalError;
    }

    // atomic operation - disable interrupts
    __disable_interrupt();               // Disable global interrupts

    // do not access USB memory if suspended (PLL off). It may produce BUS_ERROR
    if ((bFunctionSuspended) ||
        (bEnumerationStatus != ENUMERATION_COMPLETE))
    {
        // data can not be read because of USB suspended
        __enable_interrupt();                   // enable global interrupts
        return kUSBCDC_busNotAvailable;
    }

    if (CdcReadCtrl.pUserBuffer != NULL)        // receive process already started
    {
        __enable_interrupt();                   // enable global interrupts
        return kUSBCDC_intfBusyError;
    }

    CdcReadCtrl.nBytesToReceive = size;         // bytes to receive
    CdcReadCtrl.nBytesToReceiveLeft = size;     // left bytes to receive
    CdcReadCtrl.pUserBuffer = data;             // set user receive buffer

    //read rest of data from buffer, if any
    if (CdcReadCtrl.nBytesInEp > 0)
    {
        // copy data from pEP-endpoint into User's buffer
        CopyUsbToBuff(CdcReadCtrl.pCurrentEpPos, CdcReadCtrl.pCT1);

        if (CdcReadCtrl.nBytesToReceiveLeft == 0)     // the Receive opereation is completed
        {
            CdcReadCtrl.pUserBuffer = NULL;     // no more receiving pending
            //USBCDC_handleReceiveCompleted(1);   // call event handler in interrupt context
            usb_data_rcv_completed(1);
            __enable_interrupt();               // interrupts enable
            return kUSBCDC_receiveCompleted;    // receive completed
        }

        // check other EP buffer for data - exchange pCT1 with pCT2
        if (CdcReadCtrl.pCT1 == &tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTX)
        {
            CdcReadCtrl.pCT1 = &tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTY;
            CdcReadCtrl.pCurrentEpPos = (uint8_t*)OEP3_Y_BUFFER_ADDRESS;
        }
        else
        {
            CdcReadCtrl.pCT1 = &tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTX;
            CdcReadCtrl.pCurrentEpPos = (uint8_t*)OEP3_X_BUFFER_ADDRESS;
        }

        nTmp1 = *CdcReadCtrl.pCT1;
        //try read data from second buffer
        if (nTmp1 & EPBCNT_NAK)                 // if the second buffer has received data?
        {
            nTmp1 = nTmp1 &0x7f;                // clear NAK bit
            CdcReadCtrl.nBytesInEp = nTmp1;     // holds how many valid bytes in the EP buffer
            CopyUsbToBuff(CdcReadCtrl.pCurrentEpPos, CdcReadCtrl.pCT1);
        }

        if (CdcReadCtrl.nBytesToReceiveLeft == 0)     // the Receive opereation is completed
        {
            CdcReadCtrl.pUserBuffer = NULL;     // no more receiving pending
            __enable_interrupt();               // interrupts enable
            return kUSBCDC_receiveCompleted;    // receive completed
        }
    } //read rest of data from buffer, if any

    //read 'fresh' data, if available
    nTmp1 = 0;
    if (CdcReadCtrl.bCurrentBufferXY == X_BUFFER)  //this is current buffer
    {
        if (tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTX & EPBCNT_NAK) //this buffer has a valid data packet
        {
            //this is the active EP buffer
            //pEP1
            CdcReadCtrl.pCurrentEpPos = (uint8_t*)OEP3_X_BUFFER_ADDRESS;
            CdcReadCtrl.pCT1 = &tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTX;

            //second EP buffer
            CdcReadCtrl.pEP2 = (uint8_t*)OEP3_Y_BUFFER_ADDRESS;
            CdcReadCtrl.pCT2 = &tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTY;
            nTmp1 = 1;    //indicate that data is available
        }
    }
    else // Y_BUFFER
    if (tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTY & EPBCNT_NAK)
    {
        //this is the active EP buffer
        CdcReadCtrl.pCurrentEpPos = (uint8_t*)OEP3_Y_BUFFER_ADDRESS;
        CdcReadCtrl.pCT1 = &tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTY;

        //second EP buffer
        CdcReadCtrl.pEP2 = (uint8_t*)OEP3_X_BUFFER_ADDRESS;
        CdcReadCtrl.pCT2 = &tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTX;
        nTmp1 = 1;    //indicate that data is available
    }

    if (nTmp1)
    {
        // how many byte we can get from one endpoint buffer
        nTmp1 = *CdcReadCtrl.pCT1;
        while(nTmp1 == 0)
        {
            nTmp1 = *CdcReadCtrl.pCT1;
        }

        if(nTmp1 & EPBCNT_NAK)
        {
            nTmp1 = nTmp1 &0x7f;            // clear NAK bit
            CdcReadCtrl.nBytesInEp = nTmp1; // holds how many valid bytes in the EP buffer

            CopyUsbToBuff(CdcReadCtrl.pCurrentEpPos, CdcReadCtrl.pCT1);

            nTmp1 = *CdcReadCtrl.pCT2;
            //try read data from second buffer
            if ((CdcReadCtrl.nBytesToReceiveLeft > 0) &&       // do we have more data to send?
                (nTmp1 & EPBCNT_NAK))                 // if the second buffer has received data?
            {
                nTmp1 = nTmp1 &0x7f;                  // clear NAK bit
                CdcReadCtrl.nBytesInEp = nTmp1;       // holds how many valid bytes in the EP buffer
                CopyUsbToBuff(CdcReadCtrl.pEP2, CdcReadCtrl.pCT2);
                CdcReadCtrl.pCT1 = CdcReadCtrl.pCT2;
            }
        }
    }

    if (CdcReadCtrl.nBytesToReceiveLeft == 0)     // the Receive opereation is completed
    {
        CdcReadCtrl.pUserBuffer = NULL;           // no more receiving pending
        //USBCDC_handleReceiveCompleted(1);         // call event handler in interrupt context
        usb_data_rcv_completed(1);
        __enable_interrupt();                     // interrupts enable
        return kUSBCDC_receiveCompleted;
    }

    //interrupts enable
    __enable_interrupt();
    return kUSBCDC_receiveStarted;
}


//this function is used only by USB interrupt.
//It fills user receiving buffer with received data
uint16_t CdcToBufferFromHost()
{
    uint8_t * pEP1;
    uint8_t nTmp1;
    uint8_t bWakeUp = FALSE; // per default we do not wake up after interrupt

    if (CdcReadCtrl.nBytesToReceiveLeft == 0)       // do we have somtething to receive?
    {
        CdcReadCtrl.pUserBuffer = NULL;             // no more receiving pending
        return bWakeUp;
    }

    // No data to receive...
    if (!((tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTX |
           tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTY)
           & 0x80))
    {
        return bWakeUp;
    }

    if (CdcReadCtrl.bCurrentBufferXY == X_BUFFER)   //X is current buffer
    {
        //this is the active EP buffer
        pEP1 = (uint8_t*)OEP3_X_BUFFER_ADDRESS;
        CdcReadCtrl.pCT1 = &tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTX;

        //second EP buffer
        CdcReadCtrl.pEP2 = (uint8_t*)OEP3_Y_BUFFER_ADDRESS;
        CdcReadCtrl.pCT2 = &tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTY;
    }
    else
    {
        //this is the active EP buffer
        pEP1 = (uint8_t*)OEP3_Y_BUFFER_ADDRESS;
        CdcReadCtrl.pCT1 = &tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTY;

        //second EP buffer
        CdcReadCtrl.pEP2 = (uint8_t*)OEP3_X_BUFFER_ADDRESS;
        CdcReadCtrl.pCT2 = &tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTX;
    }

    // how many byte we can get from one endpoint buffer
    nTmp1 = *CdcReadCtrl.pCT1;
    while(nTmp1 == 0)
    {
        nTmp1 = *CdcReadCtrl.pCT1;
    }

    if(nTmp1 & EPBCNT_NAK)
    {
        nTmp1 = nTmp1 &0x7f;            // clear NAK bit
        CdcReadCtrl.nBytesInEp = nTmp1; // holds how many valid bytes in the EP buffer

        CopyUsbToBuff(pEP1, CdcReadCtrl.pCT1);

        nTmp1 = *CdcReadCtrl.pCT2;
        //try read data from second buffer
        if ((CdcReadCtrl.nBytesToReceiveLeft > 0) &&       // do we have more data to send?
            (nTmp1 & EPBCNT_NAK))                 // if the second buffer has received data?
        {
            nTmp1 = nTmp1 &0x7f;                  // clear NAK bit
            CdcReadCtrl.nBytesInEp = nTmp1;       // holds how many valid bytes in the EP buffer
            CopyUsbToBuff(CdcReadCtrl.pEP2, CdcReadCtrl.pCT2);
            CdcReadCtrl.pCT1 = CdcReadCtrl.pCT2;
        }
    }

    if (CdcReadCtrl.nBytesToReceiveLeft == 0)     // the Receive opereation is completed
    {
        CdcReadCtrl.pUserBuffer = NULL;   // no more receiving pending
        bWakeUp = usb_data_rcv_completed(1);        
        // if (wUsbEventMask & kUSB_receiveCompletedEvent) {bWakeUp = USBCDC_handleReceiveCompleted(1);}
        if (CdcReadCtrl.nBytesInEp){       // Is not read data still available in the EP?
        	bWakeUp = usb_data_received(1);
        }
        // if (wUsbEventMask & kUSB_dataReceivedEvent){bWakeUp = USBCDC_handleDataReceived(1);}       
    }
    return bWakeUp;
}

// helper for USB interrupt handler
uint16_t CdcIsReceiveInProgress()
{
    return (CdcReadCtrl.pUserBuffer != NULL);
}


/*
Aborts an active receive operation on interface intfNum.
  Returns the number of bytes that were received and transferred
  to the data location established for this receive operation.
*/
uint8_t USBCDC_abortReceive(uint16_t* size, uint8_t intfNum)
{
    //interrupts disable
    __disable_interrupt();

    *size = 0; //set received bytes count to 0

    //is receive operation underway?
    if (CdcReadCtrl.pUserBuffer)
    {
        //how many bytes are already received?
        *size = CdcReadCtrl.nBytesToReceive - CdcReadCtrl.nBytesToReceiveLeft;

        CdcReadCtrl.nBytesInEp = 0;
        CdcReadCtrl.pUserBuffer = NULL;
        CdcReadCtrl.nBytesToReceiveLeft = 0;
    }

     //interrupts enable
    __enable_interrupt();
    return kUSB_succeed;
}

/*
This function rejects payload data that has been received from the host.
*/
uint8_t USBCDC_rejectData(uint8_t intfNum)
{
    // atomic operation - disable interrupts
    __disable_interrupt();               // Disable global interrupts

    // do not access USB memory if suspended (PLL off). It may produce BUS_ERROR
    if (bFunctionSuspended)
    {
        __enable_interrupt();            // enable global interrupts
        return kUSBCDC_busNotAvailable;
    }

    //Is receive operation underway?
    // - do not flush buffers if any operation still active.
    if (!CdcReadCtrl.pUserBuffer)
    {
        uint8_t tmp1 = tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTX & EPBCNT_NAK;
        uint8_t tmp2 = tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTY & EPBCNT_NAK;

        if (tmp1 ^ tmp2) // switch current buffer if any and only ONE of buffers is full
        {
            //switch current buffer
            CdcReadCtrl.bCurrentBufferXY = (CdcReadCtrl.bCurrentBufferXY+1) &0x01;
        }

        tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTX = 0;  //flush buffer X
        tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTY = 0;  //flush buffer Y
        CdcReadCtrl.nBytesInEp = 0;                     // indicates that no more data available in the EP
    }

    __enable_interrupt();       //interrupts enable
    return kUSB_succeed;
}

/*
This function indicates the status of the itnerface intfNum.
  If a send operation is active for this interface,
  the function also returns the number of bytes that have been transmitted to the host.
  If a receiver operation is active for this interface, the function also returns
  the number of bytes that have been received from the host and are waiting at the assigned address.

returns kUSBCDC_waitingForSend (indicates that a call to USBCDC_SendData()
  has been made, for which data transfer has not been completed)

returns kUSBCDC_waitingForReceive (indicates that a receive operation
  has been initiated, but not all data has yet been received)

returns kUSBCDC_dataWaiting (indicates that data has been received
  from the host, waiting in the USB receive buffers)
*/
uint8_t USBCDC_intfStatus(uint8_t intfNum, uint16_t* bytesSent, uint16_t* bytesReceived)
{
    uint8_t ret = 0;
    *bytesSent = 0;
    *bytesReceived = 0;

    //interrupts disable
    __disable_interrupt();

    // Is send operation underway?
    if (CdcWriteCtrl.nCdcBytesToSendLeft != 0)
    {
        ret |= kUSBCDC_waitingForSend;
        *bytesSent = CdcWriteCtrl.nCdcBytesToSend - CdcWriteCtrl.nCdcBytesToSendLeft;
    }

    //Is receive operation underway?
    if (CdcReadCtrl.pUserBuffer != NULL)
    {
        ret |= kUSBCDC_waitingForReceive;
        *bytesReceived = CdcReadCtrl.nBytesToReceive - CdcReadCtrl.nBytesToReceiveLeft;
    }
    else // receive operation not started
    {
        // do not access USB memory if suspended (PLL off). It may produce BUS_ERROR
        if (!bFunctionSuspended)
        {
            if((tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTX & EPBCNT_NAK)  | //any of buffers has a valid data packet
               (tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTY & EPBCNT_NAK))
            {
                ret |= kUSBCDC_dataWaiting;
            }
        }
    }

    if ((bFunctionSuspended) ||
        (bEnumerationStatus != ENUMERATION_COMPLETE))
    {
        // if suspended or not enumerated - report no other tasks pending
        ret = kUSBCDC_busNotAvailable;
    }

    //interrupts enable
    __enable_interrupt();

    __no_operation();
    return ret;
}

/*
Returns how many bytes are in the buffer are received and ready to be read.
*/
uint8_t USBCDC_bytesInUSBBuffer(uint8_t intfNum)
{
    uint8_t bTmp1 = 0;

    // atomic operation - disable interrupts
    __disable_interrupt();               // Disable global interrupts

    if ((bFunctionSuspended) ||
        (bEnumerationStatus != ENUMERATION_COMPLETE))
    {
        __enable_interrupt();               // enable global interrupts
        // if suspended or not enumerated - report 0 bytes available
        return 0;
    }

    if (CdcReadCtrl.nBytesInEp > 0)         // If a RX operation is underway, part of data may was read of the OEP buffer
    {
        bTmp1 = CdcReadCtrl.nBytesInEp;
        if (*CdcReadCtrl.pCT2 & EPBCNT_NAK) // the next buffer has a valid data packet
        {
            bTmp1 += *CdcReadCtrl.pCT2 & 0x7F;
        }
    }
    else
    {
        if (tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTX & EPBCNT_NAK) //this buffer has a valid data packet
        {
            bTmp1 = tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTX & 0x7F;
        }
        if (tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTY & EPBCNT_NAK) //this buffer has a valid data packet
        {
            bTmp1 += tOutputEndPointDescriptorBlock[EDB(CDC_OUTEP_ADDR)].bEPBCTY & 0x7F;
        }
    }

    //interrupts enable
    __enable_interrupt();
    return bTmp1;
}


//----------------------------------------------------------------------------
//  Line Coding Structure
//  dwDTERate     | 4 | Data terminal rate, in bits per second
//  bCharFormat   | 1 | Stop bits, 0 = 1 Stop bit, 1 = 1,5 Stop bits, 2 = 2 Stop bits
//  bParityType   | 1 | Parity, 0 = None, 1 = Odd, 2 = Even, 3= Mark, 4 = Space
//  bDataBits     | 1 | Data bits (5,6,7,8,16)
//----------------------------------------------------------------------------
void usbGetLineCoding(void)
{
    abUsbRequestReturnData[6] = bDataBits;               // Data bits = 8
    abUsbRequestReturnData[5] = bParity;                 // No Parity
    abUsbRequestReturnData[4] = bStopBits;               // Stop bits = 1

    abUsbRequestReturnData[3] = lBaudrate >> 24;
    abUsbRequestReturnData[2] = lBaudrate >> 16;
    abUsbRequestReturnData[1] = lBaudrate >> 8;
    abUsbRequestReturnData[0] = lBaudrate;

    wBytesRemainingOnIEP0 = 0x07;                   // amount of data to be send over EP0 to host
    usbSendDataPacketOnEP0((uint8_t*)&abUsbRequestReturnData[0]);  // send data to host
}

//----------------------------------------------------------------------------

void usbSetLineCoding(void)
{
    usbReceiveDataPacketOnEP0((uint8_t*) &abUsbRequestIncomingData);     // receive data over EP0 from Host
}

//----------------------------------------------------------------------------

void usbSetControlLineState(void)
{
    usbSendZeroLengthPacketOnIEP0();    // Send ZLP for status stage
}

//----------------------------------------------------------------------------

void Handler_SetLineCoding(void)
{
    // Baudrate Settings

    lBaudrate = (unsigned long)abUsbRequestIncomingData[3] << 24 | (unsigned long)abUsbRequestIncomingData[2]<<16 |
      (unsigned long)abUsbRequestIncomingData[1]<<8 | abUsbRequestIncomingData[0];
}

#endif //ifdef _CDC_

/*----------------------------------------------------------------------------+
| End of source file                                                          |
+----------------------------------------------------------------------------*/
/*------------------------ Nothing Below This Line --------------------------*/
