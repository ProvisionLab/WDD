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
	bool CreateEmptyFile( const tstring& Path );
	bool WriteAndCheckContent( const tstring& Path, bool Included, int& LastIndex );
	bool Restore( const tstring& Destination, const tstring& Path, const tstring& Dir, int Index = 1, bool To = false, bool Deleted = false );
	bool GetContent( const tstring& Path, tstring& Content );
    bool DeleteAndCheckContent( const tstring& Destination, const tstring& Path, int Index );

	CSettings _Settings;
};
