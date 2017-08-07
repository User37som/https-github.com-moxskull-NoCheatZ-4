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

	FILE * pfile(fopen(filename, "a"));

	if (pfile)
	{
		fprintf(pfile, "Using plugin version %s-%s-%s\n\n",
			NCZ_VERSION_GIT,
#ifdef DEBUG
			"Debug",
#else
			"Release",
#endif
#ifdef GNUC
			"Linux"
#else
			"Windows"
#endif
		);
	}

	return pfile;
}

void abortHandler ( int signum )
{
#ifdef GNUC
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
		case SIGBUS:  name = "SIGBUS";   break;
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
		fprintf ( out, "Caught signal %d (%s)\n\n", signum, name );
	else
		fprintf ( out, "Caught signal %d\n\n", signum );

	// Dump a stack trace.
	// This is the function we will be implementing next.
	printStackTrace ( out );

	// If you caught one of the above signals, it is likely you just 
	// want to quit your program right now.

	if( outfile )
		fclose ( outfile );

	exit(signum);
#else
	throw signum; // Just use C++ UnhandledException filters on windows.
				 // This allows us to get current thread context without
				 // the need to use assembly code.
#endif
}

void TermFn ()
{
#ifdef GNUC
	raise ( SIGABRT );
#else
	throw SIGABRT;
#endif
}

#ifdef WIN32
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

	fprintf ( out,  "Caught Unhandled Exception code 0x%X :\n"
					"Flags : 0x%X\n"
					"Address : 0x%X\n"
					"Dr0 : 0x%X, Dr1 : 0x%X, Dr2 : 0x%X, Dr3 : 0x%X, Dr6 : 0x%X, Dr7 : 0x%X\n"
					"eax : 0x%X, ebp : 0x%X, ecx : 0x%X, edi : 0x%X\n"
					"edx : 0x%X, eip : 0x%X, esi : 0x%X, esp : 0x%X\n",
			  info->ExceptionRecord->ExceptionCode,
			  info->ExceptionRecord->ExceptionFlags,
			  (DWORD)info->ExceptionRecord->ExceptionAddress,
				info->ContextRecord->Dr0,
				info->ContextRecord->Dr1,
				info->ContextRecord->Dr2,
				info->ContextRecord->Dr3,
				info->ContextRecord->Dr6,
				info->ContextRecord->Dr7,
				info->ContextRecord ->Eax,
				info->ContextRecord ->Ebp,
				info->ContextRecord ->Ecx,
				info->ContextRecord ->Edi,
				info->ContextRecord ->Edx,
				info->ContextRecord ->Eip,
				info->ContextRecord ->Esi,
				info->ContextRecord ->Esp
		);

	StackWalker_OutFile sw ( out );
	sw.ShowCallstack (GetCurrentThread(), info->ContextRecord);

	if( outfile )
		fclose ( outfile );
}

LONG WINAPI UFilter ( LPEXCEPTION_POINTERS info )
{
	if( info->ExceptionRecord->ExceptionCode == DBG_PRINTEXCEPTION_C || info->ExceptionRecord->ExceptionCode == 0x4001000A)
		return EXCEPTION_CONTINUE_EXECUTION;

	BaseFilter ( info );

	if( KxStackTrace::old_ufilter )
		return KxStackTrace::old_ufilter ( info );
	else
		return EXCEPTION_CONTINUE_SEARCH;
}

LONG WINAPI VFilter ( LPEXCEPTION_POINTERS info )
{
	if( info->ExceptionRecord->ExceptionCode == DBG_PRINTEXCEPTION_C || info->ExceptionRecord->ExceptionCode == 0x4001000A)
		return EXCEPTION_CONTINUE_EXECUTION;

	BaseFilter ( info );

	return EXCEPTION_CONTINUE_SEARCH;
}

#endif

std::unexpected_handler KxStackTrace::old_unexp = 0;
#ifdef WIN32
LPTOP_LEVEL_EXCEPTION_FILTER KxStackTrace::old_ufilter = 0;
LPTOP_LEVEL_EXCEPTION_FILTER KxStackTrace::old_vfilter = 0;
#endif

KxStackTrace::KxStackTrace ()
{
#ifdef WIN32
	if (!IsDebuggerPresent())
#endif
	{

		signal(SIGABRT, abortHandler);
		signal(SIGSEGV, abortHandler);
		signal(SIGILL, abortHandler);
		signal(SIGFPE, abortHandler);
#ifdef GNUC
		signal(SIGBUS, abortHandler);
#endif

		//old_term = std::set_terminate(TermFn);
		old_unexp = std::set_unexpected(TermFn);
#ifdef WIN32
		old_ufilter = SetUnhandledExceptionFilter(UFilter);
		AddVectoredExceptionHandler(0, VFilter);
#endif
	}
}

KxStackTrace::~KxStackTrace()
{
#ifdef WIN32
	if (!IsDebuggerPresent())
#endif
	{
		signal(SIGABRT, SIG_DFL);
		signal(SIGSEGV, SIG_DFL);
		signal(SIGILL, SIG_DFL);
		signal(SIGFPE, SIG_DFL);
#ifdef GNUC
		signal(SIGBUS, SIG_DFL);
#endif
		//std::set_terminate(old_term);
		std::set_unexpected(nullptr);
#ifdef WIN32
		SetUnhandledExceptionFilter(nullptr);
		RemoveVectoredExceptionHandler(VFilter);
#endif
	}
}