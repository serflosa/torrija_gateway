#ifndef USB_PROCESSES_H_
#define USB_PROCESSES_H_



uint8_t usb_init_proc2(uint8_t i);
void usb_init_proc(void);
/* We declare the processes */

PROCESS_NAME(usb_input_process);

struct usb_packet{
  char addr_sensor[64];
  char port[5];
  char request[10];
  char resource[20];
  char payload[14];
};

#endif /*USB_PROCESSES_H_*/
