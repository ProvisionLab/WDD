#include "usercommon.h"
#include "Settings.h"
#include "Utils.h"

CSettings::CSettings()
    : DeleteAfterDays(30)
    , IgnoreSaveForMinutes(1)
    , NumberOfCopies(10)
    , MaxFileSizeBytes(1048576)
    , MaxBackupSizeMB(102400)
{
}

bool CSettings::Init( const std::wstring& IniPath, tstring& error )
{
    INIReader reader( IniPath );

	if( reader.ParseError() < 0 )
    {
        error = _T("Can't find/load ini file '") + IniPath + _T("'");
        TRACE_ERROR( ( tstring(_T("Settings: ")) + error).c_str() );
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
        TRACE_ERROR( ( tstring(_T("Settings: ")) + error).c_str() );
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
            TRACE_ERROR( ( tstring(_T("Settings: ")) + error).c_str() );
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
            TRACE_ERROR( ( tstring(_T("Settings: ")) + error).c_str() );
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
            TRACE_ERROR( ( tstring(_T("Settings: ")) + error).c_str() );
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
            TRACE_ERROR( ( tstring(_T("Settings: ")) + error).c_str() );
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
            TRACE_ERROR( ( tstring(_T("Settings: ")) + error).c_str() );
            return false;
        }

        ExcludedExtensions.push_back( Utils::RemoveEndingSlash( strExtension ) );
    }

    DeleteAfterDays = reader.GetInteger( _T("Cleanup"), _T("DeleteAfterDays"), 0 );
    if( DeleteAfterDays <= 1 )
        DeleteAfterDays = 1;
    IgnoreSaveForMinutes = reader.GetInteger( _T("Cleanup"), _T("IgnoreSaveForMinutes"), 0 );
    if( IgnoreSaveForMinutes <= 0 )
        IgnoreSaveForMinutes = 0;
    NumberOfCopies = reader.GetInteger( _T("Cleanup"), _T("NumberOfCopies"), 0 );
    if( NumberOfCopies <= 0 )
        NumberOfCopies = 1;

    MaxFileSizeBytes = reader.GetInteger( _T("Cleanup"), _T("MaxFileSizeBytes"), 0 );
    if( MaxFileSizeBytes <= 0 )
        MaxFileSizeBytes = 1048576;

    MaxBackupSizeMB = reader.GetInteger( _T("Cleanup"), _T("MaxBackupSizeBytes"), 0 );
    if( MaxBackupSizeMB <= 0 )
        MaxBackupSizeMB = 102400;

    return true;
}

    tstring GetDestination();
    std::vector<tstring> GetIncludedFiles();
    std::vector<tstring> GetExcludedFiles();
    std::vector<tstring> GetIncludedDirectories();
    std::vector<tstring> GetExcludedDirectories();
    std::vector<tstring> GetExcludedExtensions();
    int GetDeleteAfterDays();
    int GetIgnoreSaveForMinutes();
    int GetNumberOfCopies();
    long GetMaxFileSizeBytes();
    long GetMaxBackupSizeMB();
