/* -*- C -*- */
/* @(#)$Id: contiki-conf.h,v 1.31 2008/11/06 20:45:06 nvt-se Exp $ */

#ifndef CONTIKI_CONF_H
#define CONTIKI_CONF_H

/* Compiler specific includes */
#include "device.h"
#include <inttypes.h>


#include "clock_arch.h"

#ifdef USE_CONF_APP
#include "contiki-conf-app.h"
#endif

/// COMPILER AND CPU PARAMETERS ///

#define CCIF
#define CLIF

#define AUTOSTART_ENABLE 0
#define CC_CONF_REGISTER_ARGS 1
#define CC_CONF_FUNCTION_POINTER_ARGS 1
#define CC_CONF_INLINE inline
#define CONF_DATA_ALIGNMENT 1
/**
 * 8 bit datatype
 *
 * This typedef defines the 8-bit type used throughout contiki.
 */
typedef unsigned char u8_t;

/**
 * 16 bit datatype
 *
 * This typedef defines the 16-bit type used throughout contiki.
 */
typedef unsigned int u16_t;

/**
 * 32 bit datatype
 *
 * This typedef defines the 32-bit type used throughout contiki.
 */
typedef unsigned long u32_t;


typedef u16_t clock_time_t;

/// UIP SECTION ///

/**
 * Statistics datatype
 *
 * This typedef defines the dataype used for keeping statistics in
 * uIP.
 */
typedef unsigned int uip_stats_t;

/* 
 * IP version is 6
 */
#define WITH_UIP6   1

/*
 * We will use rime addresses to take advantage of the address handling 
 * functions provided by rime.
 */
#define RIMEADDR_CONF_SIZE              8

/* 
 * Link layer is IEEE 802.15.4
 */
#define UIP_CONF_LL_802154              1

/*
 * Stack definitions. This is new in Contiki_2.x
 */
#define NETSTACK_CONF_NETWORK	sicslowpan_driver
#define NETSTACK_CONF_MAC		nullmac_driver
#define NETSTACK_CONF_RDC		sicslowmac_driver
#define NETSTACK_CONF_FRAMER	framer_802154
#define NETSTACK_CONF_RADIO		cc2520_driver



#define UIP_CONF_IPV6                   1
#define UIP_CONF_IPV6_QUEUE_PKT         0
#define UIP_CONF_IPV6_CHECKS            1
#define UIP_CONF_IPV6_REASSEMBLY        0
#define UIP_CONF_NETIF_MAX_ADDRESSES    3
#define UIP_CONF_ND6_MAX_PREFIXES       3
#define UIP_CONF_ND6_MAX_NEIGHBORS      10
#define UIP_CONF_ND6_MAX_DEFROUTERS     1
#define UIP_CONF_IP_FORWARD             0

/*
 * Redefine the value of RETRANS_TIMER to 2000 milliseconds
 */
#define UIP_CONF_ND6_RETRANS_TIMER		2000

/* 
 * Setting the LLH to 0 we can make use of the full uip_buf buffer. Note that
 * the link layer header will be on the rime buffer and not in the uip_buf.
 */
#define UIP_CONF_LLH_LEN						0

/* 
 * UIP_CONF_ICMP6 enables the use of application level events, callbacks
 * and app polling in case ICMPv6 packet arrives.
 */
#define UIP_CONF_ICMP6							0

/* 
 * Enable UDP support.
 */
#define UIP_CONF_UDP							1

/*
 * Enable UDP Checksums. If 0 they are not sent in the packet nor
 * verified at packet arrival.
 */
#define UIP_CONF_UDP_CHECKSUMS					1

/* 
 * Enable TCP support.
 */
#define UIP_CONF_TCP							0
/*
 * Enable CoAP support.
 */
#define WITH_COAP
/*
 * The 3 different compression methods supported by contiki. They must be defined 
 * somehow so they can be used in sicslowpan.c.
 * SICSLOWPAN_CONF_COMPRESSION_IPV6 means no compression, only the dispatch byte is
 * prepended and then the IPv6 packet is sent inline.
 *
 * #define SICSLOWPAN_COMPRESSION_IPV6        0
 * #define SICSLOWPAN_COMPRESSION_HC1         1
 * #define SICSLOWPAN_COMPRESSION_HC06        2
 */

/*
 * Now we define which one from the above compression methods we are going to use.
 */
#define SICSLOWPAN_CONF_COMPRESSION             SICSLOWPAN_COMPRESSION_HC06

/*
 * We define the number of context to used in 6LoWPAN IPHC
 */
#define SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS 2

/* Fragmentation support */
#define SICSLOWPAN_CONF_FRAG                    0

#define UIP_CONF_ROUTER			0
#define UIP_FIXEDADDR			0

/*
 * The uip buffer size. As the link layer is IEEE 802.15.4, the largest packet
 * that can be sent throug this link is 128 bytes long. Considering that we do 
 * not support 6lowpan fragmentation, the 802.15.4 header is not placed in the 
 * uip_buf and that the minimum 802.15.4 header length Contiki can work with is 
 * 18 bytes long (1-byte length, 8-byte source address, 2-byte broadcast 
 * destination address, 2-byte fcf, 1-byte sequence number, 2-byte panid and 
 * 2-byte CRC, that is 1+8+2+2+1+2+2), 110 bytes is the maximum length an IP 
 * packet coming through this link can have.
 * NOTE that even though the incoming packet can have at most 110 bytes, it may 
 * contain a compressed IPv6 payload that, after uncompression may be longer 
 * than 110 bytes. Therefore, we should make sure we never exceed its size.    
 */
 /* 110 longest IEEE802154 payload + 40 IPv6 header + 8 UDP header */
#define UIP_CONF_BUFFER_SIZE    110 + 40 + 8 
//#define UIP_CONF_BUFFER_SIZE 1280
/* Node's MAC address */
#define NODE_BASE_ADDR0	0x00
#define NODE_BASE_ADDR1 0x07
#define NODE_BASE_ADDR2 0x62
#define NODE_BASE_ADDR3 0xff
#define NODE_BASE_ADDR4 0xfe

/* Do we implement 6LoWPAN Context dissemination by means of the 6CO option? */
#define CONF_6LOWPAN_ND_6CO		1
/* Do we implement the 6LoWPAN Authoritative Border Router option? */
#define CONF_6LOWPAN_ND_ABRO		0

/* In units of 60 seconds, 1 minute (for debug purposes only!)*/
#define UIP_CONF_ND6_REGISTRATION_LIFETIME 1

#endif /* CONTIKI_CONF_H */
