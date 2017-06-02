#pragma once

namespace Utils
{
    tstring MapToDestination( const tstring& Destination, const tstring& Path );
	tstring MapToOriginal( const tstring& Destination, const tstring& Path );
    bool GetLastIndex( const tstring& DstPathNoIndex, int& Index );
    bool GetIndexCount( const tstring& DstPathNoIndex, int& Count );
    tstring RemoveEndingSlash( const tstring& Dir );
    tstring ToLower( const tstring& str );
    bool DoesDirectoryExists( const tstring& Directory );
    bool CreateDirectory( const tstring& Directory );
	bool RemoveDirectory( const tstring& Directory );
	tstring GetLastErrorString();
	tstring GetErrorString( DWORD err );
	bool ExecuteProcess( const char* CommadLineA, BOOL Wait = TRUE );
    bool LogToFile( const std::string& log );
    __int64 NowTime();
    __int64 FileTimeToInt64( FILETIME FTime );
    __int64 SubstructDays( __int64 Time, int Days );
    __int64 AddMinutes( __int64 Time, int Minutes );
    int TimeToMinutes( __int64 Time );
    unsigned short Crc16( const unsigned char* Data, unsigned long Length, unsigned short crc = 0xFFFF );

	class CPathDetails
	{
	public:
		CPathDetails();
		bool Parse( bool aMapped, const tstring& Path );

		tstring Directory;
		tstring Name;
		tstring Extension;
		tstring Index;
		bool Mapped;
        bool Deleted;
	};
}

class CAutoLock
{
public:
    CAutoLock( CRITICAL_SECTION* Section )
        : _Section( Section )
    {
        EnterCriticalSection( _Section );
    }

    ~CAutoLock()
    {
        LeaveCriticalSection( _Section );
    }

    CRITICAL_SECTION* _Section;
};
