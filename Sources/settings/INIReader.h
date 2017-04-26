// Read an INI file into easy-to-access name/value pairs.

// inih and INIReader are released under the New BSD license (see LICENSE.txt).
// Go to the project home page for more info:
//
// https://github.com/benhoyt/inih

#ifndef __INIREADER_H__
#define __INIREADER_H__

// Read an INI file into easy-to-access name/value pairs. (Note that I've gone
// for simplicity here rather than speed, but it should be pretty decent.)
class INIReader
{
public:
    // Construct INIReader and parse given filename. See ini.h for more info
    // about the parsing.
    INIReader(const std::wstring& filename);
	virtual ~INIReader();

    // Return the result of ini_parse(), i.e., 0 on success, line number of
    // first error on parse error, or -1 on file open error.
    int ParseError() const;

    // Get a wstring value from INI file, returning default_value if not found.
    std::wstring Get(const std::wstring& section, const std::wstring& name,
                    const std::wstring& default_value) const;

    // Get an integer (long) value from INI file, returning default_value if
    // not found or not a valid integer (decimal "1234", "-1234", or hex "0x4d2").
    long GetInteger(const std::wstring& section, const std::wstring& name, long default_value) const;

    // Get a real (floating point double) value from INI file, returning
    // default_value if not found or not a valid floating point value
    // according to strtod().
    double GetReal(const std::wstring& section, const std::wstring& name, double default_value) const;

    // Get a boolean value from INI file, returning default_value if not found or if
    // not a valid true/false value. Valid true values are "true", "yes", "on", "1",
    // and valid false values are "false", "no", "off", "0" (not case sensitive).
    bool GetBoolean(const std::wstring& section, const std::wstring& name, bool default_value) const;

private:
    std::map<std::wstring, std::wstring> _values;
    int _parserError;
    static std::wstring MakeKey(const std::wstring& section, const std::wstring& name);
    static int ValueHandler(void* user, const TCHAR* section, const TCHAR* name, const TCHAR* value);
};

#endif  // __INIREADER_H__
