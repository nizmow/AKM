#if !defined(_STDINT_H_)
#define _STDINT_H_

#if defined(__PUREC__)
typedef long int32_t;
typedef unsigned long uint32_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned char uint8_t;
typedef char int8_t;
#else
#error "compiler not yet supported
#endif

#endif
