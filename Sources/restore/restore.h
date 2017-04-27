#pragma once

#include "usercommon.h"

class CRestore
{
public:
    bool Run( const tstring& IniPath, const tstring& Command, const tstring& Path, bool IsPath, const tstring& RestoreToDir );

private:
    bool IsRunning();
	bool ListFiles( const tstring& Destination, bool All, const tstring& Path, bool IsPath );
	bool Restore( const tstring& Destination, const tstring& Path, const tstring& RestoreToDir = _T("") );
	bool IterateDirectories( const tstring& Destination, const tstring& Directory, const tstring& Name, bool All, bool IsPath, std::vector<tstring>& Files );
};
