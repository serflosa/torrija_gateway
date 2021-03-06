#include <stdio.h>
#include <stdarg.h>
// Error:
//#include <intrinsics.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "usb-processes.h"
#include "device.h"
#include "usb_def.h"          // Basic Type declarations
#include "descriptors.h"
#include "usb.h"        // USB-specific functions
#include "usb_hardware.h"
#include "usb_cdc.h"
#include "coap_app_client.h"

#define _CDC_

#define MAX_STR_LENGTH 96

void init_startUp(void);
uint8_t retInString(char*);
uint8_t parse_line(char *wholeString);

volatile uint8_t bDataReceived_event = FALSE; // Indicates data has been received without an open receive operation
volatile uint8_t bDataReceiveCompleted_event = FALSE;  // data receive completed event
volatile uint8_t bDataSendCompleted_event = FALSE;     // data send completed event                 
static char wholeString[MAX_STR_LENGTH] = "";     // The entire input string from the last 'return'
struct usb_packet usbin_packet={"","","","",""};


PROCESS(usb_input_process, "USB process");   

static void pollhandler(void){
  uint8_t i;
  if(bDataReceived_event)                              // Some data is in the buffer; begin receiving a command              
  {
    char pieceOfString[MAX_STR_LENGTH] = "";           // Holds the new addition to the string
    // Add bytes in USB buffer to theCommand
    receiveDataInBuffer((uint8_t*)pieceOfString,MAX_STR_LENGTH,1);                 // Get the next piece of the string
    strcat(wholeString,pieceOfString);                                          // Add it to the whole
    sendData_inBackground((uint8_t*)pieceOfString,strlen(pieceOfString),1,0);      // Echoes back the characters received (needed for Hyperterm)
                  
    if(retInString(wholeString))                                                        // Has the user pressed return yet?
    {                	
      parse_line(wholeString);
      process_post(&coap_app_client,PROCESS_EVENT_CONTINUE,NULL);                 
      for(i=0;i<MAX_STR_LENGTH;i++)                                                       // Clear the string in preparation for the next one  
        wholeString[i] = 0x00;
    }                
    bDataReceived_event = FALSE;
  }
  process_poll(&usb_input_process);    
}
 
 /*---------------------------------------------------------------------------*/
//AUTOSTART_PROCESSES(&usb_input_process);
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(usb_input_process, ev, data)
{ 
 PROCESS_POLLHANDLER(pollhandler());
 PROCESS_BEGIN();

    while(1)
    {
        process_poll(&usb_input_process);
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_EXIT);
    }  
    PROCESS_END();
} 

void usb_init_proc(void){
 
 int8_t usb_not_ready = 1; 
 init_startUp();                 //initialize device     
 USB_init();  
    // See if we're already attached physically to USB, and if so, connect to it
    // Normally applications don't invoke the event handlers, but this is an exception.  	  
	if (USB_connectionInfo() & kUSB_vbusPresent){
		if (USB_enable() == kUSB_succeed){
        	USB_reset();
        	USB_connect();  // generate rising edge on DP -> the host enumerates our device as full speed device
    	}
    }
	while(usb_not_ready){   	
		switch(USB_connectionState())
        {
           case ST_USB_DISCONNECTED:
                break;               
           case ST_USB_CONNECTED_NO_ENUM:
                break;               
           case ST_ENUM_ACTIVE:	
           
            	P8OUT |= BIT1;               
                usb_not_ready = FALSE;         
			    break;
                
           case ST_ENUM_SUSPENDED:
                break;             
           case ST_ENUM_IN_PROGRESS:
                break;          
           case ST_NOENUM_SUSPENDED:                 
                break;              
           case ST_ERROR:
                break;             
           default:;
        }//switch 
    }// while(1)  
}

/*----------------------------------------------------------------------------+
| System Initialization Routines                                              |
+----------------------------------------------------------------------------*/

void init_startUp(void)
{
    __disable_interrupt();               // Disable global interrupts   
//  init_ports();                        // Init ports (do first ports because clocks do change ports)
    SetVCore(3);                         // USB core requires the VCore set to 1.8 volt, independ of CPU clock frequency
//	init_clk();
    __enable_interrupt();                // enable global interrupts
}


