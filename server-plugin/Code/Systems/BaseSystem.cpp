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

#include <stdio.h>

#include "BaseSystem.h"

#include "Players/NczPlayerManager.h"
#include "Logger.h"


/////////////////////////////////////////////////////////////////////////
// BaseSystem
/////////////////////////////////////////////////////////////////////////

BaseSystem::SystemsListT BaseSystem::m_systemsList;

BaseSystem::BaseSystem(char const * const name, SlotStatus filter, SlotStatus load_filter, SlotFilterBehavior filter_behavior, char const * const commands) : m_name(name), m_cmd_list(commands), SlotFilter(filter, load_filter, filter_behavior), m_isActive(false), m_isDisabled(false), m_configState(true), m_verbose(false)
{
	BaseSystem::m_systemsList.Add(this);
}

BaseSystem::~BaseSystem()
{
	BaseSystem::m_systemsList.Remove(this);
}

void BaseSystem::UnloadAllSystems()
{
	SystemsListT::elem_t* it = m_systemsList.GetFirst();
	while (it != nullptr)
	{
		it->m_value->SetActive(false);
		it = it->m_next;
	}
}

void BaseSystem::TryUnloadSystems()
{
	SystemsListT::elem_t* it = m_systemsList.GetFirst();
	while (it != nullptr)
	{
		if(it->m_value->ShouldUnload()) it->m_value->SetActive(false);
		it = it->m_next;
	}
}

void BaseSystem::TryLoadSystems()
{
	SystemsListT::elem_t* it = m_systemsList.GetFirst();
	while (it != nullptr)
	{
		it->m_value->SetActive(true);
		it = it->m_next;
	}
}

void BaseSystem::InitSystems()
{
	SystemsListT::elem_t* it = m_systemsList.GetFirst();
	while (it != nullptr)
	{
		it->m_value->Init();
		it = it->m_next;
	}
}

BaseSystem * BaseSystem::FindSystemByName(const char * name)
{
	SystemsListT::elem_t* it = m_systemsList.GetFirst();
	while (it != nullptr)
	{
		if (stricmp(it->m_value->GetName(), name) == 0)
			return it->m_value;
		it = it->m_next;
	}
	return nullptr;
}

bool BaseSystem::ShouldUnload()
{
	if(GetTargetSlotStatus() == INVALID) return false;

	return (g_NczPlayerManager.GetPlayerCount(GetLoadFilter(), STATUS_EQUAL_OR_BETTER) == 0);
}

void BaseSystem::SetActive(bool active)
{
	if(m_isActive == active) return;
	else if(active)
	{
		if(!m_isDisabled)
		{
			if(m_configState)
			{
				// Should load
				if(g_NczPlayerManager.GetPlayerCount(GetTargetSlotStatus(), GetFilterBehavior()) > 0)
				{
					ILogger.Msg<MSG_HINT>(Helpers::format("Starting %s", GetName()));
					Load();
					m_isActive = true;
				}
			}
			else
			{
				SystemVerbose1(Helpers::format("Wont start system %s : Disabled by server configuration file", GetName()));
			}
		}
	}
	else
	{
		ILogger.Msg<MSG_HINT>(Helpers::format("Stoping %s", GetName()));
		m_isActive = false;
		Unload();
	}
}

void BaseSystem::ncz_cmd_fn ( const SourceSdk::CCommand &args )
{
	if(args.ArgC() > 1)
	{
		SystemsListT::elem_t* it = m_systemsList.GetFirst();
		while (it != nullptr)
		{
			if(Helpers::bStriEq(it->m_value->GetName(), args.Arg(1)))
			{
				if(Helpers::bStriEq("enable", args.Arg(2)))
				{
					it->m_value->SetConfig(true);
					it->m_value->SetActive(true);
				}
				else if(Helpers::bStriEq("disable", args.Arg(2)))
				{
					it->m_value->SetConfig(false);
					it->m_value->SetActive(false);
				}
				else if(Helpers::bStriEq("verbose", args.Arg(2)))
				{
					if(args.ArgC() > 2)
					{
						it->m_value->m_verbose = atoi(args.Arg(3));
					}
					return;
				}
#ifdef NCZ_USE_METRICS
				else if(Helpers::bStriEq("printmetrics", args.Arg(2)))
				{
					(*it)->GetMetrics().PrintAll();
				}
				else if(Helpers::bStriEq("resetmetrics", args.Arg(2)))
				{
					(*it)->GetMetrics().ResetAll();
				}
#endif
				else if(!it->m_value->sys_cmd_fn(args))
				{
					printf("action %s not found.\nTry : %s\n", args.Arg(2), it->m_value->cmd_list());
				}

				return;
			}	
			it = it->m_next;
		}
		printf("System %s not found.\n", args.Arg(1));
	}
	else
	{
		printf("Usage: ncz system arg1 arg2 ...\n");
		printf("Systems list :\n");
		SystemsListT::elem_t* it = m_systemsList.GetFirst();
		while (it != nullptr)
		{
			printf("%s", it->m_value->GetName());
			if(it->m_value->IsActive())
			{
				printf(" (Running)\n");
			}
			else if(!it->m_value->IsEnabledByConfig())
			{
				printf(" (Disabled by config)\n");
			}
			else
			{
				printf(" (Sleeping - Waiting for players)\n");
			}
			printf("Commands : %s\n", it->m_value->cmd_list());
			it = it->m_next;
		}
	}
}
