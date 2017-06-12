#pragma once

#include "usercommon.h"
#include "Settings.h"

class CRestore
{
public:
    CRestore();
    virtual ~CRestore();
    static bool IsAlreadyRunning();
    bool Init( const tstring& IniPath, tstring& Error );
    bool LockAccess();

	bool ListFiles( bool All, const tstring& Path, bool IsPath, std::vector<tstring>& Files );
	bool Restore( const tstring& Path, tstring& strError, const tstring& RestoreToDir = _T("") );
	bool IterateDirectories( const tstring& Directory, const tstring& Name, bool All, bool IsPath, std::vector<tstring>& Files );

private:
    CSettings _Settings;
    HANDLE _hLockMutex;
	HANDLE _hPort;
};
