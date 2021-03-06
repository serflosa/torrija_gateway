//****************************************************************************//
// Function Library for setting and controlling PMM, TLV, DMA, ISR
//    File: .c
//
//    Texas Instruments
//
//    Version 0.3
//    04/01/11
//
//****************************************************************************////====================================================================


#include "usb_hardware.h"
#include "usb_def.h"



//****************************************************************************//
// Set VCore
//****************************************************************************//
void SetVCore (uint8_t level)
{
  unsigned int actlevel;
  level &= PMMCOREV_3;                       // Set Mask for Max. level
  actlevel = (PMMCTL0 & PMMCOREV_3);         // Get actuel VCore

  while (level != actlevel)
  {
    if (level > actlevel)
      SetVCoreUp(++actlevel);
    else
      SetVCoreDown(--actlevel);
  }
}

//****************************************************************************//
// Set VCore up
//****************************************************************************//
void SetVCoreUp (uint8_t level)
{
  // Open PMM registers for write access
  PMMCTL0_H = 0xA5;
  // Set SVS/SVM high side new level
  SVSMHCTL = SVSHE + SVSHRVL0 * level + SVMHE + SVSMHRRL0 * level;
  // Set SVM low side to new level
  SVSMLCTL = SVMLFP + SVSLE + SVMLE + SVSMLRRL0 * level;
  // Wait till SVM is settled
  while ((PMMIFG & SVSMLDLYIFG) == 0);
  // Clear already set flags
  PMMIFG &= ~(SVMLVLRIFG + SVMLIFG);
  // Set VCore to new level
  PMMCTL0_L = PMMCOREV0 * level;
  // Wait till new level reached
  if (PMMIFG & SVMLIFG)
  while ((PMMIFG & SVMLVLRIFG) == 0);
  // Set SVS/SVM low side to new level
  SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level;
  // Lock PMM registers for write access
  PMMCTL0_H = 0x00;
}


//****************************************************************************//
// Set VCore down
//****************************************************************************//
void SetVCoreDown (uint8_t level)
{
  PMMCTL0_H = 0xA5;                         // Open PMM module registers for write access
  SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level;
//  SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level | SVMLOVPE;  // Set SVM new Level low side + overvoltage detection
                                                                                 // this will wait till VCore is below the level
  // Set SVS/SVM high side new level
  SVSMHCTL = SVSHE + SVSHRVL0 * level + SVMHE + SVSMHRRL0 * level;
  while ((PMMIFG & SVSMLDLYIFG) == 0);      // Wait till SVM is settled (Delay)
  PMMCTL0 = 0xA500 | (level * PMMCOREV0);   // Set VCore to requested level
  while (PMMIFG & SVMLIFG)
    PMMIFG &= ~(SVMLIFG);                   // Wait till SVM will not be set anymore
  // Set SVS/SVM low side to new level
//  SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level;
  PMMCTL0_H = 0x00;                         // Lock PMM module registers for write access
}

// TLV Access Function Definition file


/* Function Defintion
   Passing parameters:
      tag - The specific/particular tag information (length and data address location) 
      	required by the user
      *length - Address of the variable (byte long) where the number of bytes allocated 
      	for the particular 'tag' is to be stored
      *data_address - Address of the variable (word long) which is used to access the 
      	value info of the the particular 'tag' (in the TLV structure)

  NOTE: If 'length' and 'data_address' returned = 0, it implies that there was no tag match. 
  		This condition can be checked for in the main function that calls this Get_TLV_info(..) 
  		function and appropriate actions can be taken
*/

void Get_TLV_info(uint8_t tag, uint8_t *length, unsigned int *data_address)
{
  char *TLV_address = (char *)TLV_START;               // TLV Structure Start Address


  while((*TLV_address != tag) && (*TLV_address != TLV_TAGEND) && ((unsigned int)TLV_address < TLV_END))
  {
    TLV_address += *(TLV_address+1) + 2;               // add (Current TAG address + LENGTH) + 2
  }
  if (*TLV_address == tag)                             // Check if Tag match happened..
  {
    *length = *(TLV_address+1);                        // Return length = Address + 1 
    *data_address = (unsigned int)(TLV_address+2);     // Return address of first data/value info = Address + 2 
  }
  else                                                 // If there was no tag match and the end of TLV structure was reached..
  {
    *length = 0;                                       // Return 0 for TAG not found
    data_address = 0;                                  // Return 0 for TAG not found
  }
}


