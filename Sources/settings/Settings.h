#pragma once

#include "INIReader.h"

class CSettings
{
public:
    tstring Destination;
    std::vector<tstring> IncludedFiles;
    std::vector<tstring> ExcludedFiles;
    std::vector<tstring> IncludedDirectories;
    std::vector<tstring> ExcludedDirectories;

	CSettings();
	virtual ~CSettings() {};
	bool Init( const std::wstring& IniPath, tstring& error );

private:
	bool ParseIni( INIReader& reader, const tstring& IniPath, tstring& error );
};
