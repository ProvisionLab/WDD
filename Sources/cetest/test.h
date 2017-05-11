#pragma once

#include "usercommon.h"
#include "Settings.h"

class CTest
{
public:
    bool Run( const tstring& IniPath );
	bool StopDriver();
	bool StopCeuser();
	bool StartDriver();
	bool StartCeuser();

private:
	bool CheckFile( const tstring& Path, bool Included );

	CSettings _Settings;
};
