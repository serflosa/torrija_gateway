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
#ifndef BUTTONS_H_
#define BUTTONS_H_

#include "contiki.h"

#include <msp430f5519.h>

/**********************
* Buttons
**********************/

#define BUTTON1_IN				P1IN
#define BUTTON2_IN				P2IN

#define BUTTON1_PIN				1 // pin 1.1
#define BUTTON2_PIN				1 // pin 1.2

#define clear_ifg1()			P1IFG &= ~(1 << BUTTON1_PIN)
#define clear_ifg2()			P1IFG &= ~(1 << BUTTON2_PIN)
/* 
 * The max number of processes allowed to be registered as buttons
 * listeners;
 */ 
#define MAX_BUTTON_PROCESSES	10

/* 
 * The message type to be sent to all registered processes.
 */
#define BUTTONS_MSG_TYPE		1

/*
 * The buttons.
 */
#define BUTTON1					1
#define BUTTON2					2

/* 
 * The message to be sent to all registered processes. button
 * indicates the button which has been pressed (BUTTON1 or BUTTON2)
 * and type will allways be BUTTONS_MSG_TYPE.
 */
typedef struct buttons_message{
	u8_t button;
	u8_t type;
}buttons_message_t;


/**
 * \brief Configure the IO port of the MSP430.
 * 
 * This function should be called first in order for the others to work. 
 * \hideinitializer
 */
void buttons_init(void);

int buttons_1pressed(void);
int buttons_2pressed(void);
/**
 * \brief This function registers a process as a button listener. The
 * process will receive a PROCESS_EVENT_MSG in the event of a button being 
 * pressed. 
 *
 * \param proc A pointer to the process being registered.
 * \return 1 if success, 0 otherwise.
 */ 
int buttons_register(struct process *proc);

/**
 * The interrupt handler routines.
 */
void button1_interrupt(void);
void button2_interrupt(void);

#endif /*BUTTONS_H_*/
