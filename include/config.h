#pragma once


#ifdef INT32
#define IND_TYP int32_t
#else
#define IND_TYP int64_t
#endif

#ifdef FLT64
#define FLT_TYP double
#else
#define FLT_TYP float
#endif
