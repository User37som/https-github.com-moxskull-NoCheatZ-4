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

BaseSystem::BaseSystem(char const * const name, SlotStatus filter, SlotStatus load_filter, SlotFilterBehavior filter_behavior, char const * const commands) : 
	ListMeClass(),
	SlotFilter(filter, load_filter, filter_behavior),
	m_name(name),
	m_cmd_list(commands),
	m_isActive(false),
	m_isDisabled(false),
	m_configState(true),
	m_verbose(false)
{
}

BaseSystem::~BaseSystem()
{
}

void BaseSystem::UnloadAllSystems()
{
	BaseSystem* it = GetFirst();
	while (it != nullptr)
	{
		it->SetActive(false);
		GetNext(it);
	}
}

void BaseSystem::TryUnloadSystems()
{
	BaseSystem* it = GetFirst();
	while (it != nullptr)
	{
		if(it->ShouldUnload()) it->SetActive(false);
		GetNext(it);
	}
}

void BaseSystem::TryLoadSystems()
{
	BaseSystem* it = GetFirst();
	while (it != nullptr)
	{
		it->SetActive(true);
		GetNext(it);
	}
}

void BaseSystem::InitSystems()
{
	BaseSystem* it = GetFirst();
	while (it != nullptr)
	{
		it->Init();
		GetNext(it);
	}
}

BaseSystem * BaseSystem::FindSystemByName(const char * name)
{
	BaseSystem* it = GetFirst();
	while (it != nullptr)
	{
		if (stricmp(it->GetName(), name) == 0)
			return it;
		GetNext(it);
	}
	return nullptr;
}

bool BaseSystem::ShouldUnload()
{
	//if(GetTargetSlotStatus() == INVALID) return false;

	return (NczPlayerManager::GetInstance()->GetPlayerCount(GetLoadFilter(), STATUS_EQUAL_OR_BETTER) == 0);
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
				if(NczPlayerManager::GetInstance()->GetPlayerCount(GetLoadFilter(), STATUS_EQUAL_OR_BETTER) > 0)
				{
					Logger::GetInstance()->Msg<MSG_HINT>(Helpers::format("Starting %s", GetName()));
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
		Logger::GetInstance()->Msg<MSG_HINT>(Helpers::format("Stoping %s", GetName()));
		m_isActive = false;
		Unload();
	}
}

void BaseSystem::ncz_cmd_fn ( const SourceSdk::CCommand &args )
{
	if(args.ArgC() > 1)
	{
		BaseSystem* it = GetFirst();
		while (it != nullptr)
		{
			if(Helpers::bStriEq(it->GetName(), args.Arg(1)))
			{
				if(Helpers::bStriEq("enable", args.Arg(2)))
				{
					it->SetConfig(true);
					it->SetActive(true);
				}
				else if(Helpers::bStriEq("disable", args.Arg(2)))
				{
					it->SetConfig(false);
					it->SetActive(false);
				}
				else if(Helpers::bStriEq("verbose", args.Arg(2)))
				{
					if(args.ArgC() > 2)
					{
						it->m_verbose = atoi(args.Arg(3));
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
				else if(!it->sys_cmd_fn(args))
				{
					printf("action %s not found.\nTry : %s\n", args.Arg(2), it->cmd_list());
				}

				return;
			}	
			GetNext(it);
		}
		printf("System %s not found.\n", args.Arg(1));
	}
	else
	{
		printf("Usage: ncz system arg1 arg2 ...\n");
		printf("Systems list :\n");
		BaseSystem* it = GetFirst();
		while (it != nullptr)
		{
			printf("%s", it->GetName());
			if(it->IsActive())
			{
				printf(" (Running)\n");
			}
			else if(!it->IsEnabledByConfig())
			{
				printf(" (Disabled manually)\n");
			}
			else if(it->GetDisabledByConfigIni())
			{
				printf(" (Disabled by config.ini)\n");
			}
			else
			{
				printf(" (Sleeping - Waiting for players)\n");
			}
			printf("\tCommands : %s\n", it->cmd_list());
			GetNext(it);
		}
	}
}
