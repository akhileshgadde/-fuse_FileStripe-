//#if HAVE_QUAD_SUPPORT
//# if ! HAVE_STRTOLL && HAVE_LONG_LONG
long long strtoll(const char *, char **, int);
//#  if ! defined(LLONG_MIN)
#define LLONG_MIN	(-0x7fffffffffffffffL-1)
//#  endif
//#  if ! defined(LLONG_MAX)
#define LLONG_MAX	(0x7fffffffffffffffL)
//#  endif
//# endif
//#else	/* ! HAVE_QUAD_SUPPORT */
//# define NO_LONG_LONG	1
//#endif	/* ! HAVE_QUAD_SUPPORT */
