/* inih -- simple .INI file parser

inih is released under the New BSD license (see LICENSE.txt). Go to the project
home page for more info:

https://github.com/benhoyt/inih

*/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "usercommon.h"
#include "ini.h"

#if !INI_USE_STACK
#includez <stdlib.h>
#endif

#define MAX_SECTION 50
#define MAX_NAME 50

/* Strip whitespace chars off end of given string, in place. Return s. */
static TCHAR* rstrip(TCHAR* s)
{
    TCHAR* p = s + _tcslen(s);
    while (p > s && _istspace( *--p ) )
        *p = '\0';
    return s;
}

/* Return pointer to first non-whitespace TCHAR in given string. */
static TCHAR* lskip(const TCHAR* s)
{
    while( *s && _istspace( *s ) )
        s++;
    return (TCHAR*)s;
}

/* Return pointer to first TCHAR (of chars) or inline comment in given string,
   or pointer to null at end of string if neither found. Inline comment must
   be prefixed by a whitespace character to register as a comment. */
static TCHAR* find_chars_or_comment(const TCHAR* s, const TCHAR* chars)
{
#if INI_ALLOW_INLINE_COMMENTS
    int was_space = 0;
    while (*s && (!chars || !_tcschr(chars, *s)) && !(was_space && _tcschr(INI_INLINE_COMMENT_PREFIXES, *s)))
    {
        was_space = _istspace( *s );
        s++;
    }
#else
    while (*s && (!chars || !_tcschr(chars, *s)))
    {
        s++;
    }
#endif
    return (TCHAR*)s;
}

/* Version of strncpy that ensures dest (size bytes) is null-terminated. */
static TCHAR* strncpy0(TCHAR* dest, const TCHAR* src, size_t size)
{
    _tcsncpy(dest, src, size);
    dest[size - 1] = '\0';
    return dest;
}

/* See documentation in header file. */
int ini_parse_stream(FILE* stream, ini_handler handler, void* user)
{
    /* Uses a fair bit of stack (use heap instead if you need to) */
#if INI_USE_STACK
    char line[INI_MAX_LINE] = {};
#else
    char* line = NULL;
#endif
    TCHAR section[MAX_SECTION] = _T("");
    TCHAR prev_name[MAX_NAME] = _T("");

    TCHAR* tline = NULL;
    TCHAR* start = NULL;
    TCHAR* end = NULL;
    TCHAR* name = NULL;
    TCHAR* value = NULL;
    int lineno = 0;
    int error = 0;

#if !INI_USE_STACK
    line = (char*)malloc(INI_MAX_LINE);
    if( ! line )
    {
        return -2;
    }
#endif

#if INI_HANDLER_LINENO
#define HANDLER(u, s, n, v) handler(u, s, n, v, lineno)
#else
#define HANDLER(u, s, n, v) handler(u, s, n, v)
#endif

    /* Scan through stream line by line */
    while ( fgets(line, INI_MAX_LINE, stream) != NULL)
    {
        lineno++;

        start = tline = (TCHAR*)line;
#if INI_ALLOW_BOM
        if (lineno == 1 && start[0] == 0xEF && start[1] == 0xBB && start[2] == 0xBF) 
        {
            start += 3;
        }
        if (lineno == 1 && start[0] == 0xFEFF)
        {
            start += 1;
        }
        if (lineno > 1 )
        {
            start = tline = (TCHAR*)((char*)start + 1);
        }
#endif
        start = lskip(rstrip(start));

        if (*start == _T(';') || *start == _T('#') ) {
            /* Per Python configparser, allow both ; and # comments at the
               start of a line */
        }
#if INI_ALLOW_MULTILINE
        else if (*prev_name && *start && start > tline) {
            /* Non-blank line with leading whitespace, treat as continuation
               of previous name's value (as per Python config parser). */
            if (!HANDLER(user, section, prev_name, start) && !error)
                error = lineno;
        }
#endif
        else if (*start == _T('[')) {
            /* A "[section]" line */
            end = find_chars_or_comment(start + 1, _T("]"));
            if (*end == _T(']')) {
                *end = _T('\0');
                strncpy0(section, start + 1, sizeof(section)/sizeof(TCHAR));
                *prev_name = _T('\0');
            }
            else if (!error) {
                /* No ']' found on section line */
                error = lineno;
            }
        }
        else if (*start) {
            /* Not a comment, must be a name[=:]value pair */
            end = find_chars_or_comment(start, _T("=:"));
            if (*end == _T('=') || *end == _T(':')) {
                *end = _T('\0');
                name = rstrip(start);
                value = end + 1;
#if INI_ALLOW_INLINE_COMMENTS
                end = find_chars_or_comment(value, NULL);
                if (*end)
                    *end = _T('\0');
#endif
                value = lskip(value);
                rstrip(value);

                /* Valid name[=:]value pair found, call handler */
                strncpy0(prev_name, name, sizeof(prev_name)/sizeof(TCHAR));
                if (!HANDLER(user, section, name, value) && !error)
                    error = lineno;
            }
            else if (!error) {
                /* No '=' or ':' found on name[=:]value line */
                error = lineno;
            }
        }

#if INI_STOP_ON_FIRST_ERROR
        if (error)
            break;
#endif
        memset( line, 0, INI_MAX_LINE);
    }

#if !INI_USE_STACK
    free(line);
#endif

    return error;
}

/* See documentation in header file. */
int ini_parse_file( FILE* file, ini_handler handler, void* user )
{
    return ini_parse_stream( file, handler, user );
}

/* See documentation in header file. */
int ini_parse( const TCHAR* filename, ini_handler handler, void* user )
{
    FILE* file;
    int error;

    file = _tfopen( filename, _T("r") );
    if( ! file )
        return -1;
    error = ini_parse_file( file, handler, user );
    fclose( file );
    return error;
}
