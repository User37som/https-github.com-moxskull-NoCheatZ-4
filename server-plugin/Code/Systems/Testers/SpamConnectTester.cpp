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

#include "SpamConnectTester.h"

#include "Interfaces/InterfacesProxy.h"

#include "Misc/Helpers.h"
#include "Systems/BanRequest.h"

SpamConnectTester::SpamConnectTester() :
	BaseSystem("SpamConnectTester", PLAYER_CONNECTING, INVALID, STATUS_EQUAL_OR_BETTER),
	singleton_class()
{
}

SpamConnectTester::~SpamConnectTester()
{
	Unload();
}

void SpamConnectTester::Init()
{
}

void SpamConnectTester::Load()
{
}

void SpamConnectTester::Unload()
{
	m_connect_list.RemoveAll();
}

void SpamConnectTester::ClientConnect( bool *bAllowConnect, SourceSdk::edict_t const * const pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	if(!*bAllowConnect) return;

	ConnectInfo sInfo(pszAddress);

	int pInfo = m_connect_list.Find(sInfo);

	if(pInfo == -1)
	{
		sInfo.OnConnect();
		sInfo.count = 1;
		m_connect_list.AddToHead(sInfo);
		return;
	}
	
	ConnectInfo & v = m_connect_list[pInfo];

	if(v.next_connect_time > Plat_FloatTime())
	{
		if(++(v.count) > 3)
		{
			*bAllowConnect = false;
			if(!Helpers::bStrEq("127.0.0.1", pszAddress))
			{
				SourceSdk::InterfacesProxy::Call_ServerCommand(Helpers::format("addip 20 \"%s\"\n", pszAddress).c_str());
			}
			strncpy(reject, "You are spam connecting. Please wait 20 minutes.", 48);
			m_connect_list.FindAndRemove(v);
			return;
		}
	}
	else
	{
		v.count = 1;
	}

	v.OnConnect();
}

SpamConnectTester g_SpamConnectTester;
