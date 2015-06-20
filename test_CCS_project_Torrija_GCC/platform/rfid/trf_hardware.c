
#include "trf_hardware.h"
#include "trf_parallel.h"
#include "dev/msp430_arch.h"

static struct process *registered_proc;

void initial_settings(void){
	unsigned char	command[2];
	
/*	command[0] = RegulatorControl;	
	trf_read_single(command, 1);
	command[0] = ISOControl;	
	command[1] = 0x07;
	trf_write_single(command, 2);
	trf_read_single(command, 1);*/
	register_port1IntHandler(5,trf_interrupt);
	
	command[0] = ModulatorControl;
	command[1] = 0x21;   //6.78 MHz
    //command[1] = 0x31;  //13.56 MHz
	trf_write_single(command, 2);
}

void counter_set(void)
{
	TA1CTL |= TACLR;
	TA1CTL &= ~TACLR;					//reset the timerA
	TA1CTL |= TASSEL0 + ID1 + ID0;		//ACLK, div 8, interrupt enable, timer stoped
	
	TA1R = 0x0000;
	TA1CCTL0 |= CCIE;					//compare interrupt enable
}//counter_set()


void init_ports(void)
{
    // Initialization of ports all unused pins as outputs with low-level
    // set all ports  to low on all pins
    PAOUT   =   0x0000;
    PASEL   =   0x0000;
    PADIR   =   0xFFFF;

    PBOUT   =   0x0000;
    PBSEL   =   0x0000;
    PBDIR   =   0xFFFF;

    PCOUT   =   0x0000;
    PCSEL   =   0x0000;
    PCDIR   =   0xFFFF;

    PDOUT   =   0x0000;
    PDSEL   =   0x0000;
    PDDIR   =   0xFFFF;

    PJDIR   =   0xFFFF;
    PJOUT   =   0x0000;
}

void delay_us(uint16_t usec) // 5 cycles for calling
{
    // The least we can wait is 3 usec:
    // ~1 usec for call, 1 for first compare and 1 for return
    while(usec > 3)       // 2 cycles for compare
    {                // 2 cycles for jump 
		_delay_cycles(9);
/***********************8 MHz**************************
  		_delay_cycles(8);
***********************16 MHz**************************
		_delay_cycles(8);
***********************24 MHz**************************
        _delay_cycles(8);
***********************32 MHz**************************/
        usec -= 2;        // 1 cycles for optimized decrement
    }
}                         // 4 cycles for returning

void delay_ms(uint16_t msec)
{
    while(msec-- > 0){ delay_us(1000); }
}

void proc_register(struct process *proc)
{
	registered_proc = proc;
}
/*
 =======================================================================================================================
 INTERRUPTS:
 =======================================================================================================================
 */

