#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> /*for isxdigit*/
#include "contiki.h"
#include "contiki-net.h"

#include "rest-common/buffer.h"
#include "rest-coap/coap-server.h"
#include "rest-common/rest-util.h"
#include "rest-common/rest.h" /*added for periodic_resource*/

#include "dev/leds.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((u8_t *)addr)[0], ((u8_t *)addr)[1], ((u8_t *)addr)[2], ((u8_t *)addr)[3], ((u8_t *)addr)[4], ((u8_t *)addr)[5], ((u8_t *)addr)[6], ((u8_t *)addr)[7], ((u8_t *)addr)[8], ((u8_t *)addr)[9], ((u8_t *)addr)[10], ((u8_t *)addr)[11], ((u8_t *)addr)[12], ((u8_t *)addr)[13], ((u8_t *)addr)[14], ((u8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x ",(lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3],(lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

#define MAX_PAYLOAD_LEN 120
#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[uip_l2_l3_hdr_len])
static struct uip_udp_conn *server_conn;

static u16_t current_tid;

static service_callback service_cbk = NULL;

void
coap_set_service_callback(service_callback callback)
{
  service_cbk = callback;
}

/*
 * Coap Message format
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |Ver| T |  OC   |      Code     |          Message ID           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   Options (if any) ...
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   Payload (if any) ...
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *
 * Coap Message options format
 *
 *    0   1   2   3   4   5   6   7
 * +---+---+---+---+---+---+---+---+
 * | Option Delta  |    Length     | for 0..14
 * +---+---+---+---+---+---+---+---+
 * |   Option Value ...
 * +---+---+---+---+---+---+---+---+
 *                                             for 15..270:
 * +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 * | Option Delta  | 1   1   1   1 |          Length - 15          |
 * +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 * |   Option Value ...
 * +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *
 */

error_t
parse_message(coap_packet_t* packet, u8_t* buf, u16_t size)
{
  u16_t processed = 0;
  u16_t i = 0;
  PRINTF("parse_message size %d-->\n",size);

  init_packet(packet);

  packet->ver = (buf[0] & COAP_HEADER_VERSION_MASK) >> COAP_HEADER_VERSION_POSITION;
  packet->type = (buf[0] & COAP_HEADER_TYPE_MASK) >> COAP_HEADER_TYPE_POSITION;
  packet->option_count = buf[0] & COAP_HEADER_OPTION_COUNT_MASK;
  packet->code = buf[1];
  packet->tid = (buf[2] << 8) + buf[3];

  processed += 4;

  if (packet->option_count) {
    u8_t option_index = 0;
    u8_t option_delta;
    u16_t option_len;
    u8_t* option_buf = buf + processed;
    packet->options = (header_option_t*)allocate_buffer(sizeof(header_option_t) * packet->option_count);

    if (!packet->options) {
      return MEMORY_ALLOC_ERR;
    }
    header_option_t* current_option = packet->options;
    header_option_t* prev_option = NULL;
    while ((option_index < packet->option_count) && (i <= (size - processed))) {
      option_delta = (option_buf[i] & COAP_HEADER_OPTION_DELTA_MASK)
          >> COAP_HEADER_OPTION_DELTA_POSITION;
      option_len = (option_buf[i] & COAP_HEADER_OPTION_SHORT_LENGTH_MASK);
      i++;
      if (option_len == 0xf) {
        option_len += option_buf[i];
        i++;
      }

      current_option->option = option_delta;
      current_option->len = option_len;
      current_option->value = option_buf + i;
      if (option_index) {
        prev_option->next = current_option;
        /*This field defines the difference between the option Type of
         * this option and the previous option (or zero for the first option)*/
        current_option->option += prev_option->option;
      }

      if (current_option->option == Option_Type_Uri_Path) {
        if (!packet->url) {
          /* Allocate space for url */
          packet->url = allocate_buffer(MAX_URI_PATH_LENGTH);
          if (!packet->url){
            return MEMORY_ALLOC_ERR;
          }
        } else {
          if (packet->url_len + 1 <= MAX_URI_PATH_LENGTH) {
            packet->url[packet->url_len++] = '/';
          } else {
            return MEMORY_ALLOC_ERR;
          }
        }
        if (packet->url_len + current_option->len <= MAX_URI_PATH_LENGTH) {
          strncpy(&packet->url[packet->url_len], (char*) current_option->value,
                  current_option->len);
          packet->url_len += current_option->len;
        } else {
          return MEMORY_ALLOC_ERR;
        }
      } else if (current_option->option == Option_Type_Uri_Query) {
        packet->query = (char*) current_option->value;
        packet->query_len = current_option->len;
      }

      i += option_len;
      option_index++;
      prev_option = current_option;
      if (option_index < packet->option_count) {
        /* There are still options to parse */
        current_option++;
      }
    }
    if (i > (size - processed)) {
      /* Out of bounds. Bad request? */
      return BAD_REQUEST_ERR;
    }
    current_option->next = NULL;
  }
  processed += i;

  if (processed < size) {
    packet->payload = &buf[processed];
    packet->payload_len = size - processed;
  }

  /*FIXME url is not decoded - is necessary?*/
  return NO_ERROR;
  /*If query is not already provided via Uri_Query option then check URL*/
  // FIXME: I-D specifies that it is the client who must do this, not the server
//  if (packet->url && !packet->query) {
//    if ((packet->query = strchr(packet->url, '?'))) {
//      u16_t total_url_len = packet->url_len;
//      /*set query len and update url len so that it does not include query part now*/
//      packet->url_len = packet->query - packet->url;
//      packet->query++;
//      packet->query_len = packet->url + total_url_len - packet->query;
//
//      PRINTF("url %s, url_len %u, query %s, query_len %u\n", packet->url, packet->url_len, packet->query, packet->query_len);
//    }
//  }

//  PRINTF("PACKET ver:%d type:%d oc:%d \ncode:%d tid:%u url:%s len:%u payload:%s pay_len %u\n", (int)packet->ver, (int)packet->type, (int)packet->option_count, (int)packet->code, packet->tid, packet->url, packet->url_len, packet->payload, packet->payload_len);
}

