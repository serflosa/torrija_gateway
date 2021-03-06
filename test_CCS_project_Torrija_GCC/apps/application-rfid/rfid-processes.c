
#include <stdio.h>
#include <stdarg.h>
// Error:
//#include <intrinsics.h>
#include <string.h>
#include "device.h"    	
#include "trf_hardware.h"
#include "trf_parallel.h"
#include "trf_globals.h"
#include "types.h"
#include "rfid-processes.h"
#include "coap_app_client.h"
#include "dev/leds_torrija.h"

//PORT 4.0-4.7 - (IO0-IO7) for parallel interface with reader chip
//PORTx.x - PORTx.x - USCI_A0 ---> USB/UART control signals
//PORT1.5 - IRQ
//PORT5.6 - TRF_ENABLE
//PORT5.7 - DATA_CLK
//PORT8.0 - PORT8.2 - signaling LEDs
// =======================================================================

uint8_t start_find_tagit(void);
uint8_t end_find_tagit(void);
void enable_slot_counter(void);
void disable_slot_counter(void);

unsigned char buf[BUF_LENGTH];
signed char RXTXstate;	//used for transmit recieve byte count
unsigned char RXErrorFlag;
unsigned char i_reg;	//interrupt register
unsigned char tx_reg;
unsigned char time_reg;
unsigned char CollPoss;
static unsigned char com[4];

unsigned char aux_buf[9]="";
static unsigned char outStr_buf[50]="";

u8_t* get_rfid_data() {
	static u8_t data_buf[50];
	if (strlen(outStr_buf)) {
	  strcpy(data_buf, outStr_buf);
	  strcpy(outStr_buf, "");
	  return data_buf;
	} else {
	  return NULL;
	}
}

u8_t rfid_new_data(void) {
  return strlen(outStr_buf);
}

//strcpy(outString,"\r\USB init process started correctly!! Press enter.\r\n\r\n");

// WATCHOUT: clock.h and clock_arch.c have been changed

uint8_t start_find_tagit(void)
{
	//unsigned char	command[10];
	///* Tag-it
	com[0] = ChipStateControl;
	com[1] = 0x21;
	com[2] = ISOControl;		///* set register 0x01 for TI Tag-it operation
	com[3] = 0x13;
	trf_write_single(com, 4);
	delay_ms(5);
	com[0] = 0x00;
		
	return 0;
}		

uint8_t end_find_tagit(void)
{		
	//unsigned char	command[10];
	///* Tag-it standard
	com[0] = ChipStateControl;	///* turn off RF driver
	com[1] = 0x01;
	trf_write_single(com, 2);
	delay_ms(1);
	com[0] = IRQStatus;
    com[1] = IRQMask;	
    trf_read_single(com, 1);
    return 0;
}		/* FindTags */

// enable_slot_counter and disable_slot_counter functions appeared  previously in anticollision.c file

void enable_slot_counter(void)
{
	buf[41] = IRQMask;	/* next slot counter */
	buf[40] = IRQMask;
	trf_read_single(&buf[41], 1);
	buf[41] |= BIT0;	/* set BIT0 in register 0x01 */
	trf_write_single(&buf[40], 2);
}

void disable_slot_counter(void)
{
	buf[41] = IRQMask;	/* next slot counter */
	buf[40] = IRQMask;
	trf_read_single(&buf[41], 1);
	buf[41] &= 0xfe;	/* clear BIT0 in register 0x01 */
	trf_write_single(&buf[40], 2);
}

