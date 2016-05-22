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

#include "Systems/BanRequest.h"

#include "Misc/Helpers.h"
#include "Misc/temp_Metrics.h"


BanRequest::BanRequest() :
	m_wait_time(10.0),
	m_do_writeid(false),
	TimerListener(),
	singleton_class()
{

}

void BanRequest::Init()
{
	TimerListener::AddTimerListener(this);
}

BanRequest::~BanRequest()
{
	TimerListener::RemoveTimerListener(this);
}

void BanRequest::SetWaitTime(float wait_time)
{
	m_wait_time = wait_time;
}

void BanRequest::AddAsyncBan(NczPlayer* player, int ban_time, const char * kick_message)
{
	PlayerBanRequestT req;
	req.userid = player->GetUserid();


	if(m_requests.Find(req) == nullptr)
	{
		req.ban_time = ban_time;
		req.request_time = Plat_FloatTime();
		req.kick_message = kick_message;
		strcpy_s(req.player_name, 24, player->GetName());
		strcpy_s(req.steamid, 24, player->GetSteamID());
		strcpy_s(req.ip, 24, player->GetIPAddress());
		strcpy_s(req.identity, 64, player->GetReadableIdentity().c_str());
		m_requests.Add(req);
	}

	AddTimer(m_wait_time, player->GetName(), true);
}

void BanRequest::TimerCallback(char const * const timer_name)
{
	BanRequestListT::elem_t* it = m_requests.GetFirst();
	float const curtime = Plat_FloatTime();
	while(it != nullptr)
	{
		if(it->m_value.request_time + m_wait_time < curtime)
		{
			Helpers::writeToLogfile(Helpers::format("%s banned (%f seconds after first detection).", it->m_value.identity, m_wait_time));
			if(SteamGameServer_BSecure())
			{
				SourceSdk::InterfacesProxy::Call_ServerCommand(Helpers::format("banid %d %s\n", it->m_value.ban_time, it->m_value.steamid).c_str());
			}
			SourceSdk::InterfacesProxy::Call_ServerCommand(Helpers::format("kickid %d [NoCheatZ 4] %s\n", it->m_value.userid, it->m_value.kick_message).c_str());
			if(!Helpers::bStrEq("127.0.0.1", it->m_value.ip))
			{
				SourceSdk::InterfacesProxy::Call_ServerCommand(Helpers::format("addip 1440 \"%s\"\n", it->m_value.ip).c_str());
			}
			SourceSdk::InterfacesProxy::Call_ServerExecute();

			it = m_requests.Remove(it);
			m_do_writeid = true;
		}
		else
		{
			it = it->m_next;
		}
	}
}

void BanRequest::WriteBansIfNeeded()
{
	if(m_do_writeid)
	{
		if(SteamGameServer_BSecure())
		{
			SourceSdk::InterfacesProxy::Call_ServerCommand("writeid\n");
		}
		SourceSdk::InterfacesProxy::Call_ServerCommand("writeip\n");
		SourceSdk::InterfacesProxy::Call_ServerExecute();
	}
}
