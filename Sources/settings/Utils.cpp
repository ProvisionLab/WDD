#include "usercommon.h"
#include "Utils.h"

namespace Utils
{

tstring MapToDestination( const tstring& Destination, const tstring& Path )
{
    tstring strMapped = Path;

    size_t pos = strMapped.find( _T(':') );
    if( pos == tstring::npos )
    {
        ERROR_PRINT( _T("ERROR: MapToDestination failed: Disk not found in the path %s\n"), Path.c_str() );
        return false;
    }

    strMapped = strMapped.substr( 0, pos ) + strMapped.substr( pos + 1 );
	strMapped = Destination + _T("\\") + strMapped;

    return strMapped;
}

tstring MapToOriginal( const tstring& Destination, const tstring& Path )
{
    tstring strMapped = Path.substr( Destination.size() + 1 );

    size_t pos = strMapped.find( _T('\\') );
    if( pos == tstring::npos || pos != 1 )
    {
        ERROR_PRINT( _T("ERROR: MapToOriginal failed: Disk not found in the path %s\n"), Path.c_str() );
        return false;
    }

    strMapped = strMapped.substr( 0, pos ) + _T(":\\") + strMapped.substr( pos + 1 );

    return strMapped;
}

/*File indexes
    a.txt     -> a.txt.N
    a.txt.1   -> a.txt.1.N
    Directories
    1\1.txt   -> 1\N\1.txt
    1\1\1.txt -> 1\1\N\1.txt
*/
bool GetLastIndex( const tstring& MappedPathNoIndex, int& Index )
{
    Index = 0;

    CPathDetails pd;
    if( ! pd.Parse( false, MappedPathNoIndex ) )
    {
        ERROR_PRINT( _T("ERROR: Failed to parse %s\n"), MappedPathNoIndex.c_str() );
        return false;
    }

    if( DoesDirectoryExists( pd.Directory ) )
    {
        WIN32_FIND_DATA ffd;
        HANDLE hFind = INVALID_HANDLE_VALUE;
        hFind = ::FindFirstFile( (pd.Directory + _T("\\*")).c_str(), &ffd );
        if( hFind == INVALID_HANDLE_VALUE )
        {
            ERROR_PRINT( _T("ERROR: FindFirstFile failed %s\n"), pd.Directory.c_str() );
            return false;
        }

        do
        {
            if( ! (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
            {
				tstring strName = ffd.cFileName;
                if( strName.substr( 0, pd.Name.size() + 1 ) == pd.Name + _T(".") )
                {
                    TCHAR* point = _tcsrchr( ffd.cFileName, _T('.') );
                    if( point )
                    {
                        tstring strIndex = point + 1;
                        if( strIndex.size() )
                        {
                            int iIndex = _tstoi( strIndex.c_str() );
                            if( iIndex > Index )
                                Index = iIndex;
                        }
                    }
                }
            }
        } while( ::FindNextFile( hFind, &ffd ) );

        ::FindClose( hFind );
    }

    return true;
}

tstring RemoveEndingSlash( const tstring& Dir )
{
    tstring ret = Dir;
    if( ret[ret.size()-1] == _T('\\') )
        ret.substr( 0, ret.size()-1 );

    return ret;
}

tstring ToLower( const tstring& str )
{
    tstring data = str;
    std::transform( data.begin(), data.end(), data.begin(), ::tolower ) ;

    return data;
}

bool DoesDirectoryExists( const tstring& Directory )
{
    return ::PathFileExists( Directory.c_str() ) != 0;
}

bool CreateDirectory( const tstring& Directory )
{
    size_t pos = 0;
    
    bool bExist = false ;
    while( ! bExist )
    {
        pos = Directory.find( _T('\\'), pos );

        tstring strSubDirectory;
        if( pos == -1 )
        {
            strSubDirectory = Directory;
            bExist = true;
        }
        else
            strSubDirectory = Directory.substr( 0, pos + 1 );

        if( ! DoesDirectoryExists( strSubDirectory ) )
        {
            BOOL ret = ::CreateDirectory( strSubDirectory.c_str(), NULL ) ;
            if( ! ret )
            {
                if( ::GetLastError() != ERROR_ALREADY_EXISTS )
                {
                    ERROR_PRINT( _T("ERROR: CreateDirectory %s failed, error=0x%X\n"), strSubDirectory.c_str(), ::GetLastError() );
                    return false;
                }
            }
        }

        pos ++;
    }

    return true;
}

bool RemoveDirectory( const tstring& Directory )
{
	TCHAR Name[MAX_PATH];
	ZeroMemory( Name, sizeof(Name) );
	StrCatW( Name, Directory.c_str() );

	SHFILEOPSTRUCT SHFileOp = {0};
	ZeroMemory( &SHFileOp, sizeof(SHFILEOPSTRUCT) );
    SHFileOp.hwnd = NULL;
    SHFileOp.wFunc = FO_DELETE;
    SHFileOp.pFrom = Name;
    SHFileOp.pTo = NULL;
    SHFileOp.fFlags = FOF_NOERRORUI | FOF_NOCONFIRMATION;
		
	int ret = ::SHFileOperationW( &SHFileOp );
	if( ret != 0 )
	{
		ERROR_PRINT( _T("ERROR: RemoveDirectory %s failed, status=%s\n"), Directory.c_str(), Utils::GetErrorString( ret ).c_str() );
		return false;
	}

	return true;
}

tstring GetLastErrorString()
{
	return GetErrorString( ::GetLastError() );
}

tstring GetErrorString( DWORD err )
{
    tstring ret;
    if( err == ERROR_ACCESS_DENIED )
        ret = _T("ERROR_ACCESS_DENIED");
    else if( err == ERROR_INVALID_HANDLE )
        ret = _T("ERROR_INVALID_HANDLE");
    else if( err == ERROR_INVALID_PARAMETER )
        ret = _T("ERROR_INVALID_PARAMETER");
    else if( err == ERROR_INVALID_FUNCTION )
        ret = _T("ERROR_INVALID_FUNCTION");
    else if( err == ERROR_FILE_EXISTS )
        ret = _T("ERROR_FILE_EXISTS");
    else if( err == ERROR_FILE_NOT_FOUND )
        ret = _T("ERROR_FILE_NOT_FOUND");
    else
    {
        std::wstringstream oss;
        oss << _T(" error=") << ::GetLastError();
        ret = oss.str();
    }

    return ret;
}

CPathDetails::CPathDetails()
	: Mapped(false)
{
}

bool CPathDetails::Parse( bool aMapped, const tstring& Path )
{
	Mapped = aMapped;
    Directory = _T("");

    size_t pos = Path.rfind( _T('\\') ) ;
    if( pos == tstring::npos )
        return false;

    Directory = Path.substr( 0, pos );
    Name = Path.substr( pos + 1 );

	if( aMapped )
	{
	    pos = Name.rfind( _T('.') ) ;
		if( pos == tstring::npos )
			return false;

		Index = Name.substr( pos + 1 );
		Name = Name.substr( 0, pos );
		pos = Name.rfind( pos-1, _T('.') ) ;
		if( pos != tstring::npos )
			Extension = Name.substr( pos + 1 );
	}
	else
	{
	    pos = Name.rfind( _T('.') ) ;
		if( pos != tstring::npos )
			Extension = Name.substr( pos + 1 );
	}

    return true;
}

bool ExecuteProcess( CHAR* CommadLineA, BOOL Wait )
{
	STARTUPINFOA si;
    PROCESS_INFORMATION pi;
	ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

	if( ! ::CreateProcessA( NULL, CommadLineA, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ) )
	{
		ERROR_PRINT( _T("ERROR: ExecuteProcess(%S) failed. status=%s\n"), CommadLineA, Utils::GetLastErrorString().c_str() );
		return false;
	}

	if( Wait )
		::WaitForSingleObject( pi.hProcess, INFINITE );

    // Close process and thread handles. 
    ::CloseHandle( pi.hProcess );
    ::CloseHandle( pi.hThread );

	return true;
}

}
