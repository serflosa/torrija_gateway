#ifndef TYPES_H_
#define TYPES_H_

/***********************************************************************************
 * TYPEDEFS
 */
/*
typedef signed   char   int8_t;
typedef unsigned char   uint8_t;

typedef signed   short  int16_t;
typedef unsigned short  uint16_t;

typedef signed   long   int32_t;
typedef unsigned long   uint32_t;
*/
#endif /*TYPES_H_*/

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL (void *)0
#endif

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILED
#define FAILED  1
#endif

/***********************************************************************************
* MACROS
*/

/* uint16_t processing */
#define HI_UINT16(a) (((uint16_t)(a) >> 8) & 0xFF)
#define LO_UINT16(a) ((uint16_t)(a) & 0xFF)
/* uint32_t processing */
#define HI_UINT32(a) ((uint16_t) (((uint32_t)(a)) >> 16))
#define LO_UINT32(a) ((uint16_t) ((uint32_t)(a)))

