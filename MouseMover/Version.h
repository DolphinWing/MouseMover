#pragma once

#define VERSION_MAJOR        2
#define VERSION_MINOR        0
#define VERSION_PATCH        0
#define VERSION_BUILD        0

// Preprocessor stringification helpers
#define _STR(x)              #x
#define STR(x)               _STR(x)
#define _WSTR(x)             L#x
#define WSTR(x)              _WSTR(x)

// Comma-separated format for Windows VERSIONINFO (e.g. 2,0,0,0)
#define VERSION_RC_NUM       VERSION_MAJOR,VERSION_MINOR,VERSION_PATCH,VERSION_BUILD

// String formats for metadata and runtime UI display (e.g. "2.0.0" and L"2.0.0")
#define VERSION_STRING       STR(VERSION_MAJOR) "." STR(VERSION_MINOR) "." STR(VERSION_PATCH)
#define VERSION_STRING_W     WSTR(VERSION_MAJOR) L"." WSTR(VERSION_MINOR) L"." WSTR(VERSION_PATCH)
