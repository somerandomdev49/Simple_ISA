#include <string.h>
#include <time.h>
typedef unsigned char u;
typedef unsigned short U;
#if defined(USE_UNSIGNED_INT)
typedef unsigned int UU;
typedef signed int SUU;
#else
typedef unsigned long UU;
typedef long SUU;
#endif
static u R=0, M[(((UU)1)<<24)], PTEMP[0x100], *SEGMENT;
static UU SEGMENT_PAGES=0, EMULATE_DEPTH=0;
