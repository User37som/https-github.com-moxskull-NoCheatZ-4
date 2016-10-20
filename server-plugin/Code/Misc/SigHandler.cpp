/*
	Public domain code.

	Inspired a lot from : https://oroboro.com/stack-trace-on-crash/
*/

#include "SigHandler.h"

#include <exception>
#include <stdint.h>

#ifdef GNUC
#	include <unistd.h>
#	include <execinfo.h> // backtrace
#	include <dlfcn.h> // dladdr
#endif

#include "Interfaces/InterfacesProxy.h"

#ifdef WIN32
#	include "StackWalker.h"

StackWalker_OutFile::StackWalker_OutFile ( FILE * outfile,
										   int options,
										   LPCSTR szSymPath,
										   DWORD dwProcessId,
										   HANDLE hProcess ) : StackWalker ( options, szSymPath, dwProcessId, hProcess )
{
	m_out_file = outfile;
}

StackWalker_OutFile::~StackWalker_OutFile ()
{
	if( m_out_file )
		fclose ( m_out_file );
}

void StackWalker_OutFile::OnOutput ( LPCSTR szText )
{
	if( m_out_file )
		fprintf ( m_out_file, "%s", szText );
	//StackWalker::OnOutput ( szText );
	printf ( "%s", szText );
}

#endif // WIN32

#define TRACE_MAX_FRAMES 127;

static inline void printStackTrace ( FILE * out )
{
#ifdef GNUC
	// storage array for stack trace address data
	void* addrlist[ 128 + 1 ];

	// retrieve current stack addresses
	uint32_t addrlen = backtrace ( addrlist, sizeof ( addrlist ) / sizeof ( void* ) );

	if( addrlen == 0 )
	{
		fprintf ( out, " (no trace - stack may be corrupted)  \n" );
		return;
	}

	// create readable strings to each frame.
	backtrace_symbols_fd ( addrlist, addrlen, fileno(out) );
#else
	StackWalker_OutFile sw(out);
	sw.ShowCallstack ();
#endif
}

static FILE * OpenStackFile ()
{
	static char filename[ 1024 ];
	memset ( filename, 0, 1024 );
	char * it ( filename );
#ifdef WIN32
	static HMODULE module;
	GetModuleHandleExA ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast< LPSTR >( filename ), &module );
	GetModuleFileNameA ( module, filename, 1023 );
#else
	static Dl_info info;
	dladdr ( reinterpret_cast<void*>( filename ), &info );
	memcpy ( filename, info.dli_fname, 1023 );
#endif
	it += strlen( filename );
	while( *it != '\\' && *it != '/' ) --it;
	memcpy ( ++it, "!stacktrace.txt", 16 );
	
	printf("Opening %s\n", filename );

	return fopen ( filename, "a" );
}

void abortHandler ( int signum )
{
	FILE* outfile = OpenStackFile ();

	FILE* out;
	if( outfile == nullptr )
	{
		printf( "Unable to open !stacktrace.txt\n" );
		out = stdout;
	}
	else
	{
		out = outfile;
	}

	// associate each signal with a signal name string.
	const char* name = NULL;
	switch( signum )
	{
		case SIGABRT: name = "SIGABRT";  break;
		case SIGSEGV: name = "SIGSEGV";  break;
#ifdef GNUC
		case SIGBUS:  name = "SIGBUS";   break;
#endif
		case SIGILL:  name = "SIGILL";   break;
		case SIGFPE:  name = "SIGFPE";   break;
		default:  name = "Unknown signum"; break;
	}

	// Notify the user which signal was caught. We use printf, because this is the 
	// most basic output function. Once you get a crash, it is possible that more 
	// complex output systems like streams and the like may be corrupted. So we 
	// make the most basic call possible to the lowest level, most 
	// standard print function.
	if( name )
		fprintf ( out, "Caught signal %d (%s)\n", signum, name );
	else
		fprintf ( out, "Caught signal %d\n", signum );

	// Dump a stack trace.
	// This is the function we will be implementing next.
	printStackTrace ( out );

	// If you caught one of the above signals, it is likely you just 
	// want to quit your program right now.

	if( outfile )
		fclose ( outfile );

	exit ( signum );
}

void TermFn ()
{
	raise ( SIGABRT );
}

#ifdef WIN32
static LPTOP_LEVEL_EXCEPTION_FILTER old_filter;

void BaseFilter ( LPEXCEPTION_POINTERS info )
{
	FILE* outfile = OpenStackFile ();

	FILE* out;
	if( outfile == nullptr )
	{
		out = stderr;
	}
	else
	{
		out = outfile;
	}

	fprintf ( out, "Caught Unhandled Exception code %X :\nFlags : %X\nAddress : %X\n", 
			  info->ExceptionRecord->ExceptionCode,
			  info->ExceptionRecord->ExceptionFlags,
			  info->ExceptionRecord->ExceptionAddress );

	StackWalker_OutFile sw ( out );
	sw.ShowCallstack (GetCurrentThread(), info->ContextRecord);

	if( outfile )
		fclose ( outfile );
}

LONG WINAPI UFilter ( LPEXCEPTION_POINTERS info )
{
	if( info->ExceptionRecord->ExceptionCode == DBG_PRINTEXCEPTION_C )
		return EXCEPTION_CONTINUE_EXECUTION;

	BaseFilter ( info );

	if( old_filter )
		return old_filter ( info );
	else
		return EXCEPTION_CONTINUE_SEARCH;
}

LONG WINAPI VFilter ( LPEXCEPTION_POINTERS info )
{
	if( info->ExceptionRecord->ExceptionCode == DBG_PRINTEXCEPTION_C )
		return EXCEPTION_CONTINUE_EXECUTION;

	BaseFilter ( info );

	return EXCEPTION_CONTINUE_SEARCH;
}

#endif

KxStackTrace::KxStackTrace ()
{
	signal ( SIGABRT, abortHandler );
	signal ( SIGSEGV, abortHandler );
	signal ( SIGILL, abortHandler );
	signal ( SIGFPE, abortHandler );
#ifdef GNUC
	signal ( SIGBUS, abortHandler );
#endif
	std::set_terminate ( TermFn );
	std::set_unexpected ( TermFn );
#ifdef WIN32
	old_filter = SetUnhandledExceptionFilter ( UFilter );
	AddVectoredExceptionHandler ( 0, VFilter );
#endif
}