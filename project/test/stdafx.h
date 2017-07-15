
#include "enet/enet.h"

#ifdef _MSC_VER // Microsoft Visual Studio
#   include <WinSock2.h> // htonl()
#   include <Windows.h>  // Sleep()
#else
#   include <arpa/inet.h> // htonl()
#   include <time.h>      // nanosleep()
#endif
