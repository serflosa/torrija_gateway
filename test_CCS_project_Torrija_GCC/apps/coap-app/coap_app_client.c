#include <stdio.h>
#include <stdlib.h>
#include "contiki.h"
#include "contiki-net.h"
#include "rest-common/rest.h"
#include "rest-common/buffer.h"
#include "dev/leds_torrija.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x", ((u8_t *)addr)[0], ((u8_t *)addr)[1], ((u8_t *)addr)[2], ((u8_t *)addr)[3], ((u8_t *)addr)[4], ((u8_t *)addr)[5], ((u8_t *)addr)[6], ((u8_t *)addr)[7], ((u8_t *)addr)[8], ((u8_t *)addr)[9], ((u8_t *)addr)[10], ((u8_t *)addr)[11], ((u8_t *)addr)[12], ((u8_t *)addr)[13], ((u8_t *)addr)[14], ((u8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x ",(lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3],(lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

#define LOCAL_PORT 61617
#define REMOTE_PORT 61616

char temp[100];
int xact_id;
static uip_ipaddr_t server_ipaddr;
static struct uip_udp_conn *client_conn;
#define MAX_PAYLOAD_LEN   100

#define NUMBER_OF_URLS 3
char* service_url = {"toggle"};
static unsigned char outStr_buf[50]="";

static void
response_handler(coap_packet_t* response)
{
  uint16_t payload_len = 0;
  uint8_t* payload = NULL;
  payload_len = coap_get_payload(response, &payload);

  PRINTF("Response transaction id: %u", response->tid);
  if (payload) {
    memcpy(temp, payload, payload_len);
    temp[payload_len] = 0;
    PRINTF(" payload: %s\n", temp);
  }
}

static void
send_data(void)
{
  char buf[MAX_PAYLOAD_LEN];

  if (init_buffer(COAP_DATA_BUFF_SIZE)) {
    int data_size = 0;
    coap_packet_t* request = (coap_packet_t*)allocate_buffer(sizeof(coap_packet_t));
    init_packet(request);

    coap_set_method(request, COAP_POST);
    request->tid = xact_id++;
    request->type = MESSAGE_TYPE_NON;
    coap_set_header_uri(request, service_url);

    data_size = serialize_packet(request, buf);

//    PRINTF("Client sending request to:[");
//    PRINT6ADDR(&client_conn->ripaddr);
//    PRINTF("]:%u/%s\n", (uint16_t)REMOTE_PORT, service_urls[service_id]);
    uip_udp_packet_send(client_conn, buf, data_size);

    delete_buffer();
  }
}

static void
handle_incoming_data()
{
  PRINTF("Incoming packet size: %u \n", (u16_t)uip_datalen());
  if (init_buffer(COAP_DATA_BUFF_SIZE)) {
    if (uip_newdata()) {
      coap_packet_t* response = (coap_packet_t*)allocate_buffer(sizeof(coap_packet_t));
      parse_message(response, uip_appdata, uip_datalen());
      if (response) {
        response_handler(response);
      }
    }
    delete_buffer();
  }
}

PROCESS(coap_app_client, "COAP Client");

PROCESS_THREAD(coap_app_client, ev, data)
{
  PROCESS_BEGIN();

  uip_ip6addr_u8(&server_ipaddr,0xfe,0x80,0,0,0,0,0,0,0xc0,0xa0,0x08,0x39,0xd1,0xe4,0x02,0x05);
  /* new connection with server */
  client_conn = udp_new(&server_ipaddr, UIP_HTONS(REMOTE_PORT), NULL);
  udp_bind(client_conn, UIP_HTONS(LOCAL_PORT));

 // UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));

  while(1) {
    PROCESS_YIELD();
    if (ev == PROCESS_EVENT_CONTINUE) { 
      send_data();
    } else if (ev == tcpip_event) {
    	sprintf(outStr_buf,"\r\nPacket received\r\n\r\n");
		sendData_inBackground((uint8_t*)outStr_buf,strlen(outStr_buf),1,0);
      //handle_incoming_data();
    }
  }

  PROCESS_END();
}
