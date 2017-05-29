#include <fcntl.h>
#include <io.h> 
#include "usercommon.h"
#include "log.h"

CLog& CLog::instance()
{
	static CLog inst ;
	return inst ;
}

#define DO_PRINT(level, format) { \
	va_list argl; \
	va_start(argl, format) ; \
	Print(level, format, argl) ; \
	va_end(argl) ; }

CLog::CLog()
	: _mask(tlDebug)
	, _bDebugOutput(true)
	, _bFileOutput(true)
    , _bConsoleOutput(true)
{
    InitializeCriticalSection( &_guard );
}

void CLog::debug( const TCHAR* format, ... )
{
	DO_PRINT( tlDebug, format ) ;
}

void CLog::info( const TCHAR* format, ... )
{
	DO_PRINT( tlInfo, format ) ;
}

void CLog::warning( const TCHAR* format, ... )
{
	DO_PRINT( tlWarning, format ) ;
}

void CLog::error( const TCHAR* format, ... )
{
	DO_PRINT( tlError, format ) ;
}

void CLog::debug( const tstring& format, ... )
{
	const TCHAR* f = format.c_str() ;
	DO_PRINT( tlDebug, f ) ;
}

void CLog::info( const tstring& format, ... )
{
	const TCHAR* f = format.c_str() ;
	DO_PRINT( tlInfo, f ) ;
}

void CLog::warning( const tstring& format, ... )
{
	const TCHAR* f = format.c_str() ;
	DO_PRINT( tlWarning, f ) ;
}

void CLog::error( const tstring& format, ... )
{
	const TCHAR* f = format.c_str() ;
	DO_PRINT( tlError, f ) ;
}

void CLog::ClearPrefix()
{
	CThreadFormatter<>::setOptions( opNone ) ;
}

void CLog::Print( int level, const TCHAR* format, va_list argl )
{
	if( ! format )
        return;

	if( level >= _mask )
	{
        EnterCriticalSection( &_guard );

		tstring buffer = CThreadFormatter<>::format( level, format, argl ) ;

        if( _bFileOutput )
    		_FileWriter.Write( level, buffer ) ;

		if( _bDebugOutput )
			::OutputDebugString( buffer.c_str() ) ;

        if( _bConsoleOutput )
            _tprintf( buffer.c_str() );

        LeaveCriticalSection( &_guard );
	}
}

bool CLog::Init( const tstring& Name )
{
	_bFileOutput = _FileWriter.Open( Name.c_str() ) ;

    return true;
}

bool CFileWriter::Open( const TCHAR* file, options_t openMode )
{
	if( ! OpenFile( file, openMode ) )
		return false ;

	_file = file ;
	return true ;
}

const tstring CFileWriter::GetFile() const
{
	return _file ;
}

CFileWriter::CFileWriter()
	: _handle(NULL)
	, _length(0) {}

CFileWriter::~CFileWriter()
{
	Close() ;
}

void CFileWriter::Write( int level, const tstring& buffer )
{
	if( _handle != NULL )
	{
        std::string ansi( buffer.size(), _T('\0') );
        size_t convertedChars = 0;
        wcstombs_s( &convertedChars, &ansi[0], buffer.size() + 1, buffer.c_str(), _TRUNCATE );  
		int ret = ::_write( _handle, &ansi[0], buffer.length() ) ;

		_length += (int)buffer.length() ;
	}
}

bool CFileWriter::OpenFile( const TCHAR* file, options_t openMode )
{
	Close() ;
	_length = 0 ;
    int iOpenFlag = _O_CREAT | _O_TEXT | _O_RDWR;
    if( openMode == opAppend )
        iOpenFlag |= _O_APPEND;
    else
        iOpenFlag |= _O_WRONLY;

    errno_t err = ::_tsopen_s( &_handle, file, iOpenFlag, _SH_DENYWR, _S_IREAD | _S_IWRITE );
	if( err != 0 )
		return false ;

	return _handle != -1 ;
}

void CFileWriter::Close()
{
	if( _handle )
	{
		_close( _handle ) ;
		_handle = NULL ;
	}
}
