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

void ReplaceVirtualFunctionByFakeVirtual(DWORD const replace_by, DWORD * const replace_here)
{
#ifdef WIN32
	DWORD dwOld;
	if (!VirtualProtect(replace_here, sizeof(DWORD), PAGE_EXECUTE_READWRITE, &dwOld))
	{
		return;
	}
#else // LINUX
	uint32_t psize(sysconf(_SC_PAGESIZE));
	void *p = (void *)((DWORD)(replace_here) & ~(psize - 1));
	if (mprotect(p, sizeof(DWORD) & (psize - 1), PROT_READ | PROT_WRITE | PROT_EXEC) < 0)
	{
		return;
	}
#endif // WIN32
	DebugMessage(Helpers::format("ReplaceVirtualFunctionByFakeVirtual : function 0x%X replaced by 0x%X.", replace_by, *replace_here));
	*replace_here = replace_by;
#ifdef WIN32
	VirtualProtect(replace_here, sizeof(DWORD), dwOld, &dwOld);
#else // LINUX
	mprotect(p, sizeof(DWORD) & (psize - 1), PROT_READ | PROT_EXEC);
#endif // WIN32
}
