#pragma once

#include <sys/types.h>
#include <sys/socket.h>

#ifdef FIBER_CPP_LIB
# ifndef FIBER_CPP_API
#  define FIBER_CPP_API
# endif
#elif defined(FIBER_CPP_DLL) // || defined(_WINDLL)
# if defined(FIBER_CPP_EXPORTS) || defined(fibercpp_EXPORTS)
#  ifndef FIBER_CPP_API
#   define FIBER_CPP_API __declspec(dllexport)
#  endif
# elif !defined(FIBER_CPP_API)
#  define FIBER_CPP_API __declspec(dllimport)
# endif
#elif !defined(FIBER_CPP_API)
# define FIBER_CPP_API
#endif
