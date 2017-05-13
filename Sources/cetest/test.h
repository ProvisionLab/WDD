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
	bool EmptyFile( const tstring& Path );
	bool WriteAndCheckRead( const tstring& Path, bool Included );
	bool Restore( const tstring& Destination, const tstring& Path, const tstring& Dir, bool To );
	bool GetContent( const tstring& Path, tstring& Content );

	CSettings _Settings;
};
