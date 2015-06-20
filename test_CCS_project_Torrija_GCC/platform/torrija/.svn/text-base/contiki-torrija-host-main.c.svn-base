/*
 * contiki-torrija-host-main.c 	2011/09/01
 */
#include "contiki.h"
#include "sys/autostart.h"
#include "sys/process.h"
#include "sys/procinit.h"
#include "contiki-net.h"
#include "device.h"
#include "dev/leds_torrija.h"
#include "dev/msp430_arch.h"
#include "clock_arch.h"
#include "dev/cc2520ll.h"
#include "dev/cc2520_process.h"
#include "application-rfid/rfid-processes.h"
#include "application-usb/usb-processes.h"
#include "coap-app/coap_app.h"
#include "coap-app/coap_app_client.h"

#define 	WAFERID			0x01A0A
#define 	WAFERIPOSX		0x01A0E
#define 	WAFERIPOSY		0x01A10

/*---------------------------------------------------------------------------*/
#pragma FUNC_NEVER_RETURNS(main);
void
main(void)
{
  msp430_init();
  clock_init();
	
  // initialize uip_variables
  memset(uip_buf, 0, UIP_CONF_BUFFER_SIZE);
  uip_len = 0;
	
  leds_init();
  buttons_init();
  usb_init_proc();
	
	/* Create MAC addresses based on a hash of the unique id (wafle id + x-pos 
	 * in wafel + y-pos in wafel).
	 * We use rime addresses in order to use useful rime address handling
	 * functions.
	 */
	/* The sicslowmac layer requires the mac address to be placed in the global 
	 * (extern) variable rimeaddr_node_addr (declared rimeaddr.h).
	 */
  rimeaddr_node_addr.u8[0] = NODE_BASE_ADDR0;
  rimeaddr_node_addr.u8[1] = NODE_BASE_ADDR1;
  rimeaddr_node_addr.u8[2] = NODE_BASE_ADDR2;
  rimeaddr_node_addr.u8[3] = NODE_BASE_ADDR3;
  rimeaddr_node_addr.u8[4] = NODE_BASE_ADDR4;
  rimeaddr_node_addr.u8[5] = *((unsigned char*)(WAFERID+2));	// lowest byte of wafer id
  rimeaddr_node_addr.u8[6] = *((unsigned char*)(WAFERIPOSX));	// lowest byte of x-pos in wafer
  rimeaddr_node_addr.u8[7] = *((unsigned char*)(WAFERIPOSY));	// lowest byte of y-pos in wafer

	/* The following line sets the link layer address. This must be done
 	 * before the tcpip_process is started since in its initialization
 	 * routine the function uip_netif_init() will be called from inside 
 	 * uip_init()and there the default IPv6 address will be set by combining
 	 * the link local prefix (fe80::/64)and the link layer address. 
 	 */ 
  rimeaddr_copy((rimeaddr_t*)&uip_lladdr.addr, &rimeaddr_node_addr);
  
  /* Initialize the process module */ 
  process_init();
  /* etimers must be started before ctimer_init */
  // clock_init();
  process_start(&etimer_process, NULL);
  	
  /* Start radio and radio receive process */
	if (NETSTACK_RADIO.init() == FAILED) {
		led_on(LED_RED);
		_disable_interrupts();
		LPM4; // die
	}
	
	/* Initialize stack protocols */
	queuebuf_init();
	NETSTACK_RDC.init();
	NETSTACK_MAC.init();
	NETSTACK_NETWORK.init();
    
  /* Initialize tcpip process */
	process_start(&tcpip_process, NULL);
	
	/* Initialize our application(s) */
	process_start(&rfid_find_tags, NULL);
	process_start(&usb_input_process, NULL);
	
	process_start(&coap_app_client, NULL);
	//process_start(&coap_app, NULL);
	
  /* Enter main loop */	
	while(1) {
		/* poll every running process which has requested to be polled */
    process_run();
    /* 
     * Enter low power mode 3 safely. The CPU will wake up in the timer 
     * interrupt or whenever a packet arrives. In the timer interrupt
     * routine, we check wether an etimer has expired and, if so,
     * we call the etimer_request_poll() function
     */
    //LPM3;
  }
}
