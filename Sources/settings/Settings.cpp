#include "usercommon.h"
#include "Settings.h"
#include "Utils.h"

CSettings::CSettings()
{
}

bool CSettings::Init( const std::wstring& IniPath, tstring& error )
{
    INIReader reader( IniPath );

	if( reader.ParseError() < 0 )
    {
        error = _T("Can't find/load ini file '") + IniPath + _T("'");
        ERROR_PRINT( ( tstring(_T("ERROR: Settings: ")) + error + _T("\n")).c_str() );
        return false;
    }

	return ParseIni( reader, IniPath, error );
}

bool CSettings::ParseIni( INIReader& reader, const tstring& IniPath, tstring& error )
{
    Destination = Utils::ToLower( reader.Get( _T("Destination"), _T("Path"), _T("")) );

    if( Destination.length() == 0 )
    {
        error = _T("[Destination] Path not found in '") + IniPath + _T("'");
        ERROR_PRINT( ( tstring(_T("ERROR: Settings: ")) + error + _T("\n")).c_str() );
        return false;
    }

    Destination = Utils::RemoveEndingSlash( Destination );

    int iCount = reader.GetInteger( _T("IncludeFile"), _T("Count"), 0 );
    for( int i=1; i<=iCount; i++ )
    {
        std::wstringstream oss;
        oss << _T("File" ) << i;
        tstring strName;
        tstring strFile = Utils::ToLower( reader.Get( _T("IncludeFile"), oss.str(), _T("") ) );

        if( strFile.length() == 0 )
        {
            error = _T("[IncludeFile] '") + oss.str() + _T("' not found in '") + IniPath + _T("'");
            ERROR_PRINT( ( tstring(_T("ERROR: Settings: ")) + error + _T("\n")).c_str() );
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
            error = _T("[IncludeDirectory] '") + oss.str() + _T("' not found in '") + IniPath + _T("'");
            ERROR_PRINT( ( tstring(_T("ERROR: Settings: ")) + error + _T("\n")).c_str() );
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
            error = _T("[ExcludeFile] '") + oss.str() + _T("' not found in '") + IniPath + _T("'");
            ERROR_PRINT( ( tstring(_T("ERROR: Settings: ")) + error + _T("\n")).c_str() );
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
            error = _T("[ExcludeDirectory] '") + oss.str() + _T("' not found in '") + IniPath + _T("'");
            ERROR_PRINT( ( tstring(_T("ERROR: Settings: ")) + error + _T("\n")).c_str() );
            return false;
        }

        ExcludedDirectories.push_back( Utils::RemoveEndingSlash( strDirectory ) );
    }

    iCount = reader.GetInteger( _T("ExcludeExtension"), _T("Count"), 0 );
    for( int i=1; i<=iCount; i++ )
    {
        std::wstringstream oss;
        oss << _T("Extension" ) << i;
        tstring strName;
        tstring strExtension = Utils::ToLower( reader.Get( _T("ExcludeExtension"), oss.str(), _T("") ) );

        if( strExtension.length() == 0 )
        {
            error = _T("[ExcludeExtension] '") + oss.str() + _T("' not found in '") + IniPath + _T("'");
            ERROR_PRINT( ( tstring(_T("ERROR: Settings: ")) + error + _T("\n")).c_str() );
            return false;
        }

        ExcludedExtensions.push_back( Utils::RemoveEndingSlash( strExtension ) );
    }

    return true;
}
