#if !defined(_GLIBCXX_USE_NANOSLEEP) && defined(__GNUC__) && (__GNUC__*1000+__GNUC_MINOR__)<=4007
#define HACK_GCCBUG_52680
#define _GLIBCXX_USE_NANOSLEEP
#endif
#include <thread>
#ifdef HACK_GCCBUG_52680
#undef HACK_GCCBUG_52680
#undef _GLIBCXX_USE_NANOSLEEP
#endif
