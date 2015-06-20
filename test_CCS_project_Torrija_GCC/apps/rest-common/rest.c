#include "contiki.h"
#include <string.h> /*for string operations in match_addresses*/
#include "rest.h"
#include "buffer.h"

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

/*FIXME it is possible to define some of the rest functions as MACROs rather than functions full of ifdefs.*/

PROCESS_NAME(rest_manager_process);

LIST(restful_services);
LIST(restful_observable_services);
//LIST(restful_periodic_services);

void
rest_init(void)
{
  list_init(restful_services);
  list_init(restful_observable_services);

#ifdef WITH_COAP
  coap_set_service_callback(rest_invoke_restful_service);
#else /*WITH_COAP*/
  http_set_service_callback(rest_invoke_restful_service);
#endif /*WITH_COAP*/

  /*Start rest framework process*/
  process_start(&rest_manager_process, NULL);
}

void
rest_activate_resource(resource_t* resource)
{
  /*add it to the restful web service link list*/
  list_add(restful_services, resource);
}

void
rest_activate_observable_resource(observable_resource_t* observable_resource)
{
  memb_init(observable_resource->observers_memb);
  LIST_STRUCT_INIT(observable_resource, observers);
  list_add(restful_observable_services, observable_resource);
  rest_activate_resource(observable_resource->resource);
}

/*
 * Returns the observable resource corresponding to resource. If there is no
 * observable resource corresponding to resource, returns NULL
 */
observable_resource_t*
rest_get_observable_resource(resource_t* resource)
{
  observable_resource_t* observable_resource;

  for (observable_resource = (observable_resource_t*)list_head(restful_observable_services);
      observable_resource;
      observable_resource = observable_resource->next) {
    if (observable_resource->resource == resource) {
      return observable_resource;
    }
  }
  return NULL;
}

u8_t
rest_is_observable_resource_observed(observable_resource_t* observable_resource)
{
  return (list_head(observable_resource->observers) != NULL);
}

observer_t*
rest_add_observer(observable_resource_t* obs_resource,
                  uip_ipaddr_t* addr, u16_t port)
{
  observer_t* o;
  o = memb_alloc(obs_resource->observers_memb);
  if (o) {
    /* success allocating memory for a new observer. */

    o->client_port = port;
    uip_ipaddr_copy(&o->client_ipaddr, addr);
    /* If this resource was not already being observed, start etimer for
     * periodic checks. Note that we want the event being posted to the
     * rest_manager_process process*/
    if (!rest_is_observable_resource_observed(obs_resource)) {
      PROCESS_CONTEXT_BEGIN(&rest_manager_process);
      etimer_set(obs_resource->handler_cb_timer,
                 obs_resource->check_period);
      PROCESS_CONTEXT_END(&rest_manager_process);
    }
    list_add(obs_resource->observers, o);
  }
  return o;
}

void
rest_remove_observer(observable_resource_t* obs_resource, observer_t* o)
{
  list_remove(obs_resource->observers, o);
  memb_free(obs_resource->observers_memb, o);
  if (!rest_is_observable_resource_observed(obs_resource)) {
    /* No one else is observing this resource; we can stop the etimer */
    etimer_stop(obs_resource->handler_cb_timer);
  }
  return;
}

//void
//rest_activate_periodic_resource(periodic_resource_t* periodic_resource)
//{
//  list_add(restful_periodic_services, periodic_resource);
//  rest_activate_resource(periodic_resource->resource);
//}

void
rest_set_user_data(resource_t* resource, void* user_data)
{
  resource->user_data = user_data;
}

void*
rest_get_user_data(resource_t* resource)
{
  return resource->user_data;
}

void
rest_set_pre_handler(resource_t* resource, restful_pre_handler pre_handler)
{
  resource->pre_handler = pre_handler;
}

void
rest_set_post_handler(resource_t* resource, restful_post_handler post_handler)
{
  resource->post_handler = post_handler;
}

list_t
rest_get_resources(void)
{
  return restful_services;
}

void
rest_set_response_status(RESPONSE* response, status_code_t status)
{
#ifdef WITH_COAP
  coap_set_code(response, status);
#else /*WITH_COAP*/
  http_set_status(response, status);
#endif /*WITH_COAP*/
}

#ifdef WITH_COAP
static method_t coap_to_rest_method(coap_method_t method)
{
  return (method_t)(1 << (method - 1));
}

static coap_method_t rest_to_coap_method(method_t method)
{
  coap_method_t coap_method = COAP_GET;
  switch (method) {
  case METHOD_GET:
    coap_method = COAP_GET;
    break;
  case METHOD_POST:
    coap_method = COAP_POST;
      break;
  case METHOD_PUT:
    coap_method = COAP_PUT;
      break;
  case METHOD_DELETE:
    coap_method = COAP_DELETE;
      break;
  default:
    break;
  }
  return coap_method;
}
#endif /*WITH_COAP*/

