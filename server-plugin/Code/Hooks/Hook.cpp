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

uint32_t RAII_MemoryProtectDword::m_pagesize(0);

#ifdef GNUC
void RAII_MemoryProtectDword::SetPagesize()
{
	if (m_pagesize == 0)
	{
		int32_t psize(sysconf(_SC_PAGESIZE));
		if (psize < 0)
		{
			g_Logger.Msg<MSG_ERROR>(Helpers::format("sysconf error %X", errno));
			*m_errored = true;
			return;
		}
		m_pagesize = (uint32_t)psize;
	}
}
#endif

RAII_MemoryProtectDword::RAII_MemoryProtectDword(DWORD * addr, bool * err) :
	m_addr(addr),
	m_dwold(0),
	m_errored(err),
	a(nullptr),
	b(0)
{
#ifdef WIN32
	if (!VirtualProtect(addr, 2 * sizeof(DWORD*), PAGE_READWRITE, &m_dwold))
	{
		g_Logger.Msg<MSG_ERROR>(Helpers::format("VirtualProtect Error %X", GetLastError()));
		*m_errored = true;
		return;
	}
#else // LINUX
	SetPagesize();
	if (!*m_errored)
	{
		a = ((void *)((DWORD)(m_addr) & ~(m_pagesize - 1)));
		b = ((DWORD)(m_addr) & (m_pagesize - 1)) + ((2 * sizeof(void *)));
		if (mprotect(a, b, PROT_READ | PROT_WRITE) < 0)
		{
			g_Logger.Msg<MSG_ERROR>(Helpers::format("mprotect error %X", errno));
			*m_errored = true;
			return;
		}
	}
#endif // WIN32
}

RAII_MemoryProtectDword::~RAII_MemoryProtectDword()
{
	if (!*m_errored)
	{
#ifdef WIN32
		VirtualProtect(m_addr, 2 * sizeof(DWORD*), m_dwold, &m_dwold);
#else // LINUX
		mprotect(a, b, PROT_READ);
#endif // WIN32
	}
}

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
	bool err = false;
	RAII_MemoryProtectDword z(to, &err);
	if (!err)
	{
		DebugMessage(Helpers::format("MoveVirtualFunction : function 0x%X replaced by 0x%X.", *to, *from));
		*to = *from;
	}
}

DWORD ReplaceVirtualFunctionByFakeVirtual(DWORD const replace_by, DWORD * const replace_here)
{
	bool err = false;
	RAII_MemoryProtectDword z(replace_here, &err);
	DWORD old(0);
	if (!err)
	{
		DebugMessage(Helpers::format("ReplaceVirtualFunctionByFakeVirtual : function 0x%X replaced by 0x%X.", replace_by, *replace_here));
		old = *replace_here;
		*replace_here = replace_by;
	}
	return old;
}
