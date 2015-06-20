/**
 * The Buttons driver defines a number of macros and functions 
 * allowing Button control.
 * There are two Button on the platform in pins 1.4 and 2.6.
 * 
 */

/**
 * \file
 * \brief Buttons driver.
 * \author Joaquin Juan Toledo 	<joaquin.j.toledo@gmail.com>
 * \author Luis Maqueda Ara		<lc.maqueda@gmail.com>
 * \date Agoust 2010
 */
#include <msp430f5519.h>

#include "dev/buttons.h"

static struct process *registered_processes[MAX_BUTTON_PROCESSES];

static buttons_message_t msg; 
static u8_t last_index = 0;
static struct timer button1_timer;
static struct timer button2_timer;
/*---------------------------------------------------------------------------*/

/**
 * Initialize the buttons in the board. This functions must be called at boot.
 */ 
/*---------------------------------------------------------------------------*/

void
buttons_init(void)
{
	_disable_interrupts();
	P1SEL &= ~(1 << BUTTON1_PIN);	/* Set pins 1.1 as I/O */	
	P1SEL &= ~(1 << BUTTON2_PIN);  	/* Set pins 1.2 as I/O */
		
	P1REN |= (1 << BUTTON1_PIN);		/* Enable internal resistor in pin 1.4 */
	P1REN |= (1 << BUTTON2_PIN);		/* Enable internal resistor in pin 2.6 */
	
	P1OUT |= (1 << BUTTON1_PIN);		/* Set internal resisistor as pull-upin P1.4 */
	P1OUT |= (1 << BUTTON2_PIN);		/* Set internal resisistor as pull-upin P2.6 */
	
	P1DIR &= ~(1 << BUTTON1_PIN);  	/* Set pin 1.1 as input */ 
	P1DIR &= ~(1 << BUTTON2_PIN);  	/* Set pin 1.2 as input */
	
	P1IE |= (1 << BUTTON1_PIN);        /* P1.1 interrupt enabled */
  	P1IES |= (1 << BUTTON1_PIN);       /* P1.1 Hi/Lo edge */
  	P1IFG &= ~(1 << BUTTON1_PIN);    /* P1.1 IFG cleared */
  	
  	P1IE |= (1 << BUTTON2_PIN);        /* P1.2 interrupt enabled */
  	P1IES |= (1 << BUTTON2_PIN);       /* P1.2 Hi/Lo edge */
  	P1IFG &= ~(1 << BUTTON2_PIN);    /* P1.2 IFG cleared */
  	 
	msg.type = BUTTONS_MSG_TYPE;/* Set the buttons message type */
	button1_timer.start = 0;
	button2_timer.start = 0;
	
	register_port1IntHandler(BUTTON1_PIN, button1_interrupt);
	register_port2IntHandler(BUTTON2_PIN, button2_interrupt);
	
	_enable_interrupts();
}

/*---------------------------------------------------------------------------*/

/**
 *
 * This function returns whether the button1 is pressed or not.
 *
 * \return 0 if the button 1 is not pressed.
 */
/*---------------------------------------------------------------------------*/


int
buttons_1pressed(void)
{
	return (!(BUTTON1_IN & (1 << BUTTON1_PIN)));
}

/*---------------------------------------------------------------------------*/

/**
 *
 * This function returns whether the button2 is pressed or not.
 *
 * \return 0 if the button 2 is not pressed.
 */
/*---------------------------------------------------------------------------*/
	
int
buttons_2pressed(void)
{
	return (!(BUTTON2_IN & (1 << BUTTON2_PIN)));
}

/**
 *
 * This function returns whether the button2 is pressed or not.
 *
 * \return 0 if the button 2 is not pressed.
 */
/*---------------------------------------------------------------------------*/

int 
buttons_register(struct process *proc)
{
	if (last_index == MAX_BUTTON_PROCESSES) {
		/* Can't allocate another listener */
		return 0;
	} else {
		registered_processes[last_index] = proc;
		last_index++;
		return 1;
	}
}

void button1_interrupt(void) {
	int i;
	
	if (button1_timer.start == 0 || timer_expired(&button1_timer)) {
		timer_set(&button1_timer, CLOCK_SECOND>>2);	
	
		msg.button = 1;
		for (i = 0; i < last_index; i++) {
			process_post(registered_processes[i], PROCESS_EVENT_MSG, &msg);
		}
	}
	clear_ifg1();		
}

void button2_interrupt(void) {
	int i;
	
	if (button2_timer.start == 0 || timer_expired(&button2_timer)) {
		timer_set(&button2_timer, CLOCK_SECOND>>2);	
	
		msg.button = 2;
		for (i = 0; i < last_index; i++) {
			process_post(registered_processes[i], PROCESS_EVENT_MSG, &msg);
		}
	}
	clear_ifg2();
}
