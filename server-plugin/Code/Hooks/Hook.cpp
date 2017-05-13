/*
	Copyright 2012 - Le Padellec Sylvain

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "Hook.h"
#include "Interfaces/InterfacesProxy.h"

#ifdef GNUC
#	include <sys/mman.h>
#	include <unistd.h>
#else
#	include "Misc/include_windows_headers.h"
#endif

#include "Systems/Logger.h" // DebugMessage
#include "temp_HookGuard.h" // HookInfo

/*
	This function will help in debugging error messages, and also helps finding what the plugin is doing in debug mode.
*/
basic_string GetModuleNameFromMemoryAddress ( DWORD mem_address )
{
	if (!mem_address)
		return "<unknown module>";

#ifdef WIN32
	HMODULE module;
	if( GetModuleHandleExA ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPSTR>(mem_address), &module ) )
	{
		char buf[ 1024 ];
		GetModuleFileNameA ( module, buf, 1023 );
		buf[ 1023 ] = '\0'; // Windows XP does not null-terminate string when truncated ... I do this even if we don't target XP.
		basic_string filename ( buf );
		size_t path_end ( filename.find_last_of ( "/\\" ) );
		if( path_end != basic_string::npos )
		{
			filename.remove ( 0, path_end );
		}
		return filename;
	}
	else
	{
		return "<error determining module>";
	}
#else
	Dl_info info;
	dladdr ( reinterpret_cast<void*>( mem_address ), &info );
	basic_string filename ( info.dli_fname );
	size_t path_end ( filename.find_last_of ( "/\\" ) );
	if( path_end != basic_string::npos )
	{
		filename.remove ( 0, path_end );
	}
	return filename;
#endif
}

/*
	This function gets the current directory of the module given the address. Note this does not include the module name.
*/
basic_string GetModuleDirectoryFromMemoryAddress ( DWORD mem_address )
{
#ifdef WIN32
	HMODULE module;
	if( GetModuleHandleExA ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPSTR>( mem_address ), &module ) )
	{
		char buf[ 1024 ];
		GetModuleFileNameA ( module, buf, 1023 );
		buf[ 1023 ] = '\0'; // Windows XP does not null-terminate string when truncated ... I do this even if we don't target XP.
		basic_string path ( buf );
		size_t path_end ( path.find_last_of ( "/\\" ) );
		if( path_end != basic_string::npos )
		{
			path.remove ( path_end + 1, basic_string::npos );
		}
		return path;
	}
	else
	{
		return "<error determining module>";
	}
#else
	Dl_info info;
	dladdr ( reinterpret_cast<void*>( mem_address ), &info );
	basic_string path ( info.dli_fname );
	size_t path_end ( filename.find_last_of ( "/\\" ) );
	if( path_end != basic_string::npos )
	{
		path.remove ( path_end + 1, basic_string::npos );
	}
	return path;
#endif
}

int HookCompare ( HookInfo const * a, HookInfo const * b )
{
	if( a->vf_entry > b->vf_entry ) return 1;
	else if( a->vf_entry < b->vf_entry ) return -1;
	else return 0;
}

void MoveVirtualFunction ( DWORD const * const from, DWORD * const to )
{
#ifdef WIN32
	DWORD dwOld;
	if( !VirtualProtect ( to, sizeof ( DWORD ), PAGE_EXECUTE_READWRITE, &dwOld ) )
	{
		return;
	}
#else // LINUX
	uint32_t psize ( sysconf ( _SC_PAGESIZE ) );
	void *p = ( void * ) ( ( DWORD ) ( to ) & ~( psize - 1 ) );
	if( mprotect ( p, sizeof ( DWORD ) & ( psize - 1 ), PROT_READ | PROT_WRITE | PROT_EXEC ) < 0 )
	{
		return;
	}
#endif // WIN32
	DebugMessage ( Helpers::format ( "MoveVirtualFunction : function 0x%X replaced by 0x%X.", *to, *from ) );
	*to = *from;
#ifdef WIN32
	VirtualProtect ( to, sizeof ( DWORD ), dwOld, &dwOld );
#else // LINUX
	mprotect ( p, sizeof ( DWORD ) & ( psize - 1 ), PROT_READ | PROT_EXEC );
#endif // WIN32
}

/*
	HookInfo
*/

HookInfo::HookInfo ()
{
	memset ( this, 0, sizeof ( HookInfo ) );
}

HookInfo::HookInfo ( const HookInfo& other )
{
	memcpy ( this, &other, sizeof ( HookInfo ) );
}

HookInfo& HookInfo::operator=( const HookInfo& other )
{
	memcpy ( this, &other, sizeof ( HookInfo ) );
	return *this;
}

HookInfo::HookInfo ( void* class_ptr, int vfid, DWORD new_fn ) :
	oldFn ( 0 ),
	origEnt ( class_ptr ),
	pInterface ( ( DWORD* )*( DWORD* ) origEnt ),
	vf_entry ( &( pInterface[ vfid ] ) ),
	newFn ( new_fn )
{

}

HookInfo::HookInfo ( void* class_ptr, int vfid ) :
	origEnt ( class_ptr ),
	pInterface ( ( DWORD* )*( DWORD* ) origEnt ),
	vf_entry ( &( pInterface[ vfid ] ) )
{

}

bool HookInfo::operator== ( const HookInfo& other ) const
{
	return ( vf_entry == other.vf_entry );
}
