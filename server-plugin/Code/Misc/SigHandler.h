/*
	Public domain code.

	Inspired a lot from : https://oroboro.com/stack-trace-on-crash/
*/

#ifndef SIGHANDLER_H
#define SIGHANDLER_H

#include <stdio.h>
#include <signal.h>

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
	KxStackTrace ();
};

#endif // SIGHANDLER_H


