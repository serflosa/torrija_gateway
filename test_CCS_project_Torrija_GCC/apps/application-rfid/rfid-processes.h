#ifndef RFIDPROCESSES_H_
#define RFIDPROCESSES_H_

//#include "contiki.h"
//#include "contiki-net.h"
//#include "usb-processes.h"

extern struct etimer handler_cb_timer_rfid;

u8_t* get_rfid_data(void);
u8_t rfid_new_data(void);
/* We declare the processes */
PROCESS_NAME(rfid_find_tags);
PROCESS_NAME(tagit_inventory_request);
#endif /*RFIDPROCESSES_H_*/