//function pointers
void *(*USB_TX_memcpy)(void * dest, const void * source, size_t count);
void *(*USB_RX_memcpy)(void * dest, const void * source, size_t count);

void * memcpyDMA0(void * dest, const void * source, size_t count);
void * memcpyDMA1(void * dest, const void * source, size_t count);
void * memcpyDMA2(void * dest, const void * source, size_t count);

// NOTE: this functin works only with data in the area <64k (small memory model)
void * memcpyV(void * dest, const void * source, size_t count)
{
    uint16_t i;
    volatile uint8_t bTmp;
    for (i=0; i<count; i++)
    {
        bTmp = *((uint8_t*)source +i);
        *((uint8_t*)dest  +i) = bTmp;
    }
    return dest;
}


//this function inits the
void USB_initMemcpy(void)
{
    USB_TX_memcpy = memcpyV;
    USB_RX_memcpy = memcpyV;

    switch (USB_DMA_TX)
    {
    case 0:
        DMACTL0 &= ~DMA0TSEL_31;         // DMA0 is triggered by DMAREQ
        DMACTL0 |= DMA0TSEL_0;           // DMA0 is triggered by DMAREQ
        DMA0CTL = (DMADT_1 + DMASBDB + DMASRCINCR_3 +   // configure block transfer (byte-wise) with increasing source
                       DMADSTINCR_3 );                  // and destination address
        DMACTL4 |= ENNMI;               // enable NMI interrupt
        USB_TX_memcpy = memcpyDMA0;
        break;
    case 1:
        DMACTL0 &= ~DMA1TSEL_31;         // DMA1 is triggered by DMAREQ
        DMACTL0 |= DMA1TSEL_0;           // DMA1 is triggered by DMAREQ
        DMA1CTL = (DMADT_1 + DMASBDB + DMASRCINCR_3 +   // configure block transfer (byte-wise) with increasing source
                       DMADSTINCR_3 );                  // and destination address
        DMACTL4 |= ENNMI;               // enable NMI interrupt
        USB_TX_memcpy = memcpyDMA1;
        break;
    case 2:
       DMACTL0 &= ~DMA2TSEL_31;         // DMA2 is triggered by DMAREQ
       DMACTL0 |= DMA2TSEL_0;           // DMA2 is triggered by DMAREQ
       DMA2CTL = (DMADT_1 + DMASBDB + DMASRCINCR_3 +   // configure block transfer (byte-wise) with increasing source
                       DMADSTINCR_3 );                 // and destination address
       DMACTL4 |= ENNMI;               // enable NMI interrupt
       USB_TX_memcpy = memcpyDMA2;
       break;
    }

   switch (USB_DMA_RX)
    {
    case 0:
        DMACTL0 &= ~DMA0TSEL_31;         // DMA0 is triggered by DMAREQ
        DMACTL0 |= DMA0TSEL_0;           // DMA0 is triggered by DMAREQ
        DMA0CTL = (DMADT_1 + DMASBDB + DMASRCINCR_3 +   // configure block transfer (byte-wise) with increasing source
                       DMADSTINCR_3 );                  // and destination address
        DMACTL4 |= ENNMI;               // enable NMI interrupt
        USB_RX_memcpy = memcpyDMA0;
        break;
    case 1:
        DMACTL0 &= ~DMA1TSEL_31;         // DMA1 is triggered by DMAREQ
        DMACTL0 |= DMA1TSEL_0;           // DMA1 is triggered by DMAREQ
        DMA1CTL = (DMADT_1 + DMASBDB + DMASRCINCR_3 +   // configure block transfer (byte-wise) with increasing source
                       DMADSTINCR_3 );                  // and destination address
        DMACTL4 |= ENNMI;               // enable NMI interrupt
        USB_RX_memcpy = memcpyDMA1;
        break;
    case 2:
       DMACTL0 &= ~DMA2TSEL_31;         // DMA2 is triggered by DMAREQ
       DMACTL0 |= DMA2TSEL_0;           // DMA2 is triggered by DMAREQ
       DMA2CTL = (DMADT_1 + DMASBDB + DMASRCINCR_3 +   // configure block transfer (byte-wise) with increasing source
                       DMADSTINCR_3 );                 // and destination address
       DMACTL4 |= ENNMI;               // enable NMI interrupt
       USB_RX_memcpy = memcpyDMA2;
       break;
    }
}

