/*
 * buffer.h
 *
 *  Created on: Oct 19, 2010
 *      Author: dogan
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#include "contiki.h"
#include <stdlib.h>
#include <string.h>

#ifdef CONF_MAX_BUFFER_SIZE
#define MAX_BUFFER_SIZE CONF_MAX_BUFFER_SIZE
#else
#define MAX_BUFFER_SIZE 255
#endif /* CONF_MAX_BUFFER_SIZE */

#ifdef CONF_DATA_ALIGNMENT
#define DATA_ALIGNMENT CONF_DATA_ALIGNMENT
#else
#define DATA_ALIGNMENT 0
#endif /* CONF_DATA_ALIGNMENT */

void delete_buffer(void);

#if (MAX_BUFFER_SIZE <= 255)
u8_t* init_buffer(u8_t size);
#else
u8_t* init_buffer(u16_t size);
#endif/* (MAX_BUFFER_SIZE <= 255) */

#if (MAX_BUFFER_SIZE <= 255)
u8_t* allocate_buffer(u8_t size);
#else
u8_t* allocate_buffer(u16_t size);
#endif /* (MAX_BUFFER_SIZE <= 255) */

#if (MAX_BUFFER_SIZE <= 255)
u8_t* copy_to_buffer(void* data, u8_t len);
#else
u8_t* copy_to_buffer(void* data, u16_t len);
#endif /* (MAX_BUFFER_SIZE <= 255) */

u8_t* copy_text_to_buffer(char* text);
#endif /* BUFFER_H_ */
