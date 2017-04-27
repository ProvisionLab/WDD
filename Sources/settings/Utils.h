#pragma once

namespace Utils
{
    tstring MapToDestination( const tstring& Destination, const tstring& Path );
	tstring MapToOriginal( const tstring& Destination, const tstring& Path );
    bool GetLastIndex( const tstring& MappedPathNoIndex, int& Index );
    tstring RemoveEndingSlash( const tstring& Dir );
    tstring ToLower( const tstring& str );
    bool DoesDirectoryExists( const tstring& Directory );
    bool CreateDirectories( const tstring& Directory );
    tstring GetLastErrorString();

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
	};
}