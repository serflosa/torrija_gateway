/*
 * buffer.h
 *
 *  Created on: Oct 19, 2010
 *      Author: dogan
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#include "contiki.h"

void delete_buffer(void);
u8_t* init_buffer(u16_t size);
u8_t* allocate_buffer(u16_t size);
u8_t* copy_to_buffer(void* data, u16_t len);
u8_t* copy_text_to_buffer(char* text);

#endif /* BUFFER_H_ */
