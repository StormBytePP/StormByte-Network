#pragma once

#include <StormByte/platform.h>

#ifdef WINDOWS
	#ifdef StormByte_Network_EXPORTS
		#define STORMBYTE_NETWORK_PUBLIC	__declspec(dllexport)
	#else
		#define STORMBYTE_NETWORK_PUBLIC	__declspec(dllimport)
	#endif
	#define STORMBYTE_NETWORK_PRIVATE
#else
	#define STORMBYTE_NETWORK_PUBLIC		__attribute__ ((visibility ("default")))
	#define STORMBYTE_NETWORK_PRIVATE		__attribute__ ((visibility ("hidden")))
#endif