// this functions starts DMA transfer to/from USB memory into/from RAM
// Using DMA0
// Support only for data in <64k memory area.
void * memcpyDMA0(void * dest, const void *  source, size_t count)
{
    if (count == 0)         // do nothing if zero bytes to transfer
    {
        return dest;
    }
    
    DMA0DA = (uint16_t)dest;   // set destination for DMAx
    DMA0SA = (uint16_t)source; // set source for DMAx
    DMA0SZ = count;         // how many bytes to transfer

    DMA0CTL |= DMAEN;       // enable DMAx
    DMA0CTL |= DMAREQ;      // trigger DMAx

    //wait for DMA transfer finished
    while (!(DMA0CTL & DMAIFG));

    DMA0CTL &= ~DMAEN;      // disable DMAx
    return dest;
}

// this functions starts DMA transfer to/from USB memory into/from RAM
// Using DMA1
// Support only for data in <64k memory area.
void * memcpyDMA1(void * dest, const void * source, size_t count)
{
    if (count == 0)         // do nothing if zero bytes to transfer
    {
        return dest;
    }
  
    DMA1DA = (uint16_t)dest;   // set destination for DMAx
    DMA1SA = (uint16_t)source; // set source for DMAx
    DMA1SZ = count;         // how many bytes to transfer

    DMA1CTL |= DMAEN;       // enable DMAx
    DMA1CTL |= DMAREQ;      // trigger DMAx

    //wait for DMA transfer finished
    while (!(DMA1CTL & DMAIFG));

    DMA1CTL &= ~DMAEN;      // disable DMAx
    return dest;
}

// this functions starts DMA transfer to/from USB memory into/from RAM
// Using DMA2
// Support only for data in <64k memory area.
void * memcpyDMA2(void * dest, const void * source, size_t count)
{
    if (count == 0)         // do nothing if zero bytes to transfer
    {
        return dest;
    }

    DMA2DA = (uint16_t)dest;   // set destination for DMAx
    DMA2SA = (uint16_t)source; // set source for DMAx
    DMA2SZ = count;         // how many bytes to transfer

    DMA2CTL |= DMAEN;       // enable DMAx
    DMA2CTL |= DMAREQ;      // trigger DMAx

    //wait for DMA transfer finished
    while (!(DMA2CTL & DMAIFG));

    DMA2CTL &= ~DMAEN;      // disable DMAx
    return dest;
}

/*----------------------------------------------------------------------------+
| General INTERRUPT Subroutines                                                         |
+----------------------------------------------------------------------------*/

