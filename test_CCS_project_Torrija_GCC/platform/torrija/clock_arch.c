#include "device.h"
#include "contiki-conf.h"
#include "clock_arch.h"
//#include "sys/etimer.h"
/**
 * Static variables 
 */ 
/*---------------------------------------------------------------------------*/
static volatile clock_time_t ticks;
static volatile unsigned long seconds = 0;
/* last_tar is used for calculating clock_fine */
static unsigned short last_tar = 0;
/*---------------------------------------------------------------------------*/

/**
 * TimerA0 interrupt handler 
 */ 
/*---------------------------------------------------------------------------*/
//void timer_interrupt(void);
//#pragma vector = TIMER0_A0_VECTOR
//interrupt void

void __attribute__((interrupt(TIMER0_A0_VECTOR))) timer_interrupt(void) {
	++ticks;
    if(0 == (ticks % CLOCK_CONF_SECOND)){
		++seconds;
	}
    last_tar = TA0R;
    /* If tere are Event timers pending, notify the event timer module */
    if(etimer_pending()){
        etimer_request_poll();
    }
 //   if (process_nevents() > 0) {
  //  	LPM3_EXIT;
   // }
}
/*---------------------------------------------------------------------------*/

/**
 * Initialize the clock library.
 *
 * This function initializes the clock library and should be called
 * from the main() function of the system.
 */
/*---------------------------------------------------------------------------*/
void clock_init(void) {

	/* disable interrupts */
	_disable_interrupts();
	/* Clear timer */
	TA0CTL = TACLR;

	/* And stop it */
	TA0CCR0 = 0;

	/* TA0CCR0 interrupt enabled, interrupt occurs when timer equals TACCR0. */
	TA0CCTL0 = CCIE;

	/* Interrupt after X ms. */
	TA0CCR0 = INTERVAL - 1;

	/* Select ACLK 32768Hz clock, divide by 8. TimerA in Up Mode */
	TA0CTL |= TASSEL_1 | ID_3 | MC_1;

	ticks = 0;

	/* Enable interrupts. */
	_enable_interrupts();
}
/*---------------------------------------------------------------------------*/

/**
 * Get the current clock time.
 *
 * This function returns the current system clock time.
 *
 * \return The current clock time, measured in system ticks.
 */
/*---------------------------------------------------------------------------*/
clock_time_t clock_time(void) {
    return ticks;
}
/*---------------------------------------------------------------------------*/

/** 
 * Delays the CPU 
 */
/*---------------------------------------------------------------------------*/
void clock_delay(unsigned int i) {
    while(i--){
        _nop();
    }
}
/*---------------------------------------------------------------------------*/

/**
 * Get the current clock time measured in seconds.
 *
 * This function returns the current system clock time.
 *
 * \return The current clock time, measured in seconds.
 */
/*---------------------------------------------------------------------------*/
unsigned long clock_seconds(void) {
    return seconds;
}
/*---------------------------------------------------------------------------*/

/**
 * Returns a tick measured in cpu cycles.
 */
/*---------------------------------------------------------------------------*/
int clock_fine_max(void) {
    return INTERVAL;
}
/*---------------------------------------------------------------------------*/

/**
 * Returns the time in system cycles
 */
/*---------------------------------------------------------------------------*/
unsigned short clock_fine(void) {
    unsigned short t;
    
    /* Assign last_tar to local varible that can not be changed by interrupt */
    t = last_tar;
    /* perform calc based on t, TAR will not be changed during interrupt */
    return (unsigned short) (TA0R - t);
}
/*---------------------------------------------------------------------------*/
