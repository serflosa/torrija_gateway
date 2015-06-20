/*
 * buffer.c
 *
 * Reduced and static version of buffer.c included in rest-common lib.
 * Maximum
 *
 *  Created on: Oct 19, 2010
 *      Author: Luis Maqueda
 */

#include "rest-common/buffer.h"

u8_t data_buffer[MAX_BUFFER_SIZE];
#if (MAX_BUFFER_SIZE <= 255)
u8_t buffer_size;
u8_t buffer_index;
#else
u16_t buffer_size;
u16_t buffer_index;
#endif /* (MAX_BUFFER_SIZE <= 256) */

void
delete_buffer(void)
{
  buffer_index = 0;
  buffer_size = 0;
}

#if (MAX_BUFFER_SIZE <= 255)
u8_t*
init_buffer(u8_t size)
#else
u8_t*
init_buffer(u16_t size)
#endif/* (MAX_BUFFER_SIZE <= 255) */
{
  if (size > MAX_BUFFER_SIZE) {
    return NULL;
  }
  buffer_size = size;
  buffer_index = 0;
  return data_buffer;
}

#if (MAX_BUFFER_SIZE <= 255)
u8_t*
allocate_buffer(u8_t size)
#else
u8_t*
allocate_buffer(u16_t size)
#endif /* (MAX_BUFFER_SIZE <= 255) */
{
  u8_t* buffer = NULL;

#if DATA_ALIGNMENT
  /*To get rid of alignment problems, always allocate even size*/
  if (size % 2) {
    size++;
  }
#endif

  if (buffer_index + size < buffer_size) {
    buffer = (u8_t*)(data_buffer + buffer_index);
    buffer_index += size;
  }

  return buffer;
}

#if (MAX_BUFFER_SIZE <= 255)
u8_t*
copy_to_buffer(void* data, u8_t len)
#else
u8_t*
copy_to_buffer(void* data, u16_t len)
#endif /* (MAX_BUFFER_SIZE <= 255) */
{
  u8_t* buffer = allocate_buffer(len);
  if (buffer) {
    memcpy(buffer, data, len);
  }

  return buffer;
}

u8_t*
copy_text_to_buffer(char* text)
{
  u8_t* buffer = allocate_buffer(strlen(text) + 1);
  if (buffer) {
    strcpy((char*)buffer, text);
  }

  return buffer;
}
