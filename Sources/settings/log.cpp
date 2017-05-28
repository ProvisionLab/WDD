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
	print(level, format, argl) ; \
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

void CLog::print( int level, const TCHAR* format, va_list argl )
{
	if( ! format )
        return;

	if( level >= _mask )
	{
        EnterCriticalSection( &_guard );

		tstring buffer = CThreadFormatter<>::format( level, format, argl ) ;

        if( _bFileOutput )
    		_FileWriter.write( level, buffer ) ;

		if( _bDebugOutput )
			::OutputDebugString( buffer.c_str() ) ;

        if( _bConsoleOutput )
            _tprintf( buffer.c_str() );

        LeaveCriticalSection( &_guard );
	}
}

bool CLog::Init( const tstring& Name )
{
	_bFileOutput = _FileWriter.open( Name.c_str() ) ;

    return true;
}

bool CFileWriter::open( const TCHAR* file, options_t openMode )
{
	if( ! openFile( file, openMode ) )
		return false ;

	_file = file ;
	return true ;
}

const tstring CFileWriter::getFile() const
{
	return _file ;
}

CFileWriter::CFileWriter()
	: _stream(NULL)
	, _length(0) {}

CFileWriter::~CFileWriter()
{
	close() ;
}

void CFileWriter::write( int level, const tstring& buffer )
{
	if( _stream != NULL )
	{
		::fwrite( buffer.c_str(), sizeof(TCHAR), buffer.length(), _stream ) ;
		::fflush( _stream ) ;
		_length += (int)buffer.length() ;
	}
}

bool CFileWriter::openFile( const TCHAR* file, options_t openMode )
{
	close() ;
	_length = 0 ;
	::_tfopen_s( &_stream, file, openMode == opRewrite ? _T("wt") : _T("at") ) ;
	if( ! _stream )
		return false ;

	if( openMode == opAppend )
	{
		int prev = ftell( _stream ) ;
		fseek( _stream, 0L, SEEK_END ) ;
		int sz = ftell( _stream ) ;
		fseek( _stream, prev, SEEK_SET ) ;

		_length = sz ;
	}
	return _stream != NULL ;
}

void CFileWriter::close()
{
	if( _stream )
	{
		::fclose( _stream ) ;
		_stream = NULL ;
	}
}