/*---------------------------------------------------------------------------*/
PROCESS(rfid_find_tags, "Find tags");
PROCESS(tagit_inventory_request, "Tag-it requests");
/* And set them to start at start-up time */
//AUTOSTART_PROCESSES(&rfid_find_tags);


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(rfid_find_tags, ev, data)
{ 
	
	static struct etimer find;

	PROCESS_BEGIN();
	proc_register(&rfid_find_tags);
    trf_init_par(); 		//Add logic for parallel selection.	
    initial_settings();  	//Set Port Functions for Parallel Mode  
    process_start(&tagit_inventory_request, NULL); 
    
    _enable_interrupt();       // General enable interrupts
    delay_ms(10);
    LEDpowerON;          
	
	//PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
	
	while(1){
		etimer_set(&find, CLOCK_SECOND/2);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&find));	
		led_off(LED_RED);
		etimer_set(&find, CLOCK_SECOND/2);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&find));

		start_find_tagit();
		process_post(&tagit_inventory_request, PROCESS_EVENT_CONTINUE, data);

		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
		end_find_tagit();
		      
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(tagit_inventory_request, ev, data)
{
	 static unsigned char	i = 1, found = 0, command,  length, recursive;
	 static unsigned char	*PslotNo, slotNo[17];
	 static unsigned char	NewMask[8]={0,0,0,0,0,0,0,0}, Newlength=0, masksize=0,*mask=0;
	 static int				size;	
	 static struct etimer i_r;
	 static uint8_t m;	
	PROCESS_BEGIN();
	
	proc_register(&tagit_inventory_request);
	
	while(1){
		// Wait until "rfid_find_tags" runs
		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);		
	    
	    length= 0;				// Parameteres by default in TIInventoryRequest(lenght,mask)
	    found = 0; 
	    recursive = 0;
	    
	    *mask = NewMask[0]; 	// Used by iterative TIInventoryRequest() when collition occurs
		length = Newlength;
	       
		buf[0] = RXNoResponseWaitTime;
		buf[1] = 0x14;
		buf[2] = ModulatorControl;
		buf[3] = 0x21;
		trf_write_single(buf, 4);
	
		slotNo[0] = 0x00;
		enable_slot_counter();
		PslotNo = &slotNo[0];		/* we will use the allready defined buffer for storing */
	
		masksize = (((length >> 2) + 1) >> 1);	//lenght=0 masksize=0
		size = masksize + 3;		/* mask value + mask length + command code + flags */
									// size = 3
		buf[0] = 0x8f;				// Send command 0x0F "RESET"
		buf[1] = 0x91;				// Send command 0x11 "with CRC" 
		buf[2] = 0x3d;				// Write Continous Mode from 0x1D "TX Lenght Byte1"
		buf[3] = (char) (size >> 4);	//High 2 nibbles of complete bytes to be transferred through FIFO
		buf[4] = (char) (size << 4);	//buf[3]=0 buf[4]=3
		buf[5] = 0x00;				// Write to 0x00 "Chip Status Control"?
		buf[6] = 0x50;				// SID_Pol Command Code ?
		buf[7] = (length | 0x80);	// buf[7]=0x80  Add Info_Flag=1 
		
		if(length > 0){				// Never enter here
			for(i = 0; i < masksize; i++) buf[i + 8] = *(mask + i);
		}	
		if(length & 0x04){	/* broken byte correction */
			buf[4] = (buf[4] | 0x09);						/* 4 bits */
			buf[masksize + 7] = (buf[masksize + 7] << 4);	/* move 4 bits to MSB part */
		}
	
		i_reg = 0x01;
		TRF_IRQ_CLEAR();					/* PORT2 interrupt flag clear */
		trf_raw_write(&buf[0], masksize + 8);	/* writing to FIFO */
		TRF_IRQ_ON();

		while((i_reg == 0x01)){	// wait for end of TX interrupt
				etimer_set(&i_r, CLOCK_SECOND/8);
				PROCESS_WAIT_EVENT_UNTIL((ev == PROCESS_EVENT_TIMER)||(ev == PROCESS_EVENT_CONTINUE));
			}

		for(i = 1; i < 17; i++) //16 slot
	       	{
			RXTXstate = 1;		/* prepare the global counter */
			/* the first SID will be stored from buf[1] upwards */
			i_reg = 0x01;
				
			while((i_reg == 0x01)){	// wait for RX complete 
				etimer_set(&i_r, CLOCK_SECOND/16);
				PROCESS_WAIT_EVENT_UNTIL((ev == PROCESS_EVENT_TIMER)||(ev == PROCESS_EVENT_CONTINUE));
			}
			
			if(i_reg == 0xFF){		/* recieved SID in buffer */
				found = 1;
				for(m=0;m<9;m++){
				aux_buf[m] = buf[1+m];
			}
			}
			else if(i_reg == 0x02){	/* collision occured */
				PslotNo++;
				*PslotNo = i;
			}
			else if(i_reg == 0x00){	/* slot timeout */
			//	if(!POLLING){}
			}
			else
				;
			command = Reset;
			trf_direct_command(&command);
	
			if(i < 16){		/* if less than 16 slots used send EOF(next slot) */
				command = TransmitNextSlot;
				trf_direct_command(&command);
			}
		}// for //
		
		

		disable_slot_counter();
	
		if(found)
		{
			TRF_IRQ_OFF();
			LEDtagitON;  //Here we read buffer and send to USB
			
			
		    sprintf(outStr_buf,"\r\nTag detected: %x.%x.%x.%x.%x.%x.%x.%x \r\n\r\n"
		    					,aux_buf[0],aux_buf[1],aux_buf[2],aux_buf[3]
		    					,aux_buf[4],aux_buf[5],aux_buf[6],aux_buf[7]);
			sendData_inBackground((uint8_t*)outStr_buf,strlen(outStr_buf),1,0);
			//process_post(&coap_app_client, PROCESS_EVENT_CONTINUE, data);

			TRF_IRQ_CLEAR();
			TRF_IRQ_ON();
		}
		else	{LEDtagitOFF;}
	
		Newlength = length + 4; /* the mask length is a multiple of 4 bits */
		masksize = (((Newlength >> 2) + 1) >> 1) - 1;
	
		while(*PslotNo != 0x00)
		{
			*PslotNo = *PslotNo - 1;	/* the slot counter counts from 1 to 16, */
			/* but the slot numbers go from 0 to 15 */
			for(i = 0; i < 4; i++) NewMask[i] = *(mask + i);	/* first the whole mask is copied */
			if((Newlength & BIT2) == 0x00) *PslotNo = *PslotNo << 4;
	
			NewMask[masksize] |= *PslotNo;						/* the mask is changed */
//WATCHOUT: ESTO HAY QUE HABILITARLO!!!
			//TIInventoryRequest(&NewMask[0], Newlength);		/* recursive call */
			recursive = 1;
			PslotNo--;
		}	/* while */
		
		if (recursive == 0){
			NewMask[0] = 0;
		 	Newlength = 0;
		 	*PslotNo = 0;
		}
		TRF_IRQ_OFF();		
		process_post(&rfid_find_tags, PROCESS_EVENT_CONTINUE, data);
	
		// Post event to "rfid_find_tags" when we finish 
	}		/*while  TIInventoryRequest */		    				

	PROCESS_END();
}



