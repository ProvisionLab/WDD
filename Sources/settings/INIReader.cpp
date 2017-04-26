// Read an INI file into easy-to-access name/value pairs.

// inih and INIReader are released under the New BSD license (see LICENSE.txt).
// Go to the project home page for more info:
//
// https://github.com/benhoyt/inih

#include "usercommon.h"
#include "INIReader.h"
#include "ini.h"

INIReader::INIReader(const std::wstring& filename)
    : _parserError(0)
{
    _parserError = ini_parse( filename.c_str(), ValueHandler, this );
}

INIReader::~INIReader()
{
    _values.clear();
}

int INIReader::ParseError() const
{
    return _parserError;
}

std::wstring INIReader::Get(const std::wstring& section, const std::wstring& name, const std::wstring& default_value) const
{
    std::wstring key = MakeKey(section, name);
    // Use _values.find() here instead of _values.at() to support pre C++11 compilers
    return _values.count(key) ? _values.find(key)->second : default_value;
}

long INIReader::GetInteger(const std::wstring& section, const std::wstring& name, long default_value) const
{
    std::wstring valstr = Get(section, name, _T(""));
    const TCHAR* value = valstr.c_str();
    TCHAR* end;
    // This parses "1234" (decimal) and also "0x4D2" (hex)
    long n = _tcstol(value, &end, 0);
    return end > value ? n : default_value;
}

double INIReader::GetReal(const std::wstring& section, const std::wstring& name, double default_value) const
{
    std::wstring valstr = Get(section, name, _T(""));
    const TCHAR* value = valstr.c_str();
    TCHAR* end;
    double n = _tcstod(value, &end);
    return end > value ? n : default_value;
}

bool INIReader::GetBoolean(const std::wstring& section, const std::wstring& name, bool default_value) const
{
    std::wstring valstr = Get(section, name, _T(""));
    // Convert to lower case to make wstring comparisons case-insensitive
    std::transform(valstr.begin(), valstr.end(), valstr.begin(), ::_totlower);
    if (valstr == _T("true") || valstr == _T("yes") || valstr == _T("on") || valstr == _T("1"))
        return true;
    else if (valstr == _T("false") || valstr == _T("no") || valstr == _T("off") || valstr == _T("0"))
        return false;
    else
        return default_value;
}

std::wstring INIReader::MakeKey(const std::wstring& section, const std::wstring& name)
{
    std::wstring key = section + _T("=") + name;
    // Convert to lower case to make section/name lookups case-insensitive
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    return key;
}

int INIReader::ValueHandler(void* user, const TCHAR* section, const TCHAR* name, const TCHAR* value)
{
    INIReader* reader = (INIReader*)user;
    std::wstring key = MakeKey(section, name);
    if (reader->_values[key].size() > 0)
        reader->_values[key] += _T("\n");
    reader->_values[key] += value;
    return 1;
}
