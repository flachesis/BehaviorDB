#ifndef BDB_BDB_EXPORT_H_
#define BDB_BDB_EXPORT_H_

#include "version.hpp"

#ifdef __GNUC__
#define EXPORT_SPEC  __attribute__ ((visibility("default")))
#define IMPORT_SPEC  __attribute__ ((visibility("hidden")))
#endif 

#ifdef _WIN32
#define EXPORT_SPEC __declspec(dllexport)
#define IMPORT_SPEC __declspec(dllimport)
#endif

#if !defined(EXPORT_SPEC) || !defined(IMPORT_SPEC)
#error Unkown compiler
#endif

#ifdef BDB_MAKE_DLL
#pragma message ("Create shared bdb lib with version "BDB_VERSION_ )
#define BDB_EXPORT EXPORT_SPEC
#endif

#ifdef BDB_STATIC
#pragma message ("Create static bdb lib with version "BDB_VERSION_)
#define BDB_EXPORT 
#endif


#ifdef BDB_DLL
#pragma message ("Link with shared bdb lib " BDB_VERSION_ "(dll required)")
#define BDB_EXPORT IMPORT_SPEC 
#elif !defined(BDB_STATIC) && !defined(BDB_MAKE_DLL) 
#pragma message ("You must define BDB_DLL when compile your code with dll version bdb lib.")
#pragma message ("If you are using a static version, just ignore this warning.")
#pragma message ("Link with static bdb lib " BDB_VERSION_)
#define BDB_EXPORT
#endif // end of BDB_DLL


#ifndef BDB_EXPORT
#error export macro error: BDB_EXPORT was not defined
#endif

#endif // end of header
