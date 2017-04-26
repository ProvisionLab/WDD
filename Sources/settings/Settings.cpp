#include "usercommon.h"
#include "Settings.h"
#include "Utils.h"

CSettings::CSettings()
{
}

bool CSettings::Init( const std::wstring& IniPath )
{
    INIReader reader( IniPath );

	if( reader.ParseError() < 0 )
    {
        DEBUG_PRINT( _T("Can't load '%s'\n"), IniPath.c_str() );
        return false;
    }

	return ParseIni( reader, IniPath );
}

bool CSettings::ParseIni( INIReader& reader, const tstring& IniPath )
{
    Destination = Utils::ToLower( reader.Get( _T("Destination"), _T("Path"), _T("")) );

    if( Destination.length() == 0 )
    {
        DEBUG_PRINT( _T("[Destination]Path not found in '%s'\n"), IniPath.c_str() );
        return false;
    }

    Destination = Utils::RemoveEndingSlash( Destination );

    if( ! Utils::CreateDirectories( Destination.c_str() ) )
        return false;

    INFO_PRINT( _T("[Settings] Backup directory: %s\n"), Destination.c_str() );

    int iCount = reader.GetInteger( _T("IncludeFile"), _T("Count"), 0 );
    for( int i=1; i<=iCount; i++ )
    {
        std::wstringstream oss;
        oss << _T("File" ) << i;
        tstring strName;
        tstring strFile = Utils::ToLower( reader.Get( _T("IncludeFile"), oss.str(), _T("") ) );

        if( strFile.length() == 0 )
        {
            DEBUG_PRINT( _T("[IncludeFile] '%s' not found in '%s'\n"), oss.str().c_str(), IniPath.c_str() );
            return false;
        }

        IncludedFiles.push_back( strFile );
    }

    iCount = reader.GetInteger( _T("IncludeDirectory"), _T("Count"), 0 );
    for( int i=1; i<=iCount; i++ )
    {
        std::wstringstream oss;
        oss << _T("Directory" ) << i;
        tstring strName;
        tstring strDirectory = Utils::ToLower( reader.Get( _T("IncludeDirectory"), oss.str(), _T("") ) );

        if( strDirectory.length() == 0 )
        {
            DEBUG_PRINT( _T("[IncludeDirectory] '%s' not found in '%s'\n"), oss.str().c_str(), IniPath.c_str() );
            return false;
        }

        IncludedDirectories.push_back( Utils::RemoveEndingSlash( strDirectory ) );
    }

    iCount = reader.GetInteger( _T("ExcludeFile"), _T("Count"), 0 );
    for( int i=1; i<=iCount; i++ )
    {
        std::wstringstream oss;
        oss << _T("File" ) << i;
        tstring strName;
        tstring strFile = Utils::ToLower( reader.Get( _T("ExcludeFile"), oss.str(), _T("") ) );

        if( strFile.length() == 0 )
        {
            DEBUG_PRINT( _T("[ExcludeFile] '%s' not found in '%s'\n"), oss.str().c_str(), IniPath.c_str() );
            return false;
        }

        ExcludedFiles.push_back( strFile );
    }

    iCount = reader.GetInteger( _T("ExcludeDirectory"), _T("Count"), 0 );
    for( int i=1; i<=iCount; i++ )
    {
        std::wstringstream oss;
        oss << _T("Directory" ) << i;
        tstring strName;
        tstring strDirectory = Utils::ToLower( reader.Get( _T("ExcludeDirectory"), oss.str(), _T("") ) );

        if( strDirectory.length() == 0 )
        {
            DEBUG_PRINT( _T("[ExcludeDirectory] '%s' not found in '%s'\n"), oss.str().c_str(), IniPath.c_str() );
            return false;
        }

        ExcludedDirectories.push_back( Utils::RemoveEndingSlash( strDirectory ) );
    }

    return true;
}
