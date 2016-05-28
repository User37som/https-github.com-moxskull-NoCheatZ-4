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

#include <iostream>
#include <algorithm>

#include "SdkPreprocessors.h"
#include "Hook.h"
#include "Systems/Logger.h"
#include "Misc/Helpers.h"

void HookGuard::VirtualTableHook(HookInfo& info)
{ 
	if (m_list.HasElement(info)) return;

	info.oldFn = 0;
#ifdef WIN32
		DWORD dwOld;
		if(!VirtualProtect(info.vf_entry, 2 * sizeof(DWORD*), PAGE_EXECUTE_READWRITE, &dwOld ))
		{
			return;
		}
#else // LINUX
        uint32_t psize = sysconf(_SC_PAGESIZE);
		void *p = (void *)((DWORD)(info.vf_entry) & ~(psize-1));
		if(mprotect(p, ((2 * sizeof(void *)) + ((DWORD)(info.vf_entry) & (psize-1))), PROT_READ | PROT_WRITE | PROT_EXEC ) < 0)
		{
			return;
		}
#endif // WIN32
		
		if(info.oldFn && info.oldFn != *info.vf_entry)
			Logger::GetInstance()->Msg<MSG_WARNING>("Unexpected virtual table value in VirtualTableHook. Another plugin might be in conflict.");
		if(info.newFn == *info.vf_entry)
		{
			Logger::GetInstance()->Msg<MSG_WARNING>("Virtual function pointer was the same ...");
			return;
		}

		info.oldFn = *info.vf_entry;
		*info.vf_entry = info.newFn;

#ifdef WIN32
		VirtualProtect(info.vf_entry, 2 * sizeof(DWORD*), dwOld, &dwOld);
#else // LINUX
		mprotect(p, ((2 * sizeof(void *)) + ((DWORD)(info.vf_entry) & (psize - 1))), PROT_READ | PROT_EXEC);
#endif // WIN32
		DebugMessage(Helpers::format("VirtualTableHook : function 0x%X replaced by 0x%X.", info.oldFn, info.newFn));

		m_list.AddToTail(info);
}

void MoveVirtualFunction(DWORD const * const from, DWORD * const to)
{
#ifdef WIN32
	DWORD dwOld;
	if (!VirtualProtect(to, sizeof(DWORD), PAGE_EXECUTE_READWRITE, &dwOld))
	{
		return;
	}
#else // LINUX
	uint32_t psize = sysconf(_SC_PAGESIZE);
	void *p = (void *)((DWORD)(to) & ~(psize - 1));
	if (mprotect(p, sizeof(DWORD) & (psize - 1), PROT_READ | PROT_WRITE | PROT_EXEC) < 0)
	{
		return;
	}
#endif // WIN32
	DebugMessage(Helpers::format("MoveVirtualFunction : function 0x%X replaced by 0x%X.", *to, *from));
	*to = *from;
#ifdef WIN32
	VirtualProtect(to, sizeof(DWORD), dwOld, &dwOld);
#else // LINUX
	mprotect(p, sizeof(DWORD) & (psize - 1), PROT_READ | PROT_EXEC);
#endif // WIN32
}

DWORD HookGuard::GetOldFunction(void* class_ptr, int vfid) const
{
	int it = m_list.Find(HookInfo(class_ptr, vfid));
	if (it != -1)
	{
		return m_list[it].oldFn;
	}
	else
	{
		return 0;
	}
}

DWORD HookGuard::GetOldFunctionByVirtualTable(void * class_ptr) const
{
	DWORD* vt = ((DWORD*)*(DWORD*)class_ptr);
	for (hooked_list_t::const_iterator it = m_list.begin(); it != m_list.end(); ++it)
	{
		if (it->pInterface == vt)
		{
			return it->oldFn;
		}
	}
	return 0;
}

DWORD HookGuard::GetOldFunctionByInstance(void * class_ptr) const
{
	for (hooked_list_t::const_iterator it = m_list.begin(); it != m_list.end(); ++it)
	{
		if (it->origEnt == class_ptr)
		{
			return it->oldFn;
		}
	}
	return 0;
}

void HookGuard::GuardHooks()
{
	for (hooked_list_t::iterator it = m_list.begin(); it != m_list.end(); ++it)
	{
		if (*it->vf_entry != it->newFn)
		{
			it->oldFn = 0;
			DebugMessage(Helpers::format("HookGuard::GuardHooks : Re-hooking at %X.", *it->vf_entry));
			VirtualTableHook(*it); // rehook
		}
	}
}

void HookGuard::UnhookAll()
{
	for (hooked_list_t::iterator it = m_list.begin(); it != m_list.end(); ++it)
	{
		std::swap(it->newFn, it->oldFn);
		VirtualTableHook(*it); // unhook
	}
	m_list.RemoveAll();
}