int
coap_get_query_variable(coap_packet_t* packet, const char *name, char* output, u16_t output_size)
{
  if (packet->query) {
    return get_variable(name, packet->query, packet->query_len, output, output_size, 0);
  }

  return 0;
}

int
coap_get_post_variable(coap_packet_t* packet, const char *name, char* output, u16_t output_size)
{
  if (packet->payload) {
    return get_variable(name, packet->payload, packet->payload_len, output, output_size, 1);
  }

  return 0;
}

static header_option_t*
allocate_header_option(u16_t variable_len)
{
  PRINTF("sizeof header_option_t %u variable size %u\n", sizeof(header_option_t), variable_len);
  u8_t* buffer = allocate_buffer(sizeof(header_option_t) + variable_len);
  if (buffer){
    header_option_t* option = (header_option_t*) buffer;
    option->next = NULL;
    option->len = 0;
    option->value = buffer + sizeof(header_option_t);
    return option;
  }

  return NULL;
}

/*FIXME : does not overwrite the same option yet.*/
u8_t
coap_set_option(coap_packet_t* packet, option_type option_type, u16_t len, u8_t* value)
{
  PRINTF("coap_set_option len %u\n", len);
  header_option_t* option = allocate_header_option(len);
  if (option){
    option->next = NULL;
    option->len = len;
    option->option = option_type;
    memcpy(option->value, value, len);
    header_option_t* option_current = packet->options;
    header_option_t* prev = NULL;
    while (option_current){
      if (option_current->option > option->option){
        break;
      }
      prev = option_current;
      option_current = option_current->next;
    }

    if (!prev){
      if (option_current){
        option->next = option_current;
      }
      packet->options = option;
    } else{
      option->next = option_current;
      prev->next = option;
    }

    packet->option_count++;

    PRINTF("option->len %u option->option %u option->value %x next %x\n", option->len, option->option, (unsigned int) option->value, (unsigned int)option->next);

//    int i = 0;
//    for ( ; i < option->len ; i++ ){
//      PRINTF(" (%u)", option->value[i]);
//    }
//    PRINTF("\n");

    return 1;
  }

  return 0;
}

header_option_t*
coap_get_option(coap_packet_t* packet, option_type option_type)
{
  PRINTF("coap_get_option count: %u--> \n", packet->option_count);
  int i = 0;

  header_option_t* current_option = packet->options;
  for (; i < packet->option_count; current_option = current_option->next, i++) {
    PRINTF("Current option: %u\n", current_option->option);
    if (current_option->option == option_type){
      return current_option;
    }
  }

  return NULL;
}

