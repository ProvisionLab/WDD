#pragma once

#include <sys/timeb.h>
#include <numeric>

#ifndef TRACE_DEBUG
 #define TRACE_DEBUG CLog::instance().debug
#endif // TRACE_DEBUG

#ifndef TRACE_INFO
 #define TRACE_INFO CLog::instance().info
#endif // TRACE_INFO

#ifndef TRACE_WARNING
 #define TRACE_WARNING CLog::instance().warning
#endif // TRACE_WARNING

#ifndef TRACE_ERROR
#define TRACE_ERROR CLog::instance().error
#endif // TRACE_ERROR

#define LOG_FUNCTION(x) CLogFunction l(x)

class CTimeStamp
{
public:
    CTimeStamp()
    {
	    struct _timeb timeb64 ;
	    _ftime64_s( &timeb64 ) ;
	    _tm = timeb64.time ;
    }

    tstring toString()
    {
		tstring tmp = _T("%Y-%m-%dT%H:%M:%S") ;

        TCHAR buffer[256] = {} ;
		struct tm t = {0} ;
		::gmtime_s( &t, &_tm ) ;

		::_tcsftime( buffer, sizeof(buffer), tmp.c_str(), &t ) ;
        return buffer;
    }

    time_t _tm ;
};

template<class _TimeStamp = CTimeStamp>
class CDefaultFormatter : public _TimeStamp
{
public:
	enum options_t { opNone = 0x0, opTime = 0x1, opThread = 0x2, opLevel = 0x4, opAll = 0x7 };

public:
	void setOptions( int opt ) { _options = (options_t)opt ; }
	int getOptions() const { return _options ; }
	void setMask( int level ) { _mask = level ; }
	int getMask() const { return _mask ; }

protected:
	CDefaultFormatter()
		:	_options(opAll) {}

	~CDefaultFormatter() {}

	tstring format( int level, const TCHAR* format, va_list argl )
	{
        tstring buffer;
		try
		{
			options_t options = _options;

			// add timestamp
			if( options & opTime )
			{
				_TimeStamp t ;
				buffer += StringFormat(_T("%s "), t.toString().c_str() ) ;
			}
			// add thread id, 
			if( options & opThread )
				buffer += StringFormat(_T("%5u "), ::GetCurrentThreadId() ) ;
			// add level
			if( options & opLevel )
				buffer += StringFormat( _T("[%c] "), GetLevelChar(level) ) ;
			// add body
			buffer += Vformat(format, argl);
			// add terminator
			buffer += _T('\n');
		}
		catch( ... )
		{
			buffer += _T("tracer format error");
		}

        return buffer;
	}

	TCHAR GetLevelChar(int level) const
	{
		static const TCHAR chars[] = {'D', 'I', 'W', 'E', 'Z'};
		return chars[level];
	}

	const tstring StringFormat( const TCHAR* format, ... )
	{
		va_list argl;
		va_start(argl, format);
		tstring res = Vformat(format, argl);
		va_end(argl);

		return res;
	}

	const tstring Vformat(const TCHAR* format, va_list argl)
	{
		size_t required_space = ::_vsctprintf(format, argl) + 1;
		tstring res(required_space, _T('\0'));
		int cnt = _vstprintf_s( &res[0], required_space, format, argl );
        res.resize( required_space - 1 );
		return res;
	}

private:
	volatile options_t _options;
};

///////////////////////////////////////////////////////////
// class thread_formatter
//
template<class _Formatter = CDefaultFormatter<> >
class CThreadFormatter : public _Formatter
{
private:
	typedef std::vector<tstring> indentstack_t;
	typedef std::map<int, indentstack_t> threadmap_t;

public:
	void indent(const TCHAR* indent)
	{
		getStack().push_back(indent) ;
	}

	void unindent()
	{
		getStack().pop_back() ;
	}

protected:
	CThreadFormatter() {}
	~CThreadFormatter() {}

	void Format(int level, tstring& buffer, const TCHAR* format, va_list argl)
	{
		const indentstack_t& stack = getStack() ;
		tstring str = std::accumulate(stack.begin(), stack.end(), _empty) ;
		str += format;
		_Formatter::Format(level, buffer, str.c_str(), argl) ;
	}

	indentstack_t& getStack()
	{
		return _threadMap[::GetCurrentThreadId()];
	}

private:
	threadmap_t _threadMap;
	tstring _empty;
} ;

class CFileWriter
{
public:
	enum options_t { opRewrite, opAppend } ;

	CFileWriter() ;
	~CFileWriter() ;
	bool Open( const TCHAR* file, options_t openMode = opAppend ) ;
	void Close() ;
	const tstring GetFile() const ;
	void Write( int level, const tstring& buffer ) ;
	int length() { return _length ; } ;

protected:
	bool OpenFile( const TCHAR* file, options_t openMode ) ;

private:
	tstring _file ;
	int _handle ;
	int _length ;
} ;

class CLog : public CThreadFormatter<>
{
public:
	enum trace_level_t { tlDebug, tlInfo, tlWarning, tlError, tlNone } ;

	void debug( const TCHAR* format, ... ) ;
	void info( const TCHAR* format, ... ) ;
	void warning( const TCHAR* format, ... ) ;
	void error( const TCHAR* format, ... ) ;

	void debug( const tstring& format, ... ) ;
	void info( const tstring& format, ... ) ;
	void warning( const tstring& format, ... ) ;
	void error( const tstring& format, ... ) ;

	void Print( int level, const TCHAR* format, va_list argl ) ;

	static CLog& instance() ;
	void ClearPrefix() ;
	bool Init( const tstring& Name ) ;

private:
	CLog() ;

	CRITICAL_SECTION _guard ;
	volatile int _mask ;
	bool _bDebugOutput ;
	bool _bFileOutput ;
    bool _bConsoleOutput ;

	CFileWriter _FileWriter ;
} ;
