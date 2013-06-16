#pragma once

#define VERSION "6.8.6"

//#define __SYN
#define __LOOP

//#define NOTICE_PREFIX "[Tom's Antihack] "
//#define NOTICE_PREFIX "[aProtect] "
//#define NOTICE_PREFIX "[Bubble Guard] " // Bubble
//#define NOTICE_PREFIX "[Beasty Guard] " // Imaginarum
//#define NOTICE_PREFIX "[ShineGuard] " // Light flyff
//#define NOTICE_PREFIX "" // Inposterum

#define MAX_CONNS 5000

#include "resource.h"

#include "targetver.h"


#include <winsock.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <windows.h>
#include <vector>
#include <time.h>

#pragma warning(disable:4996) // May be unsafe etc
#include "md5.h"

//#include <iostream>
//#include <string.h>