method_t
rest_get_method_type(REQUEST* request)
{
#ifdef WITH_COAP
  return coap_to_rest_method(coap_get_method(request));
#else
  return (method_t)(request->request_type);
#endif
}

/*Only defined for COAP for now.*/
#ifdef WITH_COAP
void
rest_set_method_type(REQUEST* request, method_t method)
{
  coap_set_method(request, rest_to_coap_method(method));
}
#endif /*WITH_COAP*/

void
rest_set_response_payload(RESPONSE* response, u8_t* payload, u16_t size)
{
#ifdef WITH_COAP
  coap_set_payload(response, payload, size);
#else
  http_set_res_payload(response, payload, size);
#endif /*WITH_COAP*/
}

/*Only defined for COAP for now.*/
#ifdef WITH_COAP
void
rest_set_request_payload(REQUEST* request, u8_t* payload, u16_t size)
{
  coap_set_payload(request, payload, size);
}
#endif /*WITH_COAP*/

int
rest_get_query_variable(REQUEST* request, const char *name, char* output, u16_t output_size)
{
#ifdef WITH_COAP
  return coap_get_query_variable(request, name, output, output_size);
#else
  return http_get_query_variable(request, name, output, output_size);
#endif /*WITH_COAP*/
}

int
rest_get_post_variable(REQUEST* request, const char *name, char* output, u16_t output_size)
{
#ifdef WITH_COAP
  return coap_get_post_variable(request, name, output, output_size);
#else
  return http_get_post_variable(request, name, output, output_size);
#endif /*WITH_COAP*/
}

content_type_t
rest_get_header_content_type(REQUEST* request)
{
#ifdef WITH_COAP
  return coap_get_header_content_type(request);
#else
  return http_get_header_content_type(request);
#endif /*WITH_COAP*/
}

int
rest_set_header_content_type(RESPONSE* response, content_type_t content_type)
{
#ifdef WITH_COAP
  return coap_set_header_content_type(response, content_type);
#else
  return http_set_res_header(response, HTTP_HEADER_NAME_CONTENT_TYPE, http_get_content_type_string(content_type), 1);
#endif /*WITH_COAP*/

}

int
rest_set_header_etag(RESPONSE* response, u8_t* etag, u8_t size)
{
#ifdef WITH_COAP
  return coap_set_header_etag(response, etag, size);
#else
  /*FIXME for now etag should be a "/0" ending string for http part*/
  char temp_etag[10];
  memcpy(temp_etag, etag, size);
  temp_etag[size] = 0;
  return http_set_res_header(response, HTTP_HEADER_NAME_ETAG, temp_etag, 1);
#endif /*WITH_COAP*/
}

int
rest_invoke_restful_service(REQUEST* request, RESPONSE* response)
{
  int found = 0;
  const char* url = request->url;
  u16_t url_len = request->url_len;
  header_option_t* token_option;
#ifdef WITH_COAP
  header_option_t* observe_option;
  u16_t seconds;
#endif /* WITH_COAP */
  PRINTF("rest_invoke_restful_service url %s url_len %d -->\n", url, url_len);

  resource_t* resource = NULL;
  observable_resource_t* observable_resource = NULL;
  observer_t* observer;

  for (resource = (resource_t*)list_head(restful_services); resource; resource = resource->next) {
    /*if the web service handles that kind of requests and urls matches*/
    if (url && strlen(resource->url) == url_len && strncmp(resource->url, url, url_len) == 0){
      found = 1;
      method_t method = rest_get_method_type(request);

      PRINTF("method %u, resource->methods_to_handle %u\n", (u16_t)method, resource->methods_to_handle);

      if (resource->methods_to_handle & method) {
#ifdef WITH_COAP
        /* First of all, retrieve and echo the token option in the response */
        token_option = coap_get_option(request, Option_Type_Token);
        if (token_option) {
          coap_set_option(response, Option_Type_Token,
                          token_option->len, token_option->value);
        }
        if (method == METHOD_GET) {
          /* Check if the client is requesting to create (or delete) an
           * observation relationship */
          observable_resource = rest_get_observable_resource(resource);
          if (observable_resource) {
            /* This is an observable resource. Check if there is already an
             * observation relationship between the client and this resource */
            for (observer = (observer_t*)list_head(observable_resource->observers);
                observer;
                observer = observer->next) {
              /* Check if observation relationship exists (note that token value must
               * not be taken into account; only IP address and port matter) */
              if (observer->client_port == request->port &&
                  uip_ipaddr_cmp(&observer->client_ipaddr, &request->addr)) {
                break;
              }
            }
            /* Retrieve observe option */
            observe_option = coap_get_option(request, Option_Type_Observe);
            if (observe_option) {
              /* We are going to create or refresh a relationship. */
              if (!observer) {
                /* Try to create a new observation relationship */
                observer = rest_add_observer(observable_resource,
                                             &request->addr, request->port);
              }
              if (observer) {
                /* There was already an observation relationship or there
                 * was not but we succeeded to create one. Add observe option
                 * to response */
                seconds = (u16_t)clock_seconds();
                if (seconds == 0) {
                  coap_set_option(response,
                                  Option_Type_Observe, 0, (u8_t*)&seconds);
                } else if (seconds <= 0xFF) {
                  coap_set_option(response,
                                  Option_Type_Observe, 1, (u8_t*)&seconds);
                } else {
                  coap_set_option(response,
                                  Option_Type_Observe, 2, (u8_t*)&seconds);
                }
                /* If present, attach token to the relationship */
                if (token_option) {
                  memcpy(observer->token, token_option->value, token_option->len);
                  observer->token_len = token_option->len;
                } else {
                  /* Default value for token option is "empty". Assume
                   * existing and empty token value if there is no option */
                  observer->token_len = 0;
                }
              }
            } else if (!observe_option && observer){
              /* We received a GET with no observe option for a resource
               * with which the client had an observation relationship.
               * Remove relationship as per I-D.ietf-core-observe */
              rest_remove_observer(observable_resource, observer);
            }
          }
        }
#endif /*WITH_COAP*/
        /*call pre handler if it exists*/
        if (!resource->pre_handler || resource->pre_handler(request, response)) {
          /* call handler function*/
          resource->handler(request, response);

          /*call post handler if it exists*/
          if (resource->post_handler) {
            resource->post_handler(request, response);
          }
        }
      } else {
        rest_set_response_status(response, METHOD_NOT_ALLOWED_405);
      }
      break;
    }
  }

  if (!found) {
    rest_set_response_status(response, NOT_FOUND_404);
  }

  return found;
}