static void
fill_error_packet(coap_packet_t* packet, error_t error, u16_t tid)
{
  packet->ver=1;
  packet->option_count=0;
  packet->url=NULL;
  packet->options=NULL;
  packet->tid=tid;
  //FIXME message type could also be NON depending on the request!
  packet->type=MESSAGE_TYPE_ACK;
  //FIXME: add a proper (human-readable error message)
  packet->payload = NULL;
  packet->payload_len = 0;
  switch (error) {
  case BAD_REQUEST_ERR:
    coap_set_code(packet, BAD_REQUEST_400);
    break;
  case MEMORY_ALLOC_ERR:
  case MEMORY_BOUNDARY_EXCEEDED:
    coap_set_code(packet, INTERNAL_SERVER_ERROR_500);
    break;
  default:
    break;
  }
}

static void
init_response(coap_packet_t* request, coap_packet_t* response)
{
  init_packet(response);
  response->code = CONTENT_205;
  if(request->type == MESSAGE_TYPE_CON) {
    response->tid = request->tid;
    response->type = MESSAGE_TYPE_ACK;
  } else if (request->type == MESSAGE_TYPE_NON) {
    response->type = MESSAGE_TYPE_NON;
    //FIXME: use a proper tid
    response->tid = current_tid++;
  }
  response->payload = NULL;
  response->payload_len = 0;
}

u16_t
coap_get_payload(coap_packet_t* packet, u8_t** payload)
{
  if (packet->payload) {
    *payload = packet->payload;
    return packet->payload_len;
  } else {
    *payload = NULL;
    return 0;
  }
}

int
coap_set_payload(coap_packet_t* packet, u8_t* payload, u16_t size)
{
  packet->payload = copy_to_buffer(payload, size);
  if (packet->payload) {
    packet->payload_len = size;
    return 1;
  }

  return 0;
}

int
coap_set_header_content_type(coap_packet_t* packet, content_type_t content_type)
{
  u16_t len = 1;

  return coap_set_option(packet, Option_Type_Content_Type, len, (u8_t*) &content_type);
}

content_type_t
coap_get_header_content_type(coap_packet_t* packet)
{
  header_option_t* option = coap_get_option(packet, Option_Type_Content_Type);
  if (option){
    return (u8_t)(*(option->value));
  }

  return DEFAULT_CONTENT_TYPE;
}

//int
//coap_get_header_subscription_lifetime(coap_packet_t* packet, u32_t* lifetime)
//{
//  PRINTF("coap_get_header_subscription_lifetime --> \n");
//  header_option_t* option = coap_get_option(packet, Option_Type_Subscription_Lifetime);
//  if (option){
//    PRINTF("Subs Found len %u (first byte %u)\n", option->len, (u16_t)option->value[0]);
//
//    *lifetime = read_int(option->value, option->len);
//    return 1;
//  }
//
//  return 0;
//}
//
//int
//coap_set_header_subscription_lifetime(coap_packet_t* packet, u32_t lifetime)
//{
//  u8_t temp[4];
//  u16_t len = write_variable_int(temp, lifetime);
//
//  return coap_set_option(packet, Option_Type_Subscription_Lifetime, len, temp);
//}

int
coap_get_header_block(coap_packet_t* packet, block_option_t* block)
{
  u32_t all_block;
  PRINTF("coap_get_header_block --> \n");
  header_option_t* option = coap_get_option(packet, Option_Type_Block);
  if (option){
    PRINTF("Block Found len %u (first byte %u)\n", option->len, (u16_t)option->value[0]);

    all_block = read_int(option->value, option->len);
    block->number = all_block >> 4;
    block->more = (all_block & 0x8) >> 3;
    block->size = (all_block & 0x7);
    return 1;
  }

  return 0;
}

int
coap_set_header_block(coap_packet_t* packet, u32_t number, u8_t more, u8_t size)
{
  u8_t temp[4];
  size = log_2(size/16);
  number = number << 4;
  number |= (more << 3) & 0x8;
  number |= size & 0x7;

  u16_t len = write_variable_int(temp, number);
  PRINTF("number %lu, more %u, size %u block[0] %u block[1] %u block[2] %u block[3] %u\n",
      number, (u16_t)more, (u16_t)size, (u16_t)temp[0], (u16_t)temp[1], (u16_t)temp[2], (u16_t)temp[3]);
  return coap_set_option(packet, Option_Type_Block, len, temp);
}


