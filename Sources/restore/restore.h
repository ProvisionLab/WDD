#pragma once

#include "usercommon.h"

class CRestore
{
public:
    bool Run( const tstring& IniPath );

private:
    bool IsRunning();
};