PROCESS(rest_manager_process, "Rest Process");

PROCESS_THREAD(rest_manager_process, ev, data)
{
  PROCESS_BEGIN();

  /*start the coap or http server*/
  process_start(SERVER_PROCESS, NULL);

  PROCESS_PAUSE();

  /*Periodic resources are only available to COAP implementation*/
#ifdef WITH_COAP

  observable_resource_t* observable_resource = NULL;
//  for (observable_resource = (observable_resource_t*)list_head(restful_observable_services);
//      observable_resource;
//      observable_resource = observable_resource->next) {
//    if (observable_resource->check_period) {
//      PRINTF("Set timer for Res: %s to %lu\n", observable_resource->resource->url, observable_resource->check_period);
//      etimer_set(observable_resource->handler_cb_timer, observable_resource->check_period);
//    }
//  }

//  periodic_resource_t* periodic_resource = NULL;
//  for (periodic_resource = (periodic_resource_t*)list_head(restful_periodic_services); periodic_resource; periodic_resource = periodic_resource->next) {
//    if (periodic_resource->period) {
//      PRINTF("Set timer for Res: %s to %lu\n", periodic_resource->resource->url, periodic_resource->period);
//      etimer_set(periodic_resource->handler_cb_timer, periodic_resource->period);
//    }
//  }

  while(1) {
    PROCESS_WAIT_EVENT();
    if (ev == PROCESS_EVENT_TIMER) {
      for (observable_resource = (observable_resource_t*)list_head(restful_observable_services);
          observable_resource;
          observable_resource = observable_resource->next) {
        if (rest_is_observable_resource_observed(observable_resource) &&
            etimer_expired(observable_resource->handler_cb_timer)) {
          /*call the observable handler function if exists*/
          if (observable_resource->observable_handler) {
            if ((observable_resource->observable_handler)(observable_resource)) {
              observable_resource_changed(observable_resource);
            }
          }
          etimer_reset(observable_resource->handler_cb_timer);
        }
      }
    }
//      for (periodic_resource = (periodic_resource_t*)list_head(restful_periodic_services);periodic_resource;periodic_resource = periodic_resource->next) {
//        if (periodic_resource->period && etimer_expired(periodic_resource->handler_cb_timer)) {
//          PRINTF("Etimer expired for %s (period:%lu life:%lu)\n", periodic_resource->resource->url, periodic_resource->period, periodic_resource->lifetime);
//          /*call the periodic handler function if exists*/
//          if (periodic_resource->periodic_handler) {
//            if ((periodic_resource->periodic_handler)(periodic_resource->resource)) {
//              PRINTF("RES CHANGE\n");
//              if (!stimer_expired(periodic_resource->lifetime_timer)) {
//                PRINTF("TIMER NOT EXPIRED\n");
//                resource_changed(periodic_resource);
//                periodic_resource->lifetime = stimer_remaining(periodic_resource->lifetime_timer);
//              } else {
//                periodic_resource->lifetime = 0;
//              }
//            }
//
//            PRINTF("%s lifetime %lu (%lu) expired %d\n", periodic_resource->resource->url, stimer_remaining(periodic_resource->lifetime_timer), periodic_resource->lifetime, stimer_expired(periodic_resource->lifetime_timer));
//          }
//          etimer_reset(periodic_resource->handler_cb_timer);
//        }
//      }
  }
#endif /*WITH_COAP*/

  PROCESS_END();
}
