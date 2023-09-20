//
// Created by bobqianic on 9/19/2023.
//

#ifndef CONSOLE_H
#define CONSOLE_H

#include <string>
#include <cstdarg>
#if WIN32
#define NOMINMAX
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

namespace {
#if WIN32
    // use std::wstring on Windows
    typedef std::wstring ustring;
#else
    // use std::string on other platforms
    typedef std::string ustring;
#endif

#if WIN32
    // Convert UTF-8 to UTF-16
    // Windows only
    std::wstring ConvertUTF8toUTF16(const std::string& utf8Str) {
        if (utf8Str.empty()) return {std::wstring()};

        int requiredSize = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
        if (requiredSize == 0) {
            // Handle error here
            return {std::wstring()};
        }

        std::wstring utf16Str(requiredSize, 0);
        if (MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &utf16Str[0], requiredSize) == 0) {
            // Handle error here
            return {std::wstring()};
        }

        // Remove the additional null byte from the end
        utf16Str.resize(requiredSize - 1);

        return utf16Str;
    }
#endif

#if WIN32
    // Convert UTF-16 to UTF-8
    // Windows only
    std::string ConvertUTF16toUTF8(const std::wstring & utf16Str) {
        if (utf16Str.empty()) return {std::string()};

        int requiredSize = WideCharToMultiByte(CP_UTF8, 0, utf16Str.c_str(), -1, NULL, 0, NULL, NULL);
        if (requiredSize == 0) {
            // Handle error here
            return {std::string()};
        }

        std::string utf8Str(requiredSize, 0);
        if (WideCharToMultiByte(CP_UTF8, 0, utf16Str.c_str(), -1, &utf8Str[0], requiredSize, NULL, NULL) == 0) {
            // Handle error here
            return {std::string()};
        }

        // Remove the additional null byte from the end
        utf8Str.resize(requiredSize - 1);

        return utf8Str;
    }
#else
    // Dummy function
    std::string ConvertUTF16toUTF8(const std::string & utf8Str) {
        return utf8Str;
    }
#endif

#if WIN32
    // Convert bool to std::wstring
    // true -> "true", false -> "false"
    // Windows only
    std::wstring bool2ustr(const bool & a) {
        if (a) {
            return ConvertUTF8toUTF16("true");
        }
        return ConvertUTF8toUTF16("false");
    }

    // Convert bool to std::wstring
    // true -> var(a) false -> var(b)
    // Windows only
    std::wstring bool2ustr(const bool & value, const std::string & a, const std::string & b) {
        if (value) {
            return ConvertUTF8toUTF16(a);
        }
        return ConvertUTF8toUTF16(b);
    }
#else
    // Convert bool to std::string
    // true -> "true", false -> "false"
    std::string bool2ustr(const bool & a) {
        if (a) {
            return "true";
        }
        return "false";
    }

    // Convert bool to std::string
    // true -> var(a) false -> var(b)
    std::string bool2ustr(const bool & value, const std::string & a, const std::string & b) {
        if (value) {
            return a;
        }
        return b;
    }
#endif

#if WIN32
    bool equal(const std::wstring & a, const std::string & b) {
        if (a == ConvertUTF8toUTF16(b)) {
            return true;
        }
        return false;
    }

    bool equal(const std::string & a, const std::wstring & b) {
        if (ConvertUTF8toUTF16(a) == b) {
            return true;
        }
        return false;
    }

    bool equal(const std::wstring & a, const std::wstring & b) {
        if (a == b) {
            return true;
        }
        return false;
    }

    bool equal(const wchar_t & a, const char & b) {
        // Checking if it's within ASCII range
        if (a <= 0x7F) {
            if (static_cast<char>(a) == b) {
                return true;
            }
        }
        return false;
    }

    bool equal(const char & a, const wchar_t & b) {
        // Checking if it's within ASCII range
        if (b <= 0x7F) {
            if (a == static_cast<char>(b)) {
                return true;
            }
        }
        return false;
    }

    bool equal(const wchar_t & a, const wchar_t & b) {
        if (a == b) {
            return true;
        }
        return false;
    }
#endif

    bool equal(const std::string & a, const std::string & b) {
        if (a == b) {
            return true;
        }
        return false;
    }

    bool equal(const char & a, const char & b) {
        if (a == b) {
            return true;
        }
        return false;
    }

    // Unified fprintf
    // All outputs use UTF-16 on Windows and UTF-8 on other systems
    // All inputs use UTF-8
    void fuprintf(FILE* const stream, const char* format = "", ...) {
        va_list argptr;
        va_start(argptr, format);
    #if WIN32
        vfwprintf(stream, ConvertUTF8toUTF16(format).c_str(), argptr);
    #else
        vfprintf(stream, format, argptr);
    #endif
        va_end(argptr);
    }

#if WIN32
    // Unified fprintf
    // Windows only
    // All outputs use UTF-16
    // All inputs use UTF-16
    void fuprintf(FILE* const stream, const wchar_t* format = L"", ...) {
        va_list argptr;
        va_start(argptr, format);
        vfwprintf(stream, format, argptr);
        va_end(argptr);
    }
#endif

    // Unified printf
    // All outputs use UTF-16 on Windows and UTF-8 on other systems
    // All inputs use UTF-8
    void uprintf(const char* format = "", ...) {
        va_list argptr;
        va_start(argptr, format);
    #if WIN32
        vfwprintf(stdout, ConvertUTF8toUTF16(format).c_str(), argptr);
    #else
        vfprintf(stdout, format, argptr);
    #endif
        va_end(argptr);
    }

#if WIN32
    // Unified printf
    // Windows only
    // All outputs use UTF-16
    // All inputs use UTF-16
    void uprintf(const wchar_t* format = L"", ...) {
        va_list argptr;
        va_start(argptr, format);
        vfwprintf(stdout, format, argptr);
        va_end(argptr);
    }
#endif

    // initialize the console
    // set output encoding
    bool init_console() {
    #if WIN32
        _setmode(_fileno(stdout), _O_U16TEXT);
        _setmode(_fileno(stderr), _O_U16TEXT);
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) {
            return GetLastError();
        }

        DWORD dwMode = 0;
        if (!GetConsoleMode(hOut, &dwMode)) {
            return GetLastError();
        }

        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (!SetConsoleMode(hOut, dwMode)) {
            return GetLastError();
        }
    #endif
        return true;
    }
}

#endif //CONSOLE_H
