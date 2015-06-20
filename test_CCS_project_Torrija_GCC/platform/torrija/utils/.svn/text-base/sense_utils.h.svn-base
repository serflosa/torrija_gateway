#ifndef _SENSE_UTILS_H
#define _SENSE_UTILS_H

#include <inttypes.h>

typedef struct {
    volatile uint8_t *pData;
    volatile uint16_t nBytes;
    volatile uint16_t iHead;
    volatile uint16_t iTail;
    volatile uint16_t len;    
} ringBuf_t;

/***********************************************************************************
* GLOBAL FUNCTIONS
*/
void  bufInit(ringBuf_t *pBuf, uint8_t *buffer, uint16_t len);
uint16_t bufPut(ringBuf_t *pBuf, const uint8_t *pData, uint16_t n);
uint16_t bufGet(ringBuf_t *pBuf, uint8_t *pData, uint16_t n);
uint16_t bufPeek(ringBuf_t *pBuf, uint8_t *pData, uint16_t nBytes);
uint16_t bufNumBytes(ringBuf_t *pBuf);
void bufFlush(ringBuf_t *pBuf);


#endif