//#pragma vector = UNMI_VECTOR
//__interrupt void UNMI_ISR(void)

void __attribute__((interrupt(UNMI_VECTOR))) UNMI_ISR(void)
{
    switch (__even_in_range(SYSUNIV, SYSUNIV_BUSIFG))
    {
    case SYSUNIV_NONE:
      __no_operation();
      break;
    case SYSUNIV_NMIIFG:
      __no_operation();
      break;
    case SYSUNIV_OFIFG:
      UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG); // Clear OSC flaut Flags fault flags
      SFRIFG1 &= ~OFIFG;                                // Clear OFIFG fault flag
      break;
    case SYSUNIV_ACCVIFG:
      __no_operation();
      break;
    case SYSUNIV_BUSIFG:

      // If bus error occured - the cleaning of flag and re-initializing of USB is required.
      SYSBERRIV = 0;            // clear bus error flag
      USB_disable();            // Disable
    }
}


// This function returns true if there's an 0x0D character in the string; and if so, 
// it trims the 0x0D and anything that had followed it.  
uint8_t retInString(char* string)
{
  uint8_t retPos=0,i,len;
  char tempStr[MAX_STR_LENGTH] = "";
  
  strncpy(tempStr,string,strlen(string));     // Make a copy of the string
  len = strlen(tempStr);    
  while((tempStr[retPos] != 0x0A) && (tempStr[retPos] != 0x0D) && (retPos++ < len)); // Find 0x0D; if not found, retPos ends up at len
  
  if((retPos<len) && (tempStr[retPos] == 0x0D))   // If 0x0D (carriage return) was actually found...
  {
    for(i=0;i<MAX_STR_LENGTH;i++)             // Empty the buffer
      string[i] = 0x00;
    strncpy(string,tempStr,retPos);           // ...trim the input string to just before 0x0D
    return TRUE;                              // ...and tell the calling function that we did so
  }
  
  else if((retPos<len) && (tempStr[retPos] == 0x0A))   // Check if 0x0A (new line) found
  {
    for(i=0;i<MAX_STR_LENGTH;i++)             // Empty the buffer
      string[i] = 0x00;
    strncpy(string,tempStr,retPos);           // ...trim the input string to just before 0x0D
    return TRUE;                              // ...and tell the calling function that we did so
  }
  else if (tempStr[retPos] == 0x0D)
    {
    for(i=0;i<MAX_STR_LENGTH;i++)             // Empty the buffer
      string[i] = 0x00;
    strncpy(string,tempStr,retPos);           // ...trim the input string to just before 0x0D
    return TRUE;                              // ...and tell the calling function that we did so
  }
    
  else if (retPos<len)
     {
    for(i=0;i<MAX_STR_LENGTH;i++)             // Empty the buffer
      string[i] = 0x00;
    strncpy(string,tempStr,retPos);           // ...trim the input string to just before 0x0D
    return TRUE;                              // ...and tell the calling function that we did so
  }
 
  return FALSE;                               // Otherwise, it wasn't found
}


uint8_t parse_line(char *string)
{
  int len;
  char *method;
  char *protocol;
  char *address;
  char *nul;
  char *port;
 
  strncpy(wholeString, string, strlen(string));
  len = strlen(wholeString);

  method = strtok(wholeString, " ");
  sendData_inBackground("\r\nMETHOD :",10,1,0);
  sendData_inBackground(method, strlen(method),1,0);
  protocol = strtok(NULL, "[" );
  address = strtok(NULL, "]");
  sendData_inBackground("\r\nADDRESS:",10,1,0);
  sendData_inBackground(address, strlen(address),1,0);
  nul = strtok(NULL, ":");
  port = strtok(NULL, "/");
  sendData_inBackground("\r\nPORT:",7,1,0);
  sendData_inBackground(port, strlen(port),1,0);
  
  //strcmp(S1,S2);

  sendData_inBackground("\r\n-----\r\n",9,1,0);
  return 0;
} 





