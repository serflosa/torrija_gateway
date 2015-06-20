#include "device.h"  
#include "trf_parallel.h"
#include "trf_globals.h"
#include "types.h"
#include "trf_hardware.h"

void trf_init_par(void)
{
	TRF_EN_INIT();
	TRF_DISABLE();
    delay_ms(1);
    TRF_ENABLE();
    delay_ms(1);
    
    TRF_DIR_OUT();			/* P4 output */
	TRF_SEL_IO();
	TRF_WRITE_PORT = 0x00;	/* P4 set to 0 - choose parallel inteface for the TRF796x */

	TRF_DCLK_OFF();
	TRF_DCLK_INIT();			/* DATA_CLK on P3.3 */
	TRF_IRQ_INIT();
	TRF_IRQ_RISING_EDGE();		/* rising edge interrupt */

	//leds_off();
	//leds_init();
}/* PARset */



void trf_stop_condition(void){	
	TRF_WRITE_PORT |= 0x80;	/* stop condition */
	TRF_DCLK_ON();
	TRF_WRITE_PORT = 0x00;
	TRF_DCLK_OFF();
}						/* trf_stop_condition */

void trf_stop_continous_mode(void){	/* stop condition for continous mode */
	TRF_WRITE_PORT = 0x00;
	TRF_DIR_OUT();
	TRF_WRITE_PORT = 0x80;;
	__no_operation();
	TRF_WRITE_PORT = 0x00;
}	/* STOPcond */

void trf_start_condition(void){
	TRF_WRITE_PORT = 0x00;
	TRF_DCLK_ON();
	TRF_WRITE_PORT = 0xff;
	TRF_DCLK_OFF();
}	/* trf_start_condition */

/*
 =======================================================================================================================
    Function writes only one register or a multiple number ;
    of registers with specified addresses ;
 =======================================================================================================================
 */
void trf_write_single(uint8_t *pbuf, unsigned char lenght)
{
	unsigned char	i;
//Parallel Mode
	trf_start_condition();
	while(lenght > 0)
	{
		*pbuf = (0x1f &*pbuf);	/* register address */
		/* address, write, single */
		for(i = 0; i < 2; i++)
		{
			TRF_WRITE_PORT = *pbuf;	/* send command and data */
			//PortWrite(pbuf);
			TRF_DCLK_ON();
			TRF_DCLK_OFF();
			pbuf++;
			lenght--;
		}
	}	/* while */
	trf_stop_condition();
 //end of Parallel mode
}/* trf_write_single */


/*
 =======================================================================================================================
    Function writes a specified number of registers from ;
    a specified address upwards ;
 =======================================================================================================================
 */
void trf_write_cont(unsigned char *pbuf, unsigned char lenght)
{
//Parallel Mode
	trf_start_condition();
	*pbuf = (0x20 | *pbuf); /* address, write, continous */
	*pbuf = (0x3f &*pbuf);	/* register address */
	while(lenght > 0)
	{	        	
		TRF_WRITE_PORT = *pbuf;	/* send command */
		//PortWrite(pbuf);
		TRF_DCLK_ON();
		TRF_DCLK_OFF();
		pbuf++;
		lenght--;
	}						/* while */
	trf_stop_continous_mode();
} //end of Parallel Mode
/* trf_write_cont */

/*
 =======================================================================================================================
    Function reads only one register ;
 =======================================================================================================================
 */
void trf_read_single(unsigned char *pbuf, unsigned char lenght)
{
	trf_start_condition();
	while(lenght > 0)
    {
		*pbuf = (0x40 | *pbuf); /* address, read, single */
		*pbuf = (0x5f &*pbuf);	/* register address */

		TRF_WRITE_PORT = *pbuf;		/* send command */
		//PortWrite(pbuf);
		TRF_DCLK_ON();
		TRF_DCLK_OFF();

		TRF_DIR_IN();				/* read register */
		
		TRF_DCLK_ON();
		__no_operation();

		*pbuf = TRF_READ_PORT;
		TRF_DCLK_OFF();

		TRF_WRITE_PORT = 0x00;
		//PortWriteSet(0x00);
		TRF_DIR_OUT();
		
		pbuf++;
		lenght--;
	}	/* while */

	trf_stop_condition();
} //end of parallel mode
/* trf_read_single */

/*
 =======================================================================================================================
    Function reads specified number of registers from a ;
    specified address upwards. ;
 =======================================================================================================================
 */
void trf_read_cont(unsigned char *pbuf, unsigned char lenght)
{
//Parallel Mode
	trf_start_condition();
	*pbuf = (0x60 | *pbuf); /* address, read, continous */
	*pbuf = (0x7f &*pbuf);	/* register address */
	TRF_WRITE_PORT = *pbuf;		/* send command */
	//PortWrite(pbuf);
	TRF_DCLK_ON();
	TRF_DCLK_OFF();
	TRF_DIR_IN();				/* read register */
	//TRF_WRITE_PORT = 0x00;
	
	while(lenght > 0)
	{
		TRF_DCLK_ON();
		//TRF_DIR_IN();

		__no_operation();
		*pbuf = TRF_READ_PORT;
		 //TRF_DIR_OUT();
		TRF_DCLK_OFF();
		pbuf++;
		lenght--;
	}						/* while */
	trf_stop_continous_mode();
} //end of Parallel Mode
/* trf_read_cont */

/*
 =======================================================================================================================
    Function trf_direct_command transmits a command to the reader chip
 =======================================================================================================================
 */
void trf_direct_command(unsigned char *pbuf)
{
//Parallel Mode
    trf_start_condition();
	*pbuf = (0x80 | *pbuf); /* command */
	*pbuf = (0x9f &*pbuf);	/* command code */
	TRF_WRITE_PORT = *pbuf;		/* send command */
	TRF_DCLK_ON();
	TRF_DCLK_OFF();
	trf_stop_condition();
}
/* trf_direct_command */

/*
 =======================================================================================================================
    Function used for direct writing to reader chip ;
 =======================================================================================================================
 */
void trf_raw_write(unsigned char *pbuf, unsigned char lenght)
{
//Parallel Mode
	trf_start_condition();
	while(lenght > 0)
	{
		TRF_WRITE_PORT = *pbuf;	/* send command */
		TRF_DCLK_ON();
		TRF_DCLK_OFF();
		pbuf++;
		lenght--;
	}						/* while */
	trf_stop_continous_mode();
}//end Parallel Mode
/* trf_raw_write */

/*
 =======================================================================================================================
    Direct mode (no stop condition) ;
 =======================================================================================================================
 */
void trf_direct_mode(void)
{
//Parallel Mode

	trf_start_condition();
	TRF_WRITE_PORT = ChipStateControl;
	TRF_DCLK_ON();
	TRF_DCLK_OFF();
	TRF_WRITE_PORT = 0x61;	
	//write a 1 to BIT6 in register 0x00
	TRF_DCLK_ON();
	TRF_DCLK_OFF();
	TRF_DIR_IN();			/* put the PORT1 to tristate */
}//end of Parallel mode
/* trf_direct_mode */


/*
 =======================================================================================================================
    Send a specified number of bytes from buffer to host ;
 =======================================================================================================================
 */

void reinitialize_15693_settings(void)
{
	unsigned char	command2[2];

	command2[0] = RXNoResponseWaitTime;
	command2[1] = 0x14;//20
    trf_write_single(command2, 2);

    command2[0] = RXWaitTime;
	command2[1] = 0x20; //32
	trf_write_single(command2, 2);
}