void InterruptHandlerReader(unsigned char *Register)
{
	//set the parity flag to 0 ;
    //parity is not used in 15693 and Tag-It 
	unsigned char	len;	 
	
	if(*Register == 0xA0) // Theoretically this condition was checked in the interrupt body
	{					/* TX active and only 3 bytes left in FIFO */
		i_reg = 0x00;	/* FIFO filling is done in the transmit function */
	}

	else if(*Register == BIT7)		// IRQ set due to end of TX (and only BIT7)
	{								/* TX complete */
		i_reg = 0x00;
		*Register = Reset;			/* reset the FIFO after TX */
		trf_direct_command(Register);
	}

	else if((*Register & BIT1) == BIT1) // Collision error for 14443A and 15693 sub carrier
	{								/* collision error */
		i_reg = 0x02;				/* RX complete */
		CollPoss = CollisionPosition;
		trf_read_single(&CollPoss, 1);	// Read 0x0E address. Displays the bit position of collision or error
		len = CollPoss - 0x20;		/* number of valid bytes if FIFO */

//		if(!POLLING){}
		
		if((len & 0x0f) != 0x00) len = len + 0x10;	/* add 1 byte if broken byte recieved */
		len = len >> 4;

		if(len != 0x00)
		{
			// LET'S START READING FROM FIFO		
			buf[RXTXstate] = FIFO;	
			// write the recieved bytes to the correct place of the buffer;															
			trf_read_cont(&buf[RXTXstate], len);
			RXTXstate = RXTXstate + len;
		}
		*Register = StopDecoders;	/* reset the FIFO after TX */
		trf_direct_command(Register);
		
		*Register = Reset;
		trf_direct_command(Register);

		*Register = IRQStatus;		/* IRQ status register address */
        *(Register + 1) = IRQMask;	
        trf_read_single(Register, 1);	/* function call for single address read */
		
		TRF_IRQ_CLEAR();
	}
	else if(*Register == BIT6)	// IRQ set due to RX start. SOF received and RX in process
	{	/* RX flag means that EOF has been recieved */
		if(RXErrorFlag == 0x02){
			i_reg = 0x02;
			return;
		}
		/* and the number of unread bytes is in FIFOstatus regiter */
		*Register = FIFOStatus;
		trf_read_single(Register, 1);					/* determine the number of bytes left in FIFO */	
		*Register = (0x0F &*Register) + 0x01;
		buf[RXTXstate] = FIFO;						/* write the received bytes to the correct place of the*/
												
		trf_read_cont(&buf[RXTXstate], *Register);
		RXTXstate = RXTXstate +*Register;

		*Register = TXLenghtByte2;				/* determine if there are broken bytes */					
        trf_read_cont(Register, 1);					/* determine the number of bits */

		if((*Register & BIT0) == BIT0){	//No response interrupt. Signal to MCU that next slot command can be sent
			*Register = (*Register >> 1) & 0x07;	/* mask the first 5 bits */
			*Register = 8 -*Register;
			buf[RXTXstate - 1] &= 0xFF << *Register;
		}							
		*Register = Reset;				/* reset the FIFO after last byte has been read out */
		trf_direct_command(Register);

		i_reg = 0xFF;					/* signal to the receive function that this are the last bytes */
	}
	else if(*Register == 0x60)	// IRQ set due to RX start + Signals the FIFO is 1/3>FIFO>2/3
	{									/* RX active and 9 bytes already in FIFO */
		i_reg = 0x01;
		// AQUI ESTA LA MAGIA:
		buf[RXTXstate] = FIFO;
		trf_read_cont(&buf[RXTXstate], 9);	/* read 9 bytes from FIFO */
		RXTXstate = RXTXstate + 9;

		if(TRF_IRQ_PORT & TRF_IRQ_PIN)
		{
			*Register = IRQStatus;		/* IRQ status register address */
            *(Register + 1) = IRQMask;
            trf_read_single(Register, 1);	/* function call for single address read */
			TRF_IRQ_CLEAR();

			if(*Register == 0x40)	// IRQ set due to RX start. IRQ sent when RX is finished 
			{	/* end of recieve */
				*Register = FIFOStatus;
				trf_read_single(Register, 1);					/* determine the number of bytes left in FIFO */
				
				*Register = 0x0F & (*Register + 0x01);
				buf[RXTXstate] = FIFO;						/* write the recieved bytes to the correct place of the*/

				trf_read_cont(&buf[RXTXstate], *Register);
				RXTXstate = RXTXstate +*Register;

				*Register = TXLenghtByte2;					/* determine if there are broken bytes */
				trf_read_single(Register, 1);					/* determine the number of bits */

				if((*Register & BIT0) == BIT0){	//No response interrupt. Signal to MCU that next slot command can be sent
					*Register = (*Register >> 1) & 0x07;	/* mask the first 5 bits */
					*Register = 8 -*Register;
					buf[RXTXstate - 1] &= 0xFF << *Register;
				}					

				i_reg = 0xFF;			/* signal to the recieve funnction that this are the last bytes */
				*Register = Reset;		/* reset the FIFO after last byte has been read out */
				trf_direct_command(Register);
			}
			else if(*Register == 0x50){	// End of receive and CRC error
				i_reg = 0x02;
			}
		}
		else
		{
			Register[0] = IRQStatus;
            Register[1] = IRQMask;
			trf_read_single(Register, 1); /* function call for single address read */					
			if(Register[0] == 0x00) i_reg = 0xFF;
		}
	}
	else if((*Register & BIT4) == BIT4)
	{						/* CRC error */
		if((*Register & BIT5) == BIT5)
		{
			i_reg = 0x01;	/* RX active */
			RXErrorFlag = 0x02;
		}
		else
			i_reg = 0x02;	/* end of RX */
	}
	else if((*Register & BIT2) == BIT2)
	{						/* byte framing error */
		if((*Register & BIT5) == BIT5)
		{
			i_reg = 0x01;	/* RX active */
			RXErrorFlag = 0x02;
		}
		else
			i_reg = 0x02;	/* end of RX */
	}
	else if((*Register == BIT0))
	{						/* No response interrupt */
		i_reg = 0x00;
	}
	else
	{	/* Interrupt register not properly set */
//		if(!POLLING){} //send_cstring("Interrupt error");

		i_reg = 0x02;
		*Register = StopDecoders;	/* reset the FIFO after TX */
		trf_direct_command(Register);

		*Register = Reset;
		trf_direct_command(Register);

		*Register = IRQStatus;		/* IRQ status register address */
        *(Register + 1) = IRQMask;
		trf_read_single(Register, 1);	/* function call for single address read */
		TRF_IRQ_CLEAR();
	}
}	/* InterruptHandlerReader */

