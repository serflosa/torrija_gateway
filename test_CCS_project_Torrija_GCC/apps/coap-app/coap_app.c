/*
 * coap_app.c
 *
 *  Created on: 03/08/2011
 *      Author: johnnyv
 */
#include "contiki.h"
#include "rest-common/rest.h"
#include "coap-app/coap_app.h"
#include "application-rfid/rfid-processes.h"
#include <stdio.h>
#include <string.h>

u8_t coap_data[128];

/*==============================================================*/
/* Resources are defined by RESOURCE macro, signature: resource name,
 * the CoAP methods it handles and its url*/
RESOURCE(hello, METHOD_GET, "hi");
/* For each resource defined, there corresponds an handler method which should be defined too.
 * Name of the handler method should be [resource name]_handler */
void
hello_handler(REQUEST* request, RESPONSE* response)
{
  sprintf(coap_data,"Torrija here!");
  rest_set_header_content_type(response, TEXT_PLAIN);
  rest_set_response_payload(response, coap_data, strlen(coap_data));
}

/*==============================================================*/
RESOURCE(discover, METHOD_GET, ".well-known/core");
void
discover_handler(REQUEST* request, RESPONSE* response)
{
  int index = 0;
  index += sprintf((char*)coap_data + index, "%s,", "</hi>;rt=\"hi\"");
  index += sprintf((char*)coap_data + index, "%s", "</rfid>;rt=\"rfid\";\"obs\"");
  rest_set_response_payload(response, coap_data, strlen((char*)coap_data));
  rest_set_header_content_type(response, APPLICATION_LINK_FORMAT);
}

/*==============================================================*/
/* Observable resource. Defines the observation relationship of a dummy
 * resource.
 * Parameters:
 * #1: resource name
 * #2: methods supported
 * #3: resource url (what needs to be in the uri-path option)
 * #4: periodic check time.
 * #5: maximum allowed number of observers */
OBSERVABLE_RESOURCE(rfid, METHOD_GET, "rfid", CLOCK_SECOND/16, COAP_MAX_OBSERVERS);

/* This handler will be executed whenever we receive a GET request for this
 * resource, independently of the observe option */
void
rfid_handler(REQUEST* request, RESPONSE* response)
{
  return;
}

/* This handler will be executed periodically to check whether the resource has
 * changed (and hence we must notify the resource's observers). It must return
 * a value other than 0 to indicate change; 0 to indicate no change */
int rfid_observable_handler(observable_resource_t* observable_resource)
{
  return rfid_new_data();
}

/* This handler will be executed whenever a notification is generated due
 * to a change in a resource under observation */
void rfid_observable_response_generator(REQUEST* response)
{
  strcpy(coap_data, get_rfid_data());
  rest_set_header_content_type(response, TEXT_PLAIN);
  rest_set_response_payload(response, coap_data, strlen((const char*)coap_data));
}

PROCESS(coap_app, "CoAP App");

PROCESS_THREAD(coap_app, ev, data)
{
  PROCESS_BEGIN();

  rest_init();

  rest_activate_resource(&resource_hello);
  rest_activate_resource(&resource_discover);
  rest_activate_observable_resource(&observable_resource_rfid);

  PROCESS_END();
}