int
coap_set_header_uri(coap_packet_t* packet, char* uri)
{
  return coap_set_option(packet, Option_Type_Uri_Path, strlen(uri), (u8_t*) uri);
}

int
coap_set_header_etag(coap_packet_t* packet, u8_t* etag, u8_t size)
{
  return coap_set_option(packet, Option_Type_Etag, size, etag);
}

void coap_set_type(coap_packet_t* packet, message_type type)
{
  packet->type = type;
}

void
coap_set_code(coap_packet_t* packet, status_code_t code)
{
  packet->code = (u8_t)code;
}

coap_method_t
coap_get_method(coap_packet_t* packet)
{
  return (coap_method_t)packet->code;
}

void
coap_set_method(coap_packet_t* packet, coap_method_t method)
{
  packet->code = (u8_t)method;
}

//static void send_request(coap_packet_t* request, struct uip_udp_conn *client_conn)
//{
//  char buf[MAX_PAYLOAD_LEN];
//  int data_size = 0;
//
//  data_size = serialize_packet(request, buf);
//
//  PRINTF("Created a connection with the server ");
//  PRINT6ADDR(&client_conn->ripaddr);
//  PRINTF(" local/remote port %u/%u\n",
//      uip_htons(client_conn->lport), uip_htons(client_conn->rport));
//
//  PRINTF("Sending to: ");
//  PRINT6ADDR(&client_conn->ripaddr);
//  uip_udp_packet_send(client_conn, buf, data_size);
//}

static void send_response(RESPONSE* response, observer_t* o)
{
  char buf[MAX_PAYLOAD_LEN];
  int data_size = 0;

  data_size = serialize_packet(response, buf);

  uip_ipaddr_copy(&server_conn->ripaddr, &o->client_ipaddr);
  server_conn->rport = UIP_HTONS(o->client_port);
  uip_udp_packet_send(server_conn, buf, data_size);
  /* Restore server connection to allow data from any node */
  memset(&server_conn->ripaddr, 0, sizeof(server_conn->ripaddr));
  server_conn->rport = 0;
}


static int
handle_incoming_data(void)
{
  //TODO: handle RST messages
  error_t error = NO_ERROR;
  char buf[MAX_PAYLOAD_LEN];

  PRINTF("uip_datalen received %u \n",(u16_t)uip_datalen());

  u8_t* data = (u8_t*)((u8_t*)uip_appdata + uip_ext_len);
  u16_t datalen = uip_datalen() - uip_ext_len;
  coap_packet_t* request = NULL;
  coap_packet_t* response = NULL;
  int data_size = 0;

  if (uip_newdata()) {
    if (init_buffer(COAP_DATA_BUFF_SIZE)) {
      request = (coap_packet_t*)allocate_buffer(sizeof(coap_packet_t));
      if(request) {
        error = parse_message(request, data, datalen);
        if (error == NO_ERROR) {
          uip_ipaddr_copy(&request->addr, &UIP_IP_BUF->srcipaddr);
          request->port = UIP_HTONS(UIP_UDP_BUF->srcport);
          if (request->type != MESSAGE_TYPE_ACK) {
            response = (coap_packet_t*)allocate_buffer(sizeof(coap_packet_t));
            if (response) {
              init_response(request, response);

              if (service_cbk) {
                service_cbk(request, response);
              }
            } else {
              error = MEMORY_ALLOC_ERR;
            }
          }
        }
      } else {
        error = MEMORY_ALLOC_ERR;
      }
    } else {
      error = MEMORY_ALLOC_ERR;
    }
    if ((request) && (request->type != MESSAGE_TYPE_ACK) && (error != NO_ERROR)) {
      /* There has been an error */
      /*FIXME : Crappy way of accessing TID of the incoming packet, fix it!*/
      coap_packet_t error_packet;
      fill_error_packet(&error_packet, error, (data[2] << 8) + data[3]);
      response = &error_packet;
    }

    if (response) {
      data_size = serialize_packet(response, (u8_t*)buf);

      uip_ipaddr_copy(&server_conn->ripaddr, &UIP_IP_BUF->srcipaddr);
      server_conn->rport = UIP_UDP_BUF->srcport;

      PRINTF("Responding with message size: %d\n",data_size);
      uip_udp_packet_send(server_conn, buf, data_size);
      /* Restore server connection to allow data from any node */
      memset(&server_conn->ripaddr, 0, sizeof(server_conn->ripaddr));
      server_conn->rport = 0;
    }

    /* In all cases, clear the buffer for further use */
    delete_buffer();

  }

  return error;
}