/*
 =======================================================================================================================
 =======================================================================================================================
 */
 
 void trf_interrupt(void){
 	unsigned char Register[2];
	if ((P1IFG & (1 << 5)) && (P1IE & (1 << 5))){
		
		STOP_COUNTER();					// stop timer mode 	
//		do {
			TRF_IRQ_CLEAR();			// PORT2 interrupt flag clear 
			Register[0] = IRQStatus;	// IRQ status register address 
	        Register[1] = IRQMask;		//Dummy read	
	        trf_read_cont(Register, 1);	// function call for single address read 
			// IRQ status register has to be read 
			if(*Register == 0xA0) {		// TX active and only 3 bytes left in FIFO
			// IRQ set due to end of TX. TX is in progress. Flag set at start and IRQ request sent at end
			// Signals the FIFO is 1/3>FIFO>2/3. Signals FIFO high or low (less than 4 or more than 8)
				goto FINISH;
			}	
			InterruptHandlerReader(&Register[0]);
//		} while((TRF_IRQ_PORT & TRF_IRQ_PIN) == TRF_IRQ_PIN); // While P1.5 still '1'
	FINISH:
		process_post(registered_proc, PROCESS_EVENT_CONTINUE, &Register[0]);
	}
 } 

//#pragma vector=TIMER1_A0_VECTOR
//__interrupt void TimerAhandler(void)
 void __attribute__((interrupt(TIMER1_A0_VECTOR))) TimerAhandler(void)
 {
	unsigned char Register[2];
	STOP_COUNTER();	
	//TRF_IRQ_CLEAR();				//PORT2 interrupt flag clear
    /*IRQ status register has to be read*/
	Register[0] = IRQStatus;		// IRQ status register address 
	Register[1] = IRQMask;			//Dummy read	
	trf_read_single(Register, 2);			// Here IRQstatus reg is read and stored in Register[0]
	*Register = *Register & 0xF7;	//set the parity flag to 0
	
	/*If no IRQ or error (0x00) or IRQ set due to end of TX (0x80) then: */
	if(*Register == 0x00 || *Register == 0x80) //added code
        //if(RXTXstate > 1) i_reg = 0xFF;	
        //else
			i_reg = 0x00;
	else	i_reg = 0x01;
	
	__low_power_mode_off_on_exit();
}//TimerAhandler
