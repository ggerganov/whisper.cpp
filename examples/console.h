//
// Created by bobqianic on 9/19/2023.
//

#ifndef CONSOLE_H
#define CONSOLE_H

#include <string>
#if _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

#if _WIN32
// use std::wstring on Windows
typedef std::wstring ustring;
#else
// use std::string on other platforms
typedef std::string ustring;
#endif

#if _WIN32
// Convert UTF-8 to UTF-16
// Windows only
inline std::wstring ConvertUTF8toUTF16(const std::string& utf8Str) {
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

#if _WIN32
// Convert UTF-16 to UTF-8
// Windows only
inline std::string ConvertUTF16toUTF8(const std::wstring & utf16Str) {
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
#endif

// initialize the console
// set output encoding
inline bool init_console() {
#if _WIN32
    // set output encoding to UTF-8
    SetConsoleOutputCP(CP_UTF8);
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

#endif //CONSOLE_H
