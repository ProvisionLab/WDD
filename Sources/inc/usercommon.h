#pragma once

#include <windows.h>
#include <Shlwapi.h>
#include <Shellapi.h>
#include <fltuser.h>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <ctime>

#include <tchar.h>
#ifdef UNICODE
typedef std::wstring tstring;
#else
typedef std::wstring tstring;
#endif

#pragma warning( disable : 4242 )  
#include <algorithm>

#define ERROR_PRINT _tprintf
#define INFO_PRINT  _tprintf
#define DEBUG_PRINT 
#define TMP_PRINT   _tprintf
