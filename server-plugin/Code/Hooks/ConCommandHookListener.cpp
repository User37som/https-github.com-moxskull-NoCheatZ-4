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

#include "ConCommandHookListener.h"

#include "Interfaces/InterfacesProxy.h"

#include "Misc/Helpers.h"
#include "plugin.h"
#include "Systems/ConfigManager.h"

ConCommandHookListener::ConCommandListenersListT ConCommandHookListener::m_listeners;

ConCommandHookListener::ConCommandHookListener()
{
}

ConCommandHookListener::~ConCommandHookListener()
{
}

void ConCommandHookListener::HookDispatch(void* cmd )
{
	/* Thanksfully we have the declaration of ConCommand so we can grab the
		vtable offset of Dispatch using the commented code below
		with the debugger in ASM mode :

		CSGO : (mov eax, edx+38h) = (56/4) = 14 for Windows
		CSS : (mov eax, edx+30h) = (48/4) = 12 for Windows
	
		CCommand empty;
		cmd->Dispatch(empty);
	*/	

	HookInfo info(cmd, ConfigManager::GetInstance()->GetVirtualFunctionId("dispatch"), (DWORD)nDispatch);
	HookGuard::GetInstance()->VirtualTableHook(info);
}

/*void ConCommandHookListener::UnhookDispatch(void* cmd)
{
	HookList<void>::elem_t* const hook_info = m_hooked_commands.FindByInstance(cmd);
	if (hook_info == nullptr) return;

	if (m_hooked_commands.FindByFunction(hook_info->m_value->oldFn, hook_info) == nullptr)
	{
		VirtualTableHook(hook_info->m_value->pInterface, ConfigManager::GetInstance()->GetVirtualFunctionId("dispatch"), (DWORD)hook_info->m_value->oldFn, (DWORD)nDispatch);
		m_hooked_commands.Remove(hook_info);
	}
}*/

/*void ConCommandHookListener::UnhookDispatch()
{
	HookList<void>::elem_t*  hook_info = m_hooked_commands.GetFirst();
	if (hook_info == nullptr) return;

	do
	{
		VirtualTableHook(hook_info->m_value->pInterface, ConfigManager::GetInstance()->GetVirtualFunctionId("dispatch"), (DWORD)hook_info->m_value->oldFn, (DWORD)nDispatch);
		hook_info = m_hooked_commands.Remove(hook_info);
	} while(hook_info != nullptr)
}*/

#ifdef GNUC
void HOOKFN_INT ConCommandHookListener::nDispatch(void* cmd, SourceSdk::CCommand const &args )
#else
void HOOKFN_INT ConCommandHookListener::nDispatch(void* cmd, void*, SourceSdk::CCommand const &args )
#endif
{
	const int index = GET_PLUGIN_COMMAND_INDEX();
	bool bypass = false;
	if(index >= PLUGIN_MIN_COMMAND_INDEX && index <= PLUGIN_MAX_COMMAND_INDEX)
	{
		PlayerHandler::const_iterator ph = NczPlayerManager::GetInstance()->GetPlayerHandlerByIndex(index);
	
		ConCommandListenersListT::elem_t* it = m_listeners.GetFirst();
		while (it != nullptr)
		{
			int const c = it->m_value.listener->m_mycommands.Find(cmd);
			if (c != -1)
			{
				bypass |= it->m_value.listener->ConCommandCallback(ph, cmd, args);
			}
			it = it->m_next;
		}
	}
	else if(index == 0)
	{
		bypass = false;

		ConCommandListenersListT::elem_t* it = m_listeners.GetFirst();
		while (it != nullptr)
		{
			int const c = it->m_value.listener->m_mycommands.Find(cmd);
			if (c != -1)
			{
				bypass |= it->m_value.listener->ConCommandCallback(nullptr, cmd, args);
			}
			it = it->m_next;
		}
	}
	else
	{
		bypass = true; // just in case ...
	}

	if(!bypass)
	{
		//Assert(it != nullptr);

		ST_W_STATIC Dispatch_t gpOldFn;
		*(DWORD*)&(gpOldFn) = HookGuard::GetInstance()->GetOldFunction(cmd, ConfigManager::GetInstance()->GetVirtualFunctionId("dispatch"));
		gpOldFn(cmd, args);
	}
}

void ConCommandHookListener::RegisterConCommandHookListener(ConCommandHookListener const * const listener)
{
	Assert(!listener->m_mycommands.IsEmpty());

	ConCommandListenersListT::elem_t* it = m_listeners.FindByListener(listener);
	if(it == nullptr)
	{
		it = m_listeners.Add(listener);
	}

	size_t cmd_pos = 0;
	size_t const max_pos = listener->m_mycommands.Size();
	do
	{
		HookDispatch(listener->m_mycommands[cmd_pos]);
	} while (++cmd_pos != max_pos);
}

void ConCommandHookListener::RemoveConCommandHookListener(ConCommandHookListener * const listener)
{
	/*if (!listener->m_mycommands.IsEmpty())
	{
		size_t cmd_pos = 0;
		size_t const max_pos = listener->m_mycommands.Size();
		do
		{
			bool can_unhook = true;
			for (ConCommandListenersListT::elem_t* z = m_listeners.GetFirst(); z != nullptr; z = z->m_next)
			{
				if (z->m_value.listener == listener) continue;

				if (z->m_value.listener->m_mycommands.Find(listener->m_mycommands[cmd_pos]) != -1)
				{
					can_unhook = false;
					break;
				}
			}
			if (can_unhook) UnhookDispatch(listener->m_mycommands[cmd_pos]);
		} while (++cmd_pos != max_pos);
		listener->m_mycommands.RemoveAll();
	}*/
	listener->m_mycommands.RemoveAll();
	m_listeners.Remove(listener);
}