//#pragma vector=USB_UBM_VECTOR
//__interrupt void iUsbInterruptHandler(void)
void __attribute__((interrupt(USB_UBM_VECTOR))) iUsbInterruptHandler(void)
{
    uint8_t bWakeUp = FALSE;

    //Check if the setup interrupt is pending.
    //We need to check it before other interrupts,
    //to work around that the Setup Int has lower priority then Input Endpoint 0
    if (USBIFG & SETUPIFG)
    {
        bWakeUp = SetupPacketInterruptHandler();
        USBIFG &= ~SETUPIFG;    // clear the interrupt bit
    }

    switch (__even_in_range(USBVECINT & 0x3f, USBVECINT_OUTPUT_ENDPOINT7))
    {
      case USBVECINT_NONE:
        break;
      case USBVECINT_PWR_DROP:
        __no_operation();
        break;
      case USBVECINT_PLL_LOCK:
        break;
      case USBVECINT_PLL_SIGNAL:
        break;
      case USBVECINT_PLL_RANGE:
      	bWakeUp = FALSE;
        /*if (wUsbEventMask & kUSB_clockFaultEvent){ bWakeUp = USB_handleClockEvent();}*/
        break;
      case USBVECINT_PWR_VBUSOn:
        PWRVBUSonHandler();
		bWakeUp = usb_reconnect();
        //if (wUsbEventMask & kUSB_VbusOnEvent){ bWakeUp = USB_handleVbusOnEvent();
        break;
      case USBVECINT_PWR_VBUSOff:
        PWRVBUSoffHandler();
        bWakeUp = TRUE;
        // if (wUsbEventMask & kUSB_VbusOffEvent){bWakeUp = USB_handleVbusOffEvent();}
        break;
      case USBVECINT_USB_TIMESTAMP:
        break;
      case USBVECINT_INPUT_ENDPOINT0:
        IEP0InterruptHandler();
        break;
      case USBVECINT_OUTPUT_ENDPOINT0:
        OEP0InterruptHandler();
        break;
      case USBVECINT_RSTR:
        USB_reset();
        bWakeUp = TRUE;
        // if (wUsbEventMask & kUSB_UsbResetEvent){bWakeUp = USB_handleResetEvent();}
        break;
      case USBVECINT_SUSR:
        USB_suspend();
        bWakeUp = TRUE;
        // if (wUsbEventMask & kUSB_UsbSuspendEvent){ bWakeUp = USB_handleSuspendEvent();}
        break;
      case USBVECINT_RESR:
        USB_resume();
        // if (wUsbEventMask & kUSB_UsbResumeEvent) {bWakeUp = USB_handleResumeEvent();}
        //-- after resume we will wake up! Independ what event handler says.
        bWakeUp = TRUE;
        break;
      case USBVECINT_SETUP_PACKET_RECEIVED:
// WATCHOUT:
        // NAK both IEP and OEP enpoints
        tEndPoint0DescriptorBlock.bIEPBCNT = EPBCNT_NAK;
        tEndPoint0DescriptorBlock.bOEPBCNT = EPBCNT_NAK;
//--------------------------------------------------------
        bWakeUp = SetupPacketInterruptHandler();
        break;

      case USBVECINT_STPOW_PACKET_RECEIVED:
        break;

      case USBVECINT_INPUT_ENDPOINT1:
        break;
      case USBVECINT_INPUT_ENDPOINT2:
        break;
      case USBVECINT_INPUT_ENDPOINT3:
            //send saved bytes from buffer...
            bWakeUp = CdcToHostFromBuffer();
        break;
      case USBVECINT_INPUT_ENDPOINT4:
        break;
      case USBVECINT_INPUT_ENDPOINT5:
        break;
      case USBVECINT_INPUT_ENDPOINT6:
        break;
      case USBVECINT_INPUT_ENDPOINT7:
        break;
      case USBVECINT_OUTPUT_ENDPOINT1:
        break;
      case USBVECINT_OUTPUT_ENDPOINT2:
        break;
      case USBVECINT_OUTPUT_ENDPOINT3:
        #ifdef _CDC_
            //call callback function if no receive operation is underway
            if (!CdcIsReceiveInProgress()){
            	bWakeUp = usb_data_received(1);
                // if (wUsbEventMask & kUSB_dataReceivedEvent) {bWakeUp = USBCDC_handleDataReceived(1);}
            }else{
                bWakeUp = CdcToBufferFromHost(); //complete receive opereation - copy data to user buffer
            }
        #endif
        break;

      case USBVECINT_OUTPUT_ENDPOINT4:
        break;
      case USBVECINT_OUTPUT_ENDPOINT5:
        break;
      case USBVECINT_OUTPUT_ENDPOINT6:
        break;
      case USBVECINT_OUTPUT_ENDPOINT7:
        break;
      default:
        break;
    }
    if (bWakeUp)
    {
        __bic_SR_register_on_exit(LPM3_bits); // Exit LPM0-3
        __no_operation();                     // Required for debugger
    }
}


uint8_t usb_reconnect(void)
{
   	if (USB_enable() == kUSB_succeed){	 //We switch on USB and connect to the BUS
   	USB_reset();
    USB_connect(); }
    return TRUE;
}



uint8_t usb_data_received(uint8_t intfNum)
{
	bDataReceived_event = TRUE;
    return TRUE;
}

uint8_t usb_data_send_completed(uint8_t intfNum)
{
    //TO DO: You can place your code here
    bDataSendCompleted_event = TRUE;
    return FALSE;   //return FALSE to go asleep after interrupt (in the case the CPU slept before interrupt)
}

uint8_t usb_data_rcv_completed(uint8_t intfNum)
{
    //TO DO: You can place your code here
    bDataReceiveCompleted_event = TRUE; // data received event
    return FALSE;   //return FALSE to go asleep after interrupt (in the case the CPU slept before interrupt)
}

