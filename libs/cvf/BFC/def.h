
#ifndef _FF_BFC_DEF_H
#define _FF_BFC_DEF_H


#include"ffdef.h"

#ifdef BFCS_STATIC
	#define _BFCS_API
#endif

#ifndef _BFCS_API

	#if defined(_WIN32)

		#ifdef BFCS_EXPORTS
		#define _BFCS_API __declspec(dllexport)
		#else
		#define _BFCS_API __declspec(dllimport)
		#endif

	#elif defined(__GNUC__) && __GNUC__ >= 4

		#ifdef BFCS_EXPORTS
		#define _BFCS_API __attribute__ ((visibility ("default")))
		#else
		#define _BFCS_API 
		#endif

	#elif
		#define _BFCS_API
	#endif
#endif



#endif