//uint8_t parse_line(char *string)
//{
//  uint8_t pos=0,len=0,slash=0;
//  uint8_t addr_init=0;
//  uint8_t addr_end=0;
//  uint8_t addr_found=0;
//  uint8_t port_init=0;
//  uint8_t resource_init=0;
//  uint8_t resource_end=0;
//  uint8_t request_init=0;
//  uint8_t request_end=0;
//  uint8_t payload_init=0;
//  uint8_t request_found=0;
//  uint8_t option_found=0;
//
///*
//  usbin_packet.addr_sensor = "";
//  usbin_packet.port[5] = "";
//  usbin_packet.request[10] = "";
//  usbin_packet.resource[20] = "";
//  usbin_packet.payload[14] = "";*/
//  //char tempStr[MAX_STR_LENGTH] = "";
//  //strncpy(tempStr,string,strlen(string));     // Make a copy of the string
//  strncpy(wholeString,string,strlen(string));
//  len = strlen(wholeString);
//  while(pos < len)
//  {
//  	if((pos<len) && (wholeString[pos] == 0x5B)) // "["
//  	{
//   	 addr_init = pos + 1;
//  	}
//  	if((pos<len) && (wholeString[pos] == 0x5D)) // "]"
//  	{
//  		addr_end = pos - 1;
//  		addr_found = 1;
//  		strncpy(usbin_packet.addr_sensor,&wholeString[addr_init],(addr_end-addr_init+1));
//  		if((wholeString[pos+1] == ':')){
//  			strncpy(usbin_packet.port,&wholeString[pos + 2],5);
//  		}else{
//  			strncpy(usbin_packet.port,"61616",5);
//  		}
//  	}
//   	if((pos<len) && (addr_found) && (wholeString[pos] == '/')){
//  		slash++;
//  		if (slash==1){
//  			resource_init = pos + 1;
//  		}
//  	}
//  	if((pos<len) && (0 < resource_init) && (wholeString[pos] == '?')){
//  		request_found = 1;
//  		resource_end = pos - 1;
//  		request_init = pos + 1;
//  	}
//  	if((resource_init>0)&&(wholeString[pos] == ' ')){
//
//  		if (!resource_end){
//  			resource_end = pos - 1;
//  		}
//  		if((0 < request_init)&&(!request_end)){
//  			request_end = pos - 1;
//  		}
//  	}
//  	if((pos<len) && (0 < resource_end) && (wholeString[pos] == '-')){
//  		option_found = 1;
//  		if (wholeString[pos+1] == 'l'){
//  			payload_init = pos+3; // " -l "
//  		}
//  	}pos++;
//  }
//  sendData_inBackground("\r\n-----Sending Coap packet to: ",31,1,0);
//  sendData_inBackground("\r\nADDRESS:",10,1,0);
//  sendData_inBackground(usbin_packet.addr_sensor,(addr_end-addr_init+1),1,0);
//  sendData_inBackground("\r\nPORT:",7,1,0);
//  sendData_inBackground(usbin_packet.port,5,1,0);
//
//  if (!resource_end){
//  	resource_end = len;
//  }
//  if (!request_end){
//  	request_end = len;
//  }
//  if (resource_init){
//  	strncpy(usbin_packet.resource,&wholeString[resource_init],(resource_end-resource_init+1));
//  	sendData_inBackground("\r\nRESOURCE:",11,1,0);
//  	sendData_inBackground(usbin_packet.resource,(resource_end-resource_init+1),1,0);
//  }
//  if (request_found){
//  	strncpy(usbin_packet.request,&wholeString[request_init],(request_end-request_init+1));
//  	sendData_inBackground("\r\nREQUEST:",10,1,0);
//  	sendData_inBackground(usbin_packet.request,(request_end-request_init+1),1,0);
//  }
//  if(option_found){
//  	strncpy(usbin_packet.payload,&wholeString[payload_init],(len-payload_init));
//  	sendData_inBackground("\r\nPAYLOAD:",10,1,0);
//  	sendData_inBackground(usbin_packet.payload,(len-payload_init),1,0);
//  }
//  sendData_inBackground("\r\n-----\r\n",9,1,0);
//  return 0;
//}
