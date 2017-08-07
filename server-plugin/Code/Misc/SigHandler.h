/*
	Public domain code.

	Inspired a lot from : https://oroboro.com/stack-trace-on-crash/
*/

#ifndef SIGHANDLER_H
#define SIGHANDLER_H

#include <stdio.h>
#include <signal.h>
#include <exception>

#ifdef WIN32

#include "StackWalker.h"

class StackWalker_OutFile : public StackWalker
{
private:
	FILE* m_out_file;

public:
	StackWalker_OutFile ( FILE * outfile,
						  int options = StackWalker::OptionsAll,
						  LPCSTR szSymPath = NULL,
						  DWORD dwProcessId = GetCurrentProcessId (),
						  HANDLE hProcess = GetCurrentProcess () );

	virtual ~StackWalker_OutFile ();

	virtual void OnOutput ( LPCSTR szText );
};

#endif

class KxStackTrace
{
public:
	//static std::terminate_handler old_term;
	static std::unexpected_handler old_unexp;
#ifdef WIN32
	static LPTOP_LEVEL_EXCEPTION_FILTER old_ufilter;
	static LPTOP_LEVEL_EXCEPTION_FILTER old_vfilter;
#endif

	KxStackTrace ();
	~KxStackTrace();
};

#endif // SIGHANDLER_H