/*----------------------------------------------------------------------------+
| Interrupt Sub-routines                                                      |
+----------------------------------------------------------------------------*/

uint8_t SetupPacketInterruptHandler(void)
{
    uint8_t bTemp;
    uint8_t bWakeUp = FALSE;

    USBCTL |= FRSTE;      // Function Reset Connection Enable - set enable after first setup packet was received

    usbProcessNewSetupPacket:

    // copy the MSB of bmRequestType to DIR bit of USBCTL
    if((tSetupPacket.bmRequestType & USB_REQ_TYPE_INPUT) == USB_REQ_TYPE_INPUT)
    {
        USBCTL |= DIR;
    }
    else
    {
        USBCTL &= ~DIR;
    }

    bStatusAction = STATUS_ACTION_NOTHING;

    // clear out return data buffer
    for(bTemp=0; bTemp< USB_RETURN_DATA_LENGTH; bTemp++)
    {
        abUsbRequestReturnData[bTemp] = 0x00;
    }

    // decode and process the request
    bWakeUp = usbDecodeAndProcessUsbRequest();

    // check if there is another setup packet pending
    // if it is, abandon current one by NAKing both data endpoint 0
    if((USBIFG & STPOWIFG) != 0x00)
    {
        USBIFG &= ~(STPOWIFG | SETUPIFG);
        goto usbProcessNewSetupPacket;
    }
    return bWakeUp;
}

//----------------------------------------------------------------------------
void PWRVBUSoffHandler(void)
{
    volatile unsigned int i;
    for (i =0; i < 1000; i++);
    if (!(USBPWRCTL & USBBGVBV))
    {
        USBKEYPID   =    0x9628;        // set KEY and PID to 0x9628 -> access to configuration registers enabled
        bEnumerationStatus = 0x00;      // device is not enumerated
        bFunctionSuspended = FALSE;     // device is not suspended
        USBCNF     =    0;              // disable USB module
        USBPLLCTL  &=  ~UPLLEN;         // disable PLL
        USBPWRCTL &= ~(VBOFFIE + VBOFFIFG);          // disable interrupt VBUSoff
        USBKEYPID   =    0x9600;        // access to configuration registers disabled
    }
}

//----------------------------------------------------------------------------

void PWRVBUSonHandler(void)
{
    volatile unsigned int i;
    for (i =0; i < 1000; i++);          // waiting till voltage will be stable

    USBKEYPID =  0x9628;                // set KEY and PID to 0x9628 -> access to configuration registers enabled
    USBPWRCTL |= VBOFFIE;               // enable interrupt VBUSoff
    USBPWRCTL &= ~ (VBONIFG + VBOFFIFG);             // clean int flag (bouncing)
    USBKEYPID =  0x9600;                // access to configuration registers disabled
}

//----------------------------------------------------------------------------
void IEP0InterruptHandler(void)
{
    USBCTL |= FRSTE;                              // Function Reset Connection Enable
    tEndPoint0DescriptorBlock.bOEPBCNT = 0x00;     

    if(bStatusAction == STATUS_ACTION_DATA_IN)
    {
        usbSendNextPacketOnIEP0();
    }
    else
    {
        tEndPoint0DescriptorBlock.bIEPCNFG |= EPCNF_STALL; // no more data
    }
}

//----------------------------------------------------------------------------
void OEP0InterruptHandler(void)
{
    USBCTL |= FRSTE;                              // Function Reset Connection Enable
    tEndPoint0DescriptorBlock.bIEPBCNT = 0x00;    

    if(bStatusAction == STATUS_ACTION_DATA_OUT)
    {
        usbReceiveNextPacketOnOEP0();
        if(bStatusAction == STATUS_ACTION_NOTHING)
        {
            switch(tSetupPacket.bRequest)
            {
            #ifdef _CDC_
                case USB_CDC_SET_LINE_CODING:
                    Handler_SetLineCoding();
                    break;
            #endif // _CDC_
            default:;
            }
        }
    }
    else
    {
        tEndPoint0DescriptorBlock.bOEPCNFG |= EPCNF_STALL; // no more data
    }
}

/*----------------------------------------------------------------------------+
| End of source file                                                          |
+----------------------------------------------------------------------------*/
/*------------------------ Nothing Below This Line --------------------------*/
