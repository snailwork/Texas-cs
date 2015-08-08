#pragma once

#if   defined(__native_client__)
#ifndef _stricmp
int _stricmp(const char* s1, const char* s2);
#endif
int _strnicmp(const char* s1, const char* s2, size_t n);
char *_strlwr(char * str );
#define _vsnprintf vsnprintf
#else
#if (defined(__GNUC__)  || defined(__GCCXML__) || defined(__S3E__)) && !defined(_WIN32)
#ifndef _stricmp
int _stricmp(const char* s1, const char* s2);
#endif
int _strnicmp(const char* s1, const char* s2, size_t n);
#ifndef __APPLE__
char *_strlwr(char * str );
#endif



#endif
#endif