process_event_t observable_resource_changed_event;

void
observable_resource_changed(observable_resource_t* observable_resource)
{
  process_post(&coap_server,
               observable_resource_changed_event,
               (process_data_t)observable_resource);
}

//void
//resource_changed(struct periodic_resource_t* resource)
//{
//  process_post(&coap_server, observable_resource_changed_event, (process_data_t)resource);
//}

/*---------------------------------------------------------------------------*/
PROCESS(coap_server, "Coap Server");
PROCESS_THREAD(coap_server, ev, data)
{
  u16_t seconds;
  observer_t* observer;

  PROCESS_BEGIN();
  PRINTF("COAP SERVER\n");

  current_tid = random_rand();

  observable_resource_changed_event = process_alloc_event();

  /* new connection with remote host */
  server_conn = udp_new(NULL, uip_htons(0), NULL);
  udp_bind(server_conn, uip_htons(MOTE_SERVER_LISTEN_PORT));
  PRINTF("Local/remote port %u/%u\n", uip_htons(server_conn->lport), uip_htons(server_conn->rport));

  while(1) {
    PROCESS_YIELD();

    if(ev == tcpip_event) {
      handle_incoming_data();
    } else if (ev == observable_resource_changed_event) {
      observable_resource_t* observable_resource = (observable_resource_t*)data;
      PRINTF("resource_changed_event \n");
      for(observer = (observer_t*)list_head(observable_resource->observers);
          observer;
          observer = observer->next) {
        /* Doing this for each observer is suboptimal since almost the entire
         * response is the same for each observer. However, it is necessary
         * since it is not possible to deallocate part of the buffer. */
        if (init_buffer(COAP_DATA_BUFF_SIZE)) {
          coap_packet_t* response = (coap_packet_t*)allocate_buffer(sizeof(coap_packet_t));
          init_packet(response);
          //TODO: add CON/NON selector on OBSERVABLE_RESOURCE macro
          coap_set_type(response, MESSAGE_TYPE_CON);
          coap_set_code(response, CONTENT_205);
          response->tid = current_tid++;
          seconds = (u16_t)clock_seconds();
          if (seconds == 0) {
            coap_set_option(response, Option_Type_Observe, 0, (u8_t*)&seconds);
          } else if (seconds <= 0xFF) {
            coap_set_option(response, Option_Type_Observe, 1, (u8_t*)&seconds);
          } else {
            coap_set_option(response, Option_Type_Observe, 2, (u8_t*)&seconds);
          }
          if (observer->token_len) {
            coap_set_option(response, Option_Type_Token,
                            observer->token_len, observer->token);
          }
          if (observable_resource->observable_response_generator) {
            observable_resource->observable_response_generator(response);
          }
          send_response(response, observer);
          delete_buffer();
        }
//
//        if (!resource->client_conn) {
//          /*FIXME send port is fixed for now to 61616*/
//          resource->client_conn = udp_new(&resource->addr, uip_htons(61616), NULL);
//          udp_bind(resource->client_conn, uip_htons(MOTE_CLIENT_LISTEN_PORT));
//        }
//
//        if (resource->client_conn) {
//        }

      }
//      periodic_resource_t* resource = (periodic_resource_t*)data;
//      PRINTF("resource_changed_event \n");
//
//      if (init_buffer(COAP_DATA_BUFF_SIZE)) {
//        coap_packet_t* request = (coap_packet_t*)allocate_buffer(sizeof(coap_packet_t));
//        init_packet(request);
//        coap_set_method(request, COAP_GET);
//        request->tid = current_tid++;
//        coap_set_header_subscription_lifetime(request, resource->lifetime);
//        coap_set_header_uri(request, (char *)resource->resource->url);
//        if (resource->periodic_request_generator) {
//          resource->periodic_request_generator(request);
//        }
//
//        if (!resource->client_conn) {
//          /*FIXME send port is fixed for now to 61616*/
//          resource->client_conn = udp_new(&resource->addr, uip_htons(61616), NULL);
//          udp_bind(resource->client_conn, uip_htons(MOTE_CLIENT_LISTEN_PORT));
//        }
//
//        if (resource->client_conn) {
//          send_request(request, resource->client_conn);
//        }
//
//        delete_buffer();
//      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
