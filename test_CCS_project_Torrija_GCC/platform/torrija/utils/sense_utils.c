#include "sense_utils.h"

/***********************************************************************************
* @fn      bufInit
*
* @brief   Initialise a ringbuffer. The buffer must be allocated by the
*          application.
*
* @param   pBuf - pointer to the ringbuffer
* 		   buffer - the actual buffer where data is to be stored
* 		   len	- buffer length
*
* @return  none
*/
void bufInit(ringBuf_t *pBuf, uint8_t *buffer, uint16_t len)
{        
    pBuf->nBytes = 0;
    pBuf->iHead = 0;
    pBuf->iTail = 0;
    pBuf->pData = buffer;
    pBuf->len = len;
}

/***********************************************************************************
* @fn      bufPut
*
* @brief   Add bytes to the buffer.
*
* @param   pBuf - pointer to the ringbuffer
*          pData - pointer to data to be appended to the buffer
*          nBytes - number of bytes
*
* @return  Number of bytes copied to the buffer
*/
uint16_t bufPut(ringBuf_t *pBuf, const uint8_t *pData, uint16_t nBytes)
{
	uint16_t i;
    
	if (pBuf->nBytes + nBytes < pBuf->len) {
		i = 0;
	    while(i < nBytes) {
	    	pBuf->pData[pBuf->iTail] = pData[i];
	    	pBuf->iTail++;
	    	if (pBuf->iTail == pBuf->len) {
	    		pBuf->iTail= 0;
	    	}
	    	i++;
	    }
	    pBuf->nBytes += i;
    } else {
    	i = 0;
    }
    return i;
}


/***********************************************************************************
* @fn      bufGet
*
* @brief   Extract bytes from the buffer.
*
* @param   pBuf   - pointer to the ringbuffer
*          pData  - pointer to data to be extracted
*          nBytes - number of bytes
*
* @return  Bytes actually returned
*/
uint16_t bufGet(ringBuf_t *pBuf, uint8_t *pData, uint16_t nBytes)
{
    uint16_t i;
    
    i = 0;
    while(i < nBytes && i < pBuf->nBytes) {
        pData[i] = pBuf->pData[pBuf->iHead];
        pBuf->iHead++;
        if (pBuf->iHead == pBuf->len)
            pBuf->iHead = 0;
        i++;
    }
    pBuf->nBytes-= i;

    return i;
}


/***********************************************************************************
* @fn      bufPeek
*
* @brief   Read bytes from the buffer but leave them in the queue.
*
* @param   pBuf   - pointer to the ringbuffer
*          pData  - pointer to data to be extracted
*          nBytes - number of bytes
*
* @return  Bytes actually returned
*/
uint16_t bufPeek(ringBuf_t *pBuf, uint8_t *pData, uint16_t nBytes)
{
    uint16_t i,j;

    i = 0;
    j = pBuf->iHead;
    while(i < nBytes && i < pBuf->nBytes) {
        pData[i]= pBuf->pData[j];
        j++;
        if (j == pBuf->len)
            j = 0;
        i++;
    }

    return i;
}

/***********************************************************************************
* @fn      bufNumBytes
*
* @brief   Return the byte count for the ring buffer.
*
* @param   pBuf- pointer to the buffer
*
* @return  Number of bytes present.
*/
uint16_t bufNumBytes(ringBuf_t *pBuf)
{
    return pBuf->nBytes;
}

/***********************************************************************************
* @fn      bufFlush
*
* @brief   Flush the buffer.
*
* @param   pBuf- pointer to the buffer
*/
void bufFlush(ringBuf_t *pBuf)
{
	bufInit(pBuf, pBuf->pData, pBuf->len);	
}
