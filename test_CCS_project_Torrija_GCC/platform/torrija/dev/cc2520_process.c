/*
 * This is an example of how to write a network device driver ("packet
 * driver") for Contiki. A packet driver is a regular Contiki process
 * that does two things:
 * # Checks for incoming packets and delivers those to the TCP/IP stack
 * # Provides an output function that transmits packets
 *
 * The output function is registered with the Contiki TCP/IP stack,
 * whereas incoming packets must be checked inside a Contiki process.
 * We use the same process for checking for incoming packets and for
 * registering the output function.
 */

/*
 * We include the "contiki-net.h" file to get all the network functions.
 */
#include "contiki-net.h"

/*
 * And the "enc28j60.h" file to get all the lower level driver functions.
 */
#include "cc2520ll.h"
#include "dev/leds_torrija.h"

/*---------------------------------------------------------------------------*/
/*
 * We declare the process that we use to register with the TCP/IP stack,
 * and to check for incoming packets.
 */
PROCESS(cc2520_process, "cc2520 process");
/*---------------------------------------------------------------------------*/
/* We declare the funcions used in the radio_driver struct.
 * All of them with exception of cc2520_setReceiverFunc and cc2520_input are 
 * defined in cc2520.c.
 * cc2520_setReceiverFunc and cc2520_input are not defined in cc2520.c but in 
 * this file. They have these names just to keep homogeneity the rest of the 
 * driver functions.
 */
//void cc2520_setReceiverFunc(void (* recv)(const struct radio_driver *d));
int cc2520_init(void);
int cc2520_prepare(const void *payload, unsigned short payload_len);
int cc2520_transmit(unsigned short transmit_len);
int cc2520_send(const void *payload, unsigned short payload_len);
int cc2520_read(void *buf, unsigned short buf_len);
int cc2520_channel_clear(void);
int cc2520_receiving_packet(void);
int cc2520_pending_packet(void);
int cc2520_on(void);
int cc2520_off(void);

/* The radio driver struct. 
 * This variable is going to be passed from here to the 
 * upper layers (sicslowmac and sicslowpan) and therefore there is no need to register
 * an output function in tcpip with tcpip_set_output_func; this will be performed
 * by sicslowpan.
 */
const struct radio_driver cc2520_driver =
  {
  	cc2520_init,
  	cc2520_prepare,
  	cc2520_transmit,
    cc2520_send,
	cc2520_read,
	cc2520_channel_clear,
	cc2520_receiving_packet,
	cc2520_pending_packet,
	cc2520_on,
	cc2520_off
  };
  

/* And we define as well the callback funcion to be called if a packet arrives*/
// NOT NEEDED?
//static void (* receiver_callback)(const struct radio_driver *);

/*---------------------------------------------------------------------------*/
/*
 * This function registers the calback function to called in the event of 
 * packet arrival.
 */
 // NOT NEEDED?
//void
//cc2520_setReceiverFunc(void (* recv)(const struct radio_driver *))
//{
//  receiver_callback = recv;
//}
/*---------------------------------------------------------------------------*/
int cc2520_init(){
	if (cc2520ll_init()== FAILED) {
		return 0;
  	} else {
		process_start(&cc2520_process, NULL);
		return 1;
	}
}

/*---------------------------------------------------------------------------*/
int
cc2520_prepare(const void *payload, unsigned short payload_len)
{
	return cc2520ll_prepare(payload, payload_len);
}
/*---------------------------------------------------------------------------*/
int
cc2520_transmit(unsigned short transmit_len)
{
	return cc2520ll_transmit();
}
/*---------------------------------------------------------------------------*/
int cc2520_send(const void *payload, unsigned short payload_len)
{
	//cc2520ll_packetSend(payload, payload_len);
	if (cc2520_prepare(payload, payload_len)) {
		return cc2520_transmit(payload_len);
	} else {
		return FAILED;
	}
}
/*---------------------------------------------------------------------------*/
/*
 * This function receives a packet from the radio and substract the 2-byte 
 * CRC length (as the rime functions require it). 
 */
int cc2520_read(void *buf, unsigned short buf_len) {
	return cc2520ll_packetReceive(buf, buf_len) - 2;
}
/*---------------------------------------------------------------------------*/
int
cc2520_channel_clear()
{
  return cc2520ll_channel_clear();
}
/*---------------------------------------------------------------------------*/
int
cc2520_receiving_packet()
{
	return cc2520ll_rxtx_packet();
}
/*---------------------------------------------------------------------------*/
int
cc2520_pending_packet()
{
	return cc2520ll_pending_packet();
}
/*---------------------------------------------------------------------------*/
int cc2520_on(){
	cc2520ll_receiveOn();
	return 1;
}
/*---------------------------------------------------------------------------*/
int cc2520_off(){
	cc2520ll_receiveOff();
	return 1;
}
/*---------------------------------------------------------------------------*/
/*
 * This is the poll handler function in the process below. This poll
 * handler function checks for incoming packets and delivers them to
 * the TCP/IP stack.
 */
static void
pollhandler(void)
{
	int len;
	/* 
	 * If a new packet has arrived process it.
     */
	if (cc2520_pending_packet()) {
		
   		packetbuf_clear();
    	len = cc2520_read(packetbuf_dataptr(), PACKETBUF_SIZE); // PACKET_BUF default value is 128
    	packetbuf_set_datalen(len);
    	/* 
    	 * Forward the packet to the upper level in the stack
    	 */ 
		NETSTACK_RDC.input();
   	}
   
  /*
   * Now we'll make sure that the poll handler is executed repeatedly.
   * We do this by calling process_poll() with this process as its
   * argument.
   *
   * In many cases, the hardware will cause an interrupt to be executed
   * when a new packet arrives. For such hardware devices, the interrupt
   * handler calls process_poll().
   * However in our case, the incoming packet is copied into a ring buffer 
   * in the interrupt handler to overcome the limititations in size of the 
   * fifo buffer in the cc2520. If we call process_poll() in the interrupt 
   * handler we could miss a second packet arriving before the first poll
   * is performed. 
   */
  process_poll(&cc2520_process);
}
/*---------------------------------------------------------------------------*/
/*
 * Finally, we define the process that does the work. 
 */
PROCESS_THREAD(cc2520_process, ev, data)
{
  /*
   * This process has a poll handler, so we declare it here. Note that
   * the PROCESS_POLLHANDLER() macro must come before the PROCESS_BEGIN()
   * macro.
   */
  PROCESS_POLLHANDLER(pollhandler());
  
  /*
   * This process has an exit handler, so we declare it here. Note that
   * the PROCESS_EXITHANDLER() macro must come before the PROCESS_BEGIN()
   * macro.
   */
  PROCESS_EXITHANDLER(_nop());

  /*
   * The process begins here.
   */
  PROCESS_BEGIN();

  /*
   * Now we'll make sure that the poll handler is executed initially. We do
   * this by calling process_poll() with this process as its argument.
   */
  process_poll(&cc2520_process);

  /*
   * And we wait for the process to exit.
   */
  PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_EXIT);

  /*
   * Here ends the process.
   */
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/