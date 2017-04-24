#pragma once

#include <windows.h>
#include <Shlwapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <winioctl.h>
#include <string.h>
#include <crtdbg.h>
#include <assert.h>
#include <fltuser.h>
#include <dontuse.h>
#include <iostream>
#include <sstream>
#include <tchar.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

#ifdef UNICODE
typedef std::wstring tstring;
#else
typedef std::wstring tstring;
#endif
