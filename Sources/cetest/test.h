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
	bool StartCeuser( bool cetest );

private:
	bool CreateEmptyFile( const tstring& Path );
	bool WriteAndCheckContent( const tstring& Path, bool Included, bool ignore, int& LastIndex );
	bool Restore( const tstring& Destination, const tstring& Path, const tstring& Dir, int Index = 1, bool To = false, bool Deleted = false );
	bool GetContent( const tstring& Path, tstring& Content );
    bool WriteContent( const tstring& Path, const tstring& Content );
    bool DeleteAndCheckContent( const tstring& Destination, const tstring& Path, int Index );
    bool CleanUserApp();

	CSettings _Settings;
};